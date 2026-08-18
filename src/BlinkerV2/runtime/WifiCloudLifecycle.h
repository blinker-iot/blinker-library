#ifndef BLINKER_RUNTIME_WIFICLOUDLIFECYCLE_H
#define BLINKER_RUNTIME_WIFICLOUDLIFECYCLE_H

#include "../api/Client.h"
#include "../interface/IProductLifecycle.h"
#include "../transport/ManagedMqttTransport.h"
#include "WifiConnectionLifecycle.h"

namespace blinker {

enum class WifiCloudLifecycleState : uint8_t {
    Stopped = 0U,
    NetworkConnecting,
    CloudConnecting,
    Online,
    Fault
};

// Portable WiFi-only product ordering. Identity/session policy stays inside
// the supplied cloud transport; this lifecycle only coordinates connectivity,
// Client polling and terminal fault rollback. The template parameters are
// test seams, not user-facing platform variants.
template <typename WifiConnection, typename CloudTransport>
class BasicWifiCloudLifecycle final : public IProductLifecycle {
public:
    BasicWifiCloudLifecycle(
        WifiConnection& wifi,
        CloudTransport& cloud)
        : wifi_(wifi),
          cloud_(cloud),
          client_(nullptr),
          state_(WifiCloudLifecycleState::Stopped),
          lastError_(ErrorCode::Ok) {}

    ~BasicWifiCloudLifecycle() override { stop(); }

    Result attach(Client& client) override {
        if (state_ != WifiCloudLifecycleState::Stopped ||
            client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        const Result result = client.addTransport(cloud_);
        if (result) client_ = &client;
        return result;
    }

    Result start() override {
        if (state_ != WifiCloudLifecycleState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }

        lastError_ = ErrorCode::Ok;
        cloud_.setNetworkAvailable(false);
        Result result = wifi_.start();
        if (result) result = client_->begin();
        if (!result) return failStart(result.code());
        state_ = WifiCloudLifecycleState::NetworkConnecting;
        updateState();
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == WifiCloudLifecycleState::Stopped ||
            state_ == WifiCloudLifecycleState::Fault) {
            return;
        }

        wifi_.poll();
        if (wifi_.state() == WifiConnectionState::Fault) {
            enterFault(wifi_.lastError());
            return;
        }

        cloud_.setNetworkAvailable(wifi_.online());
        client_->poll(totalBudgetMicros);
        if (cloud_.state() == TransportState::Error) {
            enterFault(cloud_.lastError());
            return;
        }
        updateState();
    }

    void stop() override {
        if (state_ == WifiCloudLifecycleState::Stopped) return;
        cloud_.setNetworkAvailable(false);
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = ErrorCode::Ok;
        state_ = WifiCloudLifecycleState::Stopped;
    }

    ProductLifecycleStatus status() const override {
        ProductLifecycleState product = ProductLifecycleState::Starting;
        if (state_ == WifiCloudLifecycleState::Stopped) {
            product = ProductLifecycleState::Stopped;
        } else if (state_ == WifiCloudLifecycleState::Fault) {
            product = ProductLifecycleState::Fault;
        } else if (state_ == WifiCloudLifecycleState::Online) {
            product = ProductLifecycleState::Active;
        } else if (wifi_.state() ==
                   WifiConnectionState::WaitingForCredentials) {
            product = ProductLifecycleState::Provisioning;
        }
        return ProductLifecycleStatus(product, lastError_, wifi_.online());
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(ProductCapabilityCloudData);
    }

    WifiCloudLifecycleState state() const { return state_; }

private:
    Result failStart(ErrorCode error) {
        cloud_.setNetworkAvailable(false);
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = WifiCloudLifecycleState::Stopped;
        return Result::failure(lastError_);
    }

    void updateState() {
        if (state_ == WifiCloudLifecycleState::Stopped ||
            state_ == WifiCloudLifecycleState::Fault) {
            return;
        }
        if (!wifi_.online()) {
            state_ = WifiCloudLifecycleState::NetworkConnecting;
            lastError_ = wifi_.lastError();
        } else if (cloud_.state() == TransportState::Online) {
            state_ = WifiCloudLifecycleState::Online;
            lastError_ = ErrorCode::Ok;
        } else {
            state_ = WifiCloudLifecycleState::CloudConnecting;
            lastError_ = ErrorCode::Ok;
        }
    }

    void enterFault(ErrorCode error) {
        cloud_.setNetworkAvailable(false);
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = WifiCloudLifecycleState::Fault;
    }

    WifiConnection& wifi_;
    CloudTransport& cloud_;
    Client* client_;
    WifiCloudLifecycleState state_;
    ErrorCode lastError_;

    BasicWifiCloudLifecycle(const BasicWifiCloudLifecycle&);
    BasicWifiCloudLifecycle& operator=(const BasicWifiCloudLifecycle&);
};

typedef BasicWifiCloudLifecycle<
    WifiConnectionLifecycle,
    ManagedMqttTransport>
    WifiCloudLifecycle;

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(WifiCloudLifecycle) <= 20U,
    "WifiCloudLifecycle exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
