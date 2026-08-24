#ifndef BLINKER_RUNTIME_BLEONLYLIFECYCLE_H
#define BLINKER_RUNTIME_BLEONLYLIFECYCLE_H

#include "BleSetupLifecycle.h"
#include "../api/Client.h"
#include "../interface/IProductLifecycle.h"
#include "../provisioning/BleLocalProvisioningEndpoint.h"

namespace blinker {

enum class BleOnlyLifecycleState : uint8_t {
    Stopped = 0U,
    Provisioning,
    Direct,
    Fault
};

template <typename ProvisioningEndpoint>
class BasicBleOnlyLifecycle final : public IProductLifecycle {
public:
    BasicBleOnlyLifecycle(
        BleModeCoordinator& ble,
        ProvisioningEndpoint& provisioning,
        IBleSetupCompletion& completion,
        IFrameTransport& directBle,
        IAuthorizationProvider& directAuthorization,
        IRandom& random,
        const BleSetupLifecycleConfig& config = BleSetupLifecycleConfig())
        : bleSetup_(ble, provisioning, completion, random, config),
          directBle_(directBle),
          directAuthorization_(directAuthorization), client_(nullptr),
          state_(BleOnlyLifecycleState::Stopped),
          lastError_(ErrorCode::Ok) {}

    ~BasicBleOnlyLifecycle() override { stop(); }

    Result attach(Client& client) override {
        if (state_ != BleOnlyLifecycleState::Stopped || client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        Result result = client.addTransport(
            directBle_, TransportLifecyclePolicy::External);
        if (result) {
            result = client.setAuthorizationProvider(&directAuthorization_);
        }
        if (!result) {
            (void)client.removeTransport(directBle_);
            return result;
        }
        client_ = &client;
        return Result::success();
    }

    Result start() override {
        if (state_ != BleOnlyLifecycleState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        Result result = client_->begin();
        if (result) result = bleSetup_.start(true);
        if (!result) {
            bleSetup_.stop();
            client_->end();
            lastError_ = result.code();
            return result;
        }
        lastError_ = ErrorCode::Ok;
        updateState();
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == BleOnlyLifecycleState::Stopped ||
            state_ == BleOnlyLifecycleState::Fault) {
            return;
        }
        const uint32_t bleBudget = totalBudgetMicros / 2U;
        Result result = bleSetup_.poll(true, bleBudget);
        if (!result) {
            enterFault(result.code());
            return;
        }
        client_->poll(totalBudgetMicros - bleBudget);
        updateState();
    }

    void stop() override {
        if (state_ == BleOnlyLifecycleState::Stopped) return;
        bleSetup_.stop();
        if (client_ != nullptr) client_->end();
        state_ = BleOnlyLifecycleState::Stopped;
        lastError_ = ErrorCode::Ok;
    }

    ProductLifecycleStatus status() const override {
        ProductLifecycleState product = ProductLifecycleState::Active;
        if (state_ == BleOnlyLifecycleState::Stopped) {
            product = ProductLifecycleState::Stopped;
        } else if (state_ == BleOnlyLifecycleState::Provisioning) {
            product = ProductLifecycleState::Provisioning;
        } else if (state_ == BleOnlyLifecycleState::Fault) {
            product = ProductLifecycleState::Fault;
        }
        return ProductLifecycleStatus(product, lastError_, false);
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(static_cast<uint16_t>(
            ProductCapabilityDirectBleData |
            ProductCapabilityBleSetup));
    }

    BleOnlyLifecycleState state() const { return state_; }
    bool setupComplete() const { return bleSetup_.setupComplete(); }

private:
    void updateState() {
        state_ = bleSetup_.activeMode() == ble::ApplicationMode::Provisioning
                     ? BleOnlyLifecycleState::Provisioning
                     : BleOnlyLifecycleState::Direct;
        lastError_ = ErrorCode::Ok;
    }

    void enterFault(ErrorCode error) {
        bleSetup_.stop();
        if (client_ != nullptr) client_->end();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = BleOnlyLifecycleState::Fault;
    }

    BasicBleSetupLifecycle<ProvisioningEndpoint> bleSetup_;
    IFrameTransport& directBle_;
    IAuthorizationProvider& directAuthorization_;
    Client* client_;
    BleOnlyLifecycleState state_;
    ErrorCode lastError_;
};

typedef BasicBleOnlyLifecycle<BleLocalProvisioningEndpoint>
    BleOnlyLifecycle;

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(BleOnlyLifecycle) <= 48U,
    "BLE-only lifecycle exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
