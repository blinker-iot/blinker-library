#ifndef BLINKER_V2_ARDUINO_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_WIFI_PRODUCT_H

#include "ArduinoRuntime.h"

#include "../ports/arduino/ArduinoClientHttp.h"
#include "../ports/arduino/HardwareRandom.h"
#include "../ports/arduino/PubSubMqtt.h"

#include <BlinkerV2/api/Client.h>
#include <BlinkerV2/control/HttpDeviceKeySessionProvider.h>
#include <BlinkerV2/core/ResourceProfile.h>
#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/identity/ConfiguredDeviceKeySource.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/interface/IProductLifecycle.h>
#include <BlinkerV2/provisioning/WifiCredential.h>
#include <BlinkerV2/runtime/DeviceKeySessionArena.h>
#include <BlinkerV2/runtime/WifiCloudLifecycle.h>
#include <BlinkerV2/runtime/WifiConnectionLifecycle.h>
#include <BlinkerV2/transport/ManagedMqttTransport.h>
#include <BlinkerV2/transport/MqttFrameTransport.h>

#include <BlinkerV2/arduino/config/OfficialConfig.h>

#include <PubSubClient.h>
#include <string.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline HttpDeviceKeySessionConfig deviceKeySessionConfig() {
    HttpDeviceKeySessionConfig config;
    config.host = StringView(official::controlHost);
    config.port = official::controlPort;
    config.firmwareVersion = StringView(official::firmwareVersion);
    config.httpSecurity = official::cloudSessionTls
                              ? HttpSecurity::Tls
                              : HttpSecurity::PlainTcp;
    config.mqttSecurity = official::mqttTls
                              ? MqttSecurity::Tls
                              : MqttSecurity::PlainTcp;
    return config;
}

inline MqttFrameTransportConfig deviceKeyMqttTransportConfig() {
    MqttFrameTransportConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

// Final lower half of the manual/education WiFi graph. Platform supplies only
// storage, WiFi station and two Client-compatible network objects. DeviceKey
// authentication is refreshable and does not persist MQTT credentials.
template <typename Platform>
class DeviceKeyWifiStack {
public:
    typedef typename Platform::CloudSessionClient CloudSessionClient;
    typedef typename Platform::MqttClient MqttClient;
    typedef ArduinoClientHttpAdapter<CloudSessionClient, 97U, 384U, 128U>
        ControlHttp;

    DeviceKeyWifiStack(
        Platform& platform,
        IDeviceKeySource& deviceKey)
        : platform_(platform),
          deviceKey_(deviceKey),
          clock_(),
          random_(),
          deviceInstance_(),
          deviceInstanceStore_(platform_.deviceInstanceBlob()),
          wifiProfile_(),
          wifiLifecycle_(
              platform_.wifiCredentials(),
              platform_.wifiStation(),
              clock_,
              wifiProfile_),
          arena_(),
          controlHttp_(
              platform_.controlNetwork(),
              official::cloudSessionTls
                  ? HttpSecurity::Tls
                  : HttpSecurity::PlainTcp,
              clock_),
          cloudSession_(
              deviceKey_,
              deviceInstance_,
              random_,
              controlHttp_,
              clock_,
              deviceKeySessionConfig(),
              arena_.requestBuffer(),
              arena_.responseBuffer(),
              arena_.credentialArena()),
          nativeMqtt_(platform_.mqttNetwork()),
          mqttAdapter_(
              nativeMqtt_,
              official::mqttTls
                  ? MqttSecurity::Tls
                  : MqttSecurity::PlainTcp,
              kMqttPacketBufferSize,
              1U),
          mqttTransport_(
              mqttAdapter_,
              clock_,
              deviceKeyMqttTransportConfig()),
          cloudTransport_(cloudSession_, mqttTransport_),
          initialized_(false) {}

    ~DeviceKeyWifiStack() { end(); }

    Result open() {
        if (initialized_) return Result::success();
        platform_.configureSecurity();
        Result result = platform_.begin();
        if (result) {
            result = deviceInstanceStore_.loadOrCreate(
                random_, deviceInstance_);
        }
        if (!result) {
            platform_.end();
            return result;
        }
        initialized_ = true;
        return Result::success();
    }

    Result begin() {
        Result result = open();
        if (!result) return result;

        DeviceKey key;
        result = deviceKey_.load(key);
        clearDeviceKey(key);
        if (!result) end();
        return result;
    }

    void end() {
        if (!initialized_) return;
        platform_.end();
        initialized_ = false;
    }

    bool initialized() const { return initialized_; }
    Platform& platform() { return platform_; }
    const DeviceInstanceId& deviceInstance() const {
        return deviceInstance_;
    }
    ArduinoClock& clock() { return clock_; }
    PlatformHardwareRandom& random() { return random_; }
    WifiConnectionLifecycle& wifiLifecycle() { return wifiLifecycle_; }
    ManagedMqttTransport& cloudTransport() { return cloudTransport_; }

private:
    enum : uint16_t {
        kMqttPacketBufferSize =
            BLINKER_DEVICE_FRAME_SIZE +
            kDeviceKeyMaximumTopicSize + 9U
    };

    Platform& platform_;
    IDeviceKeySource& deviceKey_;
    ArduinoClock clock_;
    PlatformHardwareRandom random_;
    DeviceInstanceId deviceInstance_;
    DeviceInstanceIdStore deviceInstanceStore_;
    WifiCredentialProfile wifiProfile_;
    WifiConnectionLifecycle wifiLifecycle_;
    DeviceKeySessionArena arena_;
    ControlHttp controlHttp_;
    HttpDeviceKeySessionProvider cloudSession_;
    PubSubClient nativeMqtt_;
    PubSubClientAdapter<0U, 96U, 64U, 64U, 96U, 96U> mqttAdapter_;
    MqttFrameTransport mqttTransport_;
    ManagedMqttTransport cloudTransport_;
    bool initialized_;

    DeviceKeyWifiStack(const DeviceKeyWifiStack&);
    DeviceKeyWifiStack& operator=(const DeviceKeyWifiStack&);
};

// Canonical manual/education WiFi product. It is kept beside its stack so a
// platform wrapper includes one product graph instead of assembling several
// implementation headers.
template <typename Platform>
class DeviceKeyWifiProduct final : public IProductLifecycle {
public:
    DeviceKeyWifiProduct()
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

    ~DeviceKeyWifiProduct() override {
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
        if (result) result = client.setMonotonicClock(&stack_.clock());
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

    DeviceKeyWifiProduct(const DeviceKeyWifiProduct&);
    DeviceKeyWifiProduct& operator=(const DeviceKeyWifiProduct&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
