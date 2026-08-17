#ifndef BLINKER_RUNTIME_WIFIBLELIFECYCLE_H
#define BLINKER_RUNTIME_WIFIBLELIFECYCLE_H

#include <stdint.h>

#include "../api/Client.h"
#include "../interface/IProductLifecycle.h"
#include "../provisioning/BleLocalProvisioningEndpoint.h"
#include "../transport/ManagedMqttTransport.h"
#include "BleSetupLifecycle.h"
#include "WifiConnectionLifecycle.h"

namespace blinker {

enum class WifiBleLifecycleState : uint8_t {
    Stopped = 0U,
    Provisioning,
    Direct,
    CloudConnecting,
    Online,
    CloudError,
    Fault
};

struct WifiBleLifecycleConfig {
    bool useProvisioningPsk;

    WifiBleLifecycleConfig() : useProvisioningPsk(false) {}
};

// Coordinates one physical BLE radio, one WiFi station and one Device Client.
// The direct BLE transport is registered as External because
// BleModeCoordinator alone owns its start/stop/poll lifecycle. Provisioning
// and direct BBP/2 therefore never compete for the same GATT callbacks.
//
// The three template parameters are dependency seams, not product variants.
// Official builds use the typedef below; host tests can supply small fakes
// without adding another virtual interface to constrained targets.
template <
    typename WifiConnection,
    typename CloudTransport,
    typename ProvisioningEndpoint>
class BasicWifiBleLifecycle final : public IProductLifecycle {
public:
    BasicWifiBleLifecycle(
        WifiConnection& wifi,
        CloudTransport& cloud,
        BleModeCoordinator& ble,
        ProvisioningEndpoint& provisioning,
        IBleSetupCompletion& completion,
        IFrameTransport& directBle,
        IAuthorizationProvider& directAuthorization,
        IRandom& random,
        const WifiBleLifecycleConfig& config = WifiBleLifecycleConfig())
        : wifi_(wifi),
          cloud_(cloud),
          bleSetup_(
              ble,
              provisioning,
              completion,
              random,
              bleSetupConfig(config)),
          directBle_(directBle),
          directAuthorization_(directAuthorization),
          client_(nullptr),
          state_(WifiBleLifecycleState::Stopped),
          lastError_(ErrorCode::Ok) {}

    ~BasicWifiBleLifecycle() override { stop(); }

    Result attach(Client& client) override {
        if (state_ != WifiBleLifecycleState::Stopped || client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }

        Result result = client.addTransport(cloud_);
        if (!result) return result;
        result = client.addTransport(
            directBle_, TransportLifecyclePolicy::External);
        if (!result) {
            (void)client.removeTransport(cloud_);
            return result;
        }
        result = client.setAuthorizationProvider(&directAuthorization_);
        if (!result) {
            (void)client.removeTransport(directBle_);
            (void)client.removeTransport(cloud_);
            return result;
        }
        client_ = &client;
        return Result::success();
    }

    Result start() override {
        if (state_ != WifiBleLifecycleState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return Result::failure(ErrorCode::NotConfigured);
        }

        lastError_ = ErrorCode::Ok;
        cloud_.setNetworkAvailable(false);
        Result result = wifi_.start();
        if (result) result = client_->begin();
        if (result) result = bleSetup_.start(wifi_.confirmedProfile());
        if (!result) {
            return failStart(result.code());
        }

        state_ = bleSetup_.activeMode() == ble::ApplicationMode::Provisioning
                     ? WifiBleLifecycleState::Provisioning
                     : WifiBleLifecycleState::Direct;
        updateState();
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == WifiBleLifecycleState::Stopped ||
            state_ == WifiBleLifecycleState::Fault) {
            return;
        }

        const uint32_t bleBudget = totalBudgetMicros / 2U;
        Result result = bleSetup_.poll(
            wifi_.confirmedProfile(), bleBudget);
        if (!result) {
            enterFault(result.code());
            return;
        }

