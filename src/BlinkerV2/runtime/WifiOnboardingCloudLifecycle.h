#ifndef BLINKER_RUNTIME_WIFIONBOARDINGCLOUDLIFECYCLE_H
#define BLINKER_RUNTIME_WIFIONBOARDINGCLOUDLIFECYCLE_H

#include <limits.h>

#include "../api/Client.h"
#include "../interface/IClock.h"
#include "../interface/IProductLifecycle.h"
#include "../transport/ManagedMqttTransport.h"
#include "WifiConnectionLifecycle.h"
#include "WifiControlPlaneArena.h"

namespace blinker {

enum class WifiOnboardingCloudState : uint8_t {
    Stopped = 0U,
    NetworkConnecting,
    Enrolling,
    CloudConnecting,
    Online,
    CloudError,
    Fault
};

struct WifiOnboardingCloudLifecycleConfig {
    uint32_t transientRetryMs;

    WifiOnboardingCloudLifecycleConfig() : transientRetryMs(5000U) {}
};

// Sequences one WiFi connection, the durable onboarding transaction and the
// existing Cloud Session/MQTT transport. The template keeps the concrete
// WifiOnboardingFlow allocation-free while allowing deterministic host fakes.
template <typename OnboardingFlow>
class BasicWifiOnboardingCloudLifecycle final : public IProductLifecycle {
public:
    BasicWifiOnboardingCloudLifecycle(
        WifiConnectionLifecycle& wifi,
        OnboardingFlow& onboarding,
        WifiControlPlaneArena& arena,
        ManagedMqttTransport& cloud,
        IClock& clock,
        const WifiOnboardingCloudLifecycleConfig& config =
            WifiOnboardingCloudLifecycleConfig())
        : wifi_(wifi),
          onboarding_(onboarding),
          arena_(arena),
          cloud_(cloud),
          clock_(clock),
          client_(nullptr),
          config_(config),
          retryAtMillis_(0U),
          state_(WifiOnboardingCloudState::Stopped),
          lastError_(ErrorCode::Ok),
          retryScheduled_(false) {}

    ~BasicWifiOnboardingCloudLifecycle() override { stop(); }

