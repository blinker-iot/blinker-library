#ifndef BLINKER_OFFICIAL_DEVICE_KEY_WIFI_COMPOSITION_H
#define BLINKER_OFFICIAL_DEVICE_KEY_WIFI_COMPOSITION_H

#include "BlinkerOfficialDeviceKeyWifiStack.h"

#include <string.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Internal candidate for the next public WiFi profile. It intentionally has
// no public facade binding until the APP/service DeviceKey endpoints exist.
template <typename Platform>
class DeviceKeyWifiComposition final : public IProductLifecycle {
public:
    DeviceKeyWifiComposition()
        : platform_(),
          deviceKey_(),
          stack_(platform_, deviceKey_),
          lifecycle_(
              stack_.wifiLifecycle(),
              stack_.cloudTransport()),
          setupSsid_(),
          setupCredential_(),
          setupSsidSize_(0U),
          setupCredentialSize_(0U),
          configurationError_(ErrorCode::Ok),
          setupConfigured_(false) {}

    ~DeviceKeyWifiComposition() override {
        lifecycle_.stop();
        stack_.end();
        clearNetworkSetup();
    }

    Result configure(
        StringView ssid,
        StringView password,
        StringView deviceKey) {
        if (stack_.initialized() || setupConfigured_) {
            return remember(ErrorCode::AlreadyExists);
        }

        WifiNetworkConfig network;
        network.ssid = asBytes(ssid);
        network.authentication = password.empty()
                                     ? WifiAuthentication::Open
                                     : WifiAuthentication::Wpa2Personal;
        network.credential = asBytes(password);
        Result result = validateWifiNetworkConfig(network);
        if (result) result = deviceKey_.configure(deviceKey);
        if (!result) return remember(result.code());

        memcpy(setupSsid_, network.ssid.data, network.ssid.size);
        if (!network.credential.empty()) {
            memcpy(
                setupCredential_,
                network.credential.data,
                network.credential.size);
        }
        setupSsidSize_ = static_cast<uint8_t>(network.ssid.size);
        setupCredentialSize_ =
            static_cast<uint8_t>(network.credential.size);
        setupConfigured_ = true;
        return Result::success();
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = lifecycle_.attach(client);
        if (!result) configurationError_ = result.code();
        return result;
    }

    Result start() override {
        const Result result = stack_.initialized()
                                  ? lifecycle_.start()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        if (!result) configurationError_ = result.code();
        return result;
    }

    void poll(uint32_t totalBudgetMicros) override {
        lifecycle_.poll(totalBudgetMicros);
    }

    void stop() override { lifecycle_.stop(); }

    ProductLifecycleStatus status() const override {
        ProductLifecycleStatus current = lifecycle_.status();
        if (configurationError_ != ErrorCode::Ok &&
            current.state == ProductLifecycleState::Stopped) {
            current.state = ProductLifecycleState::Fault;
            current.lastError = configurationError_;
        }
        return current;
    }

    ProductCapabilities capabilities() const override {
        return lifecycle_.capabilities();
    }

private:
    static ByteView asBytes(StringView value) {
        return ByteView(
            reinterpret_cast<const uint8_t*>(value.data), value.size);
    }

    Result initialize() {
        if (stack_.initialized()) return Result::success();
        if (configurationError_ != ErrorCode::Ok || !setupConfigured_) {
            return Result::failure(
                configurationError_ == ErrorCode::Ok
                    ? ErrorCode::NotConfigured
                    : configurationError_);
        }

        Result result = stack_.begin();
        if (result) {
            WifiNetworkConfig network;
            network.ssid = ByteView(setupSsid_, setupSsidSize_);
            network.authentication = setupCredentialSize_ == 0U
                                         ? WifiAuthentication::Open
                                         : WifiAuthentication::Wpa2Personal;
            network.credential = ByteView(
                setupCredential_, setupCredentialSize_);
            result = stack_.platform().wifiCredentials().commit(network);
        }
        clearNetworkSetup();
        if (!result) {
            stack_.end();
            configurationError_ = result.code();
        }
        return result;
    }

    Result remember(ErrorCode error) {
        configurationError_ = error;
        return Result::failure(error);
    }

    void clearNetworkSetup() {
        secureZero(MutableByteSpan(setupSsid_, sizeof(setupSsid_)));
        secureZero(MutableByteSpan(
            setupCredential_, sizeof(setupCredential_)));
        setupSsidSize_ = 0U;
        setupCredentialSize_ = 0U;
    }

    Platform platform_;
    ConfiguredDeviceKeySource deviceKey_;
    DeviceKeyWifiStack<Platform> stack_;
    WifiCloudLifecycle lifecycle_;
    uint8_t setupSsid_[kWifiSsidMaxSize];
    uint8_t setupCredential_[kWifiCredentialMaxSize];
    uint8_t setupSsidSize_;
    uint8_t setupCredentialSize_;
    ErrorCode configurationError_;
    bool setupConfigured_;

    DeviceKeyWifiComposition(const DeviceKeyWifiComposition&);
    DeviceKeyWifiComposition& operator=(
        const DeviceKeyWifiComposition&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
