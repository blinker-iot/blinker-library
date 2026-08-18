#ifndef BLINKER_OFFICIAL_ESP32_PROVISIONED_DEVICE_KEY_WIFI_COMPOSITION_H
#define BLINKER_OFFICIAL_ESP32_PROVISIONED_DEVICE_KEY_WIFI_COMPOSITION_H

#include "BlinkerOfficialDeviceKeyWifiProduct.h"

#include <BlinkerEsp32WifiProvAdapter.h>

#include <string.h>

namespace blinker {
namespace integration {
namespace official_detail {

enum class ProvisionedDeviceKeyWifiState : uint8_t {
    Stopped = 0U,
    Provisioning,
    Cloud,
    Fault
};

template <typename Platform>
class NoProvisionedWifiExtension {
public:
    NoProvisionedWifiExtension(
        Platform&,
        const DeviceInstanceId&,
        ArduinoClock&,
        PlatformHardwareRandom&) {}

    Result attach(Client&) { return Result::success(); }
    IDeviceAccessStore* deviceAccessStore() { return nullptr; }
    uint16_t capabilities() const { return 0U; }
};

class Esp32BleWifiProvPolicy {
public:
    Result configure(StringView serviceKey) {
        return serviceKey.empty()
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }

    Esp32WifiProvConfig makeConfig(
        const char* serviceName,
        const char* proofOfPossession) const {
        return esp32BleWifiProvConfig(
            serviceName,
            proofOfPossession);
    }

    uint16_t capabilities() const { return ProductCapabilityBleSetup; }
};

class Esp32SoftApWifiProvPolicy {
public:
    Esp32SoftApWifiProvPolicy() : serviceKey_() {}
    ~Esp32SoftApWifiProvPolicy() { clear(); }

    Result configure(StringView serviceKey) {
        if (serviceKey.data == nullptr || serviceKey.size < 8U ||
            serviceKey.size >= sizeof(serviceKey_)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        memcpy(serviceKey_, serviceKey.data, serviceKey.size);
        serviceKey_[serviceKey.size] = '\0';
        return Result::success();
    }

    Esp32WifiProvConfig makeConfig(
        const char* serviceName,
        const char* proofOfPossession) const {
        return esp32SoftApWifiProvConfig(
            serviceName,
            proofOfPossession,
            serviceKey_);
    }

    uint16_t capabilities() const { return 0U; }

private:
    void clear() {
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(serviceKey_),
            sizeof(serviceKey_)));
    }

    char serviceKey_[64U];