    Result attach(Client& client) override {
        if (state_ != WifiOnboardingCloudState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        const Result result = client.addTransport(cloud_);
        if (result) client_ = &client;
        return result;
    }

    Result start() override {
        if (state_ != WifiOnboardingCloudState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (config_.transientRetryMs == 0U ||
            config_.transientRetryMs > static_cast<uint32_t>(INT32_MAX)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }

        arena_.clear();
        cloud_.setNetworkAvailable(false);
        lastError_ = ErrorCode::Ok;
        retryAtMillis_ = 0U;
        retryScheduled_ = false;

        Result result = wifi_.start();
        if (result) result = client_->begin();
        if (!result) {
            wifi_.stop();
            arena_.clear();
            state_ = WifiOnboardingCloudState::Stopped;
            lastError_ = result.code();
            return result;
        }

        state_ = WifiOnboardingCloudState::NetworkConnecting;
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == WifiOnboardingCloudState::Stopped ||
            state_ == WifiOnboardingCloudState::Fault) {
            return;
        }

        wifi_.poll();
        if (wifi_.state() == WifiConnectionState::Fault) {
            enterFault(wifi_.lastError());
            return;
        }

        if (wifi_.online() && onboardingStarted() &&
            !onboardingComplete() &&
            onboarding_.state() != WifiOnboardingState::Fault) {
            onboarding_.poll(totalBudgetMicros);
        }

        if (wifi_.online() && !onboardingComplete()) {
            if (!onboardingStarted()) {
                beginOnboarding();
            } else if (onboarding_.active()) {
                finishOnboarding();
            } else if (onboarding_.state() ==
                       WifiOnboardingState::Fault) {
                retryOnboardingWhenDue();
            }
        }

        cloud_.setNetworkAvailable(
            wifi_.online() && onboardingComplete());
        if (client_ != nullptr) {
            client_->poll(totalBudgetMicros);
        }
        updateState();
    }

    void stop() override {
        if (state_ == WifiOnboardingCloudState::Stopped) return;
        cloud_.setNetworkAvailable(false);
        if (client_ != nullptr) client_->end();
        onboarding_.stop();
        retryScheduled_ = false;
        retryAtMillis_ = 0U;
        wifi_.stop();
        arena_.clear();
        lastError_ = ErrorCode::Ok;
        state_ = WifiOnboardingCloudState::Stopped;
    }

    ProductLifecycleStatus status() const override {
        ProductLifecycleState lifecycleState = ProductLifecycleState::Active;
        if (state_ == WifiOnboardingCloudState::Stopped) {
            lifecycleState = ProductLifecycleState::Stopped;
        } else if (state_ == WifiOnboardingCloudState::Fault) {
            lifecycleState = ProductLifecycleState::Fault;
        } else if (!onboardingComplete()) {
            lifecycleState =
                wifi_.state() == WifiConnectionState::WaitingForCredentials
                    ? ProductLifecycleState::Provisioning
                    : onboardingStarted()
                          ? ProductLifecycleState::Enrolling
                          : ProductLifecycleState::Starting;
        }
        return ProductLifecycleStatus(
            lifecycleState,
            lastError_,
            wifi_.online());
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(ProductCapabilityCloudData);
    }

    WifiOnboardingCloudState state() const { return state_; }

private:
    static bool timeReached(uint32_t now, uint32_t target) {
        return static_cast<int32_t>(now - target) >= 0;
    }

    bool onboardingStarted() const {
        return onboarding_.state() != WifiOnboardingState::Stopped;
    }

    bool onboardingComplete() const { return onboarding_.active(); }

    void beginOnboarding() {
        const Result result = onboarding_.start(
            arena_.onboardingRequest(),
            arena_.onboardingResponse(),
            arena_.onboardingWorkspace());
        if (onboarding_.active()) {
            finishOnboarding();
        } else if (!result || onboarding_.state() ==
                                  WifiOnboardingState::Fault) {
            handleOnboardingFailure(result.code());
        } else {
            state_ = WifiOnboardingCloudState::Enrolling;
            lastError_ = ErrorCode::Ok;
        }
    }

    void finishOnboarding() {
        retryScheduled_ = false;
        retryAtMillis_ = 0U;
        lastError_ = ErrorCode::Ok;
        state_ = WifiOnboardingCloudState::CloudConnecting;
    }

    void retryOnboardingWhenDue() {
        if (!retryScheduled_) {
            handleOnboardingFailure(onboarding_.lastError());
        }
        if (state_ == WifiOnboardingCloudState::Fault ||
            !timeReached(clock_.monotonicMillis(), retryAtMillis_)) {
            return;
        }

        retryScheduled_ = false;
        const Result result = onboarding_.retry();
        if (onboarding_.active()) {
            finishOnboarding();
        } else if (!result || onboarding_.state() ==
                                  WifiOnboardingState::Fault) {
            handleOnboardingFailure(result.code());
        } else {
            state_ = WifiOnboardingCloudState::Enrolling;
            lastError_ = ErrorCode::Ok;
        }
    }

    void handleOnboardingFailure(ErrorCode fallback) {
        const WifiOnboardingRetryClass retryClass =
            onboarding_.retryClass();
        if (onboarding_.state() != WifiOnboardingState::Fault ||
            retryClass == WifiOnboardingRetryClass::Never ||
            retryClass == WifiOnboardingRetryClass::ReplaceEnrollmentKey ||
            retryClass == WifiOnboardingRetryClass::ReclaimOwnership) {
            const ErrorCode error = onboarding_.lastError() != ErrorCode::Ok
                                        ? onboarding_.lastError()
                                        : fallback;
            enterFault(error);
            return;
        }

        uint32_t delay = config_.transientRetryMs;
        if (onboarding_.hasRetryAfter()) {
            const uint32_t maximumSeconds =
                static_cast<uint32_t>(INT32_MAX) / 1000U;
            const uint32_t seconds =
                onboarding_.retryAfterSeconds() > maximumSeconds
                    ? maximumSeconds
                    : onboarding_.retryAfterSeconds();
            delay = seconds * 1000U;
        }
        retryAtMillis_ = clock_.monotonicMillis() + delay;
        retryScheduled_ = true;
        lastError_ = onboarding_.lastError() != ErrorCode::Ok
                         ? onboarding_.lastError()
                         : fallback;
        state_ = WifiOnboardingCloudState::Enrolling;
    }

    void updateState() {
        if (state_ == WifiOnboardingCloudState::Stopped ||
            state_ == WifiOnboardingCloudState::Fault) {
            return;
        }
        if (!wifi_.online()) {
            state_ = WifiOnboardingCloudState::NetworkConnecting;
        } else if (!onboardingComplete()) {
            state_ = WifiOnboardingCloudState::Enrolling;
        } else if (cloud_.state() == TransportState::Online) {
            state_ = WifiOnboardingCloudState::Online;
            lastError_ = ErrorCode::Ok;
        } else if (cloud_.state() == TransportState::Error) {
            state_ = WifiOnboardingCloudState::CloudError;
            lastError_ = cloud_.lastError();
        } else {
            state_ = WifiOnboardingCloudState::CloudConnecting;
            lastError_ = ErrorCode::Ok;
        }
    }

    void enterFault(ErrorCode error) {
        cloud_.setNetworkAvailable(false);
        if (client_ != nullptr) client_->end();
        onboarding_.stop();
        retryScheduled_ = false;
        wifi_.stop();
        arena_.clear();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = WifiOnboardingCloudState::Fault;
    }

    WifiConnectionLifecycle& wifi_;
    OnboardingFlow& onboarding_;
    WifiControlPlaneArena& arena_;
    ManagedMqttTransport& cloud_;
    IClock& clock_;
    Client* client_;
    WifiOnboardingCloudLifecycleConfig config_;
    uint32_t retryAtMillis_;
    WifiOnboardingCloudState state_;
    ErrorCode lastError_;
    bool retryScheduled_;

    BasicWifiOnboardingCloudLifecycle(
        const BasicWifiOnboardingCloudLifecycle&);
    BasicWifiOnboardingCloudLifecycle& operator=(
        const BasicWifiOnboardingCloudLifecycle&);
};

typedef BasicWifiOnboardingCloudLifecycle<WifiOnboardingFlow>
    WifiOnboardingCloudLifecycle;

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(WifiOnboardingCloudLifecycle) <= 40U,
    "WiFi onboarding/cloud lifecycle exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
