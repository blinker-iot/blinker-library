#ifndef BLINKER_OFFICIAL_DEVICE_KEY_WIFI_STACK_H
#define BLINKER_OFFICIAL_DEVICE_KEY_WIFI_STACK_H

#include "BlinkerOfficialArduinoRuntime.h"

#include <BlinkerV2Advanced.h>
#include <BlinkerArduinoClientHttpAdapter.h>
#include <BlinkerHardwareRandomAdapter.h>
#include <BlinkerOfficialBuildConfig.h>
#include <BlinkerPubSubClientAdapter.h>

#include <PubSubClient.h>

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

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