        wifi_.poll();
        if (wifi_.state() == WifiConnectionState::Fault) {
            enterFault(wifi_.lastError());
            return;
        }

        cloud_.setNetworkAvailable(
            bleSetup_.setupComplete() && wifi_.online());
        client_->poll(totalBudgetMicros - bleBudget);
        updateState();
    }

    void stop() override {
        if (state_ == WifiBleLifecycleState::Stopped) return;
        cloud_.setNetworkAvailable(false);
        bleSetup_.stop();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = ErrorCode::Ok;
        state_ = WifiBleLifecycleState::Stopped;
    }

    ProductLifecycleStatus status() const override {
        ProductLifecycleState productState = ProductLifecycleState::Active;
        if (state_ == WifiBleLifecycleState::Stopped) {
            productState = ProductLifecycleState::Stopped;
        } else if (state_ == WifiBleLifecycleState::Fault) {
            productState = ProductLifecycleState::Fault;
        } else if (state_ == WifiBleLifecycleState::Provisioning) {
            productState = ProductLifecycleState::Provisioning;
        }
        return ProductLifecycleStatus(
            productState, lastError_, wifi_.online());
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(static_cast<uint16_t>(
            ProductCapabilityCloudData |
            ProductCapabilityDirectBleData |
            ProductCapabilityBleSetup));
    }

    WifiBleLifecycleState state() const { return state_; }
    bool setupComplete() const { return bleSetup_.setupComplete(); }

private:
    static BleSetupLifecycleConfig bleSetupConfig(
        const WifiBleLifecycleConfig& config) {
        BleSetupLifecycleConfig value;
        value.contract = BleSetupContract::PlatformEnrollment;
        value.acceptsWifiConfig = true;
        value.useProvisioningPsk = config.useProvisioningPsk;
        return value;
    }

    Result failStart(ErrorCode error) {
        cloud_.setNetworkAvailable(false);
        bleSetup_.stop();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = WifiBleLifecycleState::Stopped;
        return Result::failure(lastError_);
    }

    void updateState() {
        if (state_ == WifiBleLifecycleState::Stopped ||
            state_ == WifiBleLifecycleState::Fault) {
            return;
        }
        if (bleSetup_.activeMode() == ble::ApplicationMode::Provisioning) {
            state_ = WifiBleLifecycleState::Provisioning;
            lastError_ = wifi_.lastError();
        } else if (!wifi_.online()) {
            state_ = WifiBleLifecycleState::Direct;
            lastError_ = wifi_.lastError();
        } else if (cloud_.state() == TransportState::Online) {
            state_ = WifiBleLifecycleState::Online;
            lastError_ = ErrorCode::Ok;
        } else if (cloud_.state() == TransportState::Error) {
            state_ = WifiBleLifecycleState::CloudError;
            lastError_ = cloud_.lastError();
        } else {
            state_ = WifiBleLifecycleState::CloudConnecting;
            lastError_ = ErrorCode::Ok;
        }
    }

    void enterFault(ErrorCode error) {
        cloud_.setNetworkAvailable(false);
        bleSetup_.stop();
        if (client_ != nullptr) client_->end();
        wifi_.stop();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = WifiBleLifecycleState::Fault;
    }

    WifiConnection& wifi_;
    CloudTransport& cloud_;
    BasicBleSetupLifecycle<ProvisioningEndpoint> bleSetup_;
    IFrameTransport& directBle_;
    IAuthorizationProvider& directAuthorization_;
    Client* client_;
    WifiBleLifecycleState state_;
    ErrorCode lastError_;

    BasicWifiBleLifecycle(const BasicWifiBleLifecycle&);
    BasicWifiBleLifecycle& operator=(const BasicWifiBleLifecycle&);
};

typedef BasicWifiBleLifecycle<
    WifiConnectionLifecycle,
    ManagedMqttTransport,
    BleLocalProvisioningEndpoint>
    WifiBleLifecycle;

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(WifiBleLifecycle) <= 52U,
    "WiFi+BLE lifecycle exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