    Esp32SoftApWifiProvPolicy(const Esp32SoftApWifiProvPolicy&);
    Esp32SoftApWifiProvPolicy& operator=(
        const Esp32SoftApWifiProvPolicy&);
};

// Shared ESP32 WiFiProv -> DeviceKey cloud composition. P3 selects BLE setup
// without a Direct extension; P6 selects SoftAP setup plus NimBLE Direct.
// Both therefore keep one onboarding and cloud implementation.
template <
    typename Platform,
    typename Extension,
    typename ProvisioningPolicy>
class Esp32ProvisionedDeviceKeyWifiComposition final
    : public IProductLifecycle {
public:
    Esp32ProvisionedDeviceKeyWifiComposition()
        : platform_(),
          stack_(platform_, platform_.deviceKeys()),
          extension_(
              platform_,
              stack_.deviceInstance(),
              stack_.clock(),
              stack_.random()),
          endpoint_(
              stack_.deviceInstance(),
              platform_.deviceKeys(),
              extension_.deviceAccessStore()),
          provisioner_(endpoint_, platform_.wifiCredentials()),
          cloud_(stack_.wifiLifecycle(), stack_.cloudTransport()),
          provisioningPolicy_(),
          client_(nullptr),
          serviceName_(),
          proofOfPossession_(),
          state_(ProvisionedDeviceKeyWifiState::Stopped),
          lastError_(ErrorCode::Ok),
          configured_(false),
          forceProvisioning_(false) {}

    ~Esp32ProvisionedDeviceKeyWifiComposition() override {
        stop();
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(serviceName_),
            sizeof(serviceName_)));
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(proofOfPossession_),
            sizeof(proofOfPossession_)));
    }

    Result configure(
        StringView serviceName,
        StringView proofOfPossession,
        StringView provisioningServiceKey,
        bool forceProvisioning) {
        if (state_ != ProvisionedDeviceKeyWifiState::Stopped || configured_) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (!copyText(serviceName, serviceName_, sizeof(serviceName_)) ||
            !copyText(
                proofOfPossession,
                proofOfPossession_,
                sizeof(proofOfPossession_))) {
            lastError_ = ErrorCode::InvalidArgument;
            return Result::failure(lastError_);
        }
        const Result policy =
            provisioningPolicy_.configure(provisioningServiceKey);
        if (!policy) {
            secureZero(MutableByteSpan(
                reinterpret_cast<uint8_t*>(serviceName_),
                sizeof(serviceName_)));
            secureZero(MutableByteSpan(
                reinterpret_cast<uint8_t*>(proofOfPossession_),
                sizeof(proofOfPossession_)));
            lastError_ = policy.code();
            return policy;
        }
        configured_ = true;
        forceProvisioning_ = forceProvisioning;
        return Result::success();
    }

    Result attach(Client& client) override {
        if (client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }
        Result result = extension_.attach(client);
        if (result) result = cloud_.attach(client);
        if (result) client_ = &client;
        return result;
    }

    Result start() override {
        if (state_ != ProvisionedDeviceKeyWifiState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (!configured_ || client_ == nullptr) {
            return failStart(ErrorCode::NotConfigured);
        }

        Result result = stack_.open();
        if (!result) return failStart(result.code());

        DeviceKey current;
        result = platform_.deviceKeys().load(current);
        clearDeviceKey(current);
        const bool hasDeviceKey = result.ok();
        if (!result && result.code() != ErrorCode::NotFound) {
            return failStart(result.code());
        }

        if (!forceProvisioning_ && hasDeviceKey &&
            platform_.wifiCredentials().hasActiveProfile()) {
            return startCloud();
        }

        const Esp32WifiProvConfig config =
            provisioningPolicy_.makeConfig(
                serviceName_,
                proofOfPossession_);
        forceProvisioning_ = false;
        result = provisioner_.begin(config);
        if (!result) return failStart(result.code());

        state_ = ProvisionedDeviceKeyWifiState::Provisioning;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == ProvisionedDeviceKeyWifiState::Provisioning) {
            provisioner_.poll();
            if (provisioner_.state() == Esp32WifiProvState::Fault) {
                const ErrorCode error = provisioner_.lastError();
                provisioner_.end();
                enterFault(error);
            } else if (provisioner_.state() ==
                       Esp32WifiProvState::Complete) {
                provisioner_.end();
                (void)startCloud();
            }
        } else if (state_ == ProvisionedDeviceKeyWifiState::Cloud) {
            cloud_.poll(totalBudgetMicros);
            const ProductLifecycleStatus current = cloud_.status();
            if (current.state == ProductLifecycleState::Fault) {
                enterFault(current.lastError);
            }
        }
    }

    void stop() override {
        provisioner_.end();
        cloud_.stop();
        stack_.end();
        state_ = ProvisionedDeviceKeyWifiState::Stopped;
        lastError_ = ErrorCode::Ok;
    }

    ProductLifecycleStatus status() const override {
        if (state_ == ProvisionedDeviceKeyWifiState::Stopped) {
            return ProductLifecycleStatus();
        }
        if (state_ == ProvisionedDeviceKeyWifiState::Provisioning) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Provisioning,
                lastError_,
                false);
        }
        if (state_ == ProvisionedDeviceKeyWifiState::Fault) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Fault,
                lastError_,
                false);
        }
        return cloud_.status();
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(static_cast<uint16_t>(
            ProductCapabilityCloudData |
            provisioningPolicy_.capabilities() |
            extension_.capabilities()));
    }

private:
    static bool copyText(
        StringView source,
        char* destination,
        size_t capacity) {
        if (source.data == nullptr || source.empty() ||
            source.size >= capacity) {
            return false;
        }
        memcpy(destination, source.data, source.size);
        destination[source.size] = '\0';
        return true;
    }

    Result startCloud() {
        Result result = stack_.begin();
        if (result) result = cloud_.start();
        if (!result) return failStart(result.code());
        state_ = ProvisionedDeviceKeyWifiState::Cloud;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    Result failStart(ErrorCode error) {
        enterFault(error);
        return Result::failure(lastError_);
    }

    void enterFault(ErrorCode error) {
        provisioner_.end();
        cloud_.stop();
        stack_.end();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = ProvisionedDeviceKeyWifiState::Fault;
    }

    Platform platform_;
    DeviceKeyWifiStack<Platform> stack_;
    Extension extension_;
    DeviceKeyProvisioningEndpoint endpoint_;
    Esp32WifiProvAdapter provisioner_;
    WifiCloudLifecycle cloud_;
    ProvisioningPolicy provisioningPolicy_;
    Client* client_;
    char serviceName_[32U];
    char proofOfPossession_[65U];
    ProvisionedDeviceKeyWifiState state_;
    ErrorCode lastError_;
    bool configured_;
    bool forceProvisioning_;

    Esp32ProvisionedDeviceKeyWifiComposition(
        const Esp32ProvisionedDeviceKeyWifiComposition&);
    Esp32ProvisionedDeviceKeyWifiComposition& operator=(
        const Esp32ProvisionedDeviceKeyWifiComposition&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
