#ifndef BLINKER_V2_ARDUINO_ESP32_WIFI_PLATFORM_H
#define BLINKER_V2_ARDUINO_ESP32_WIFI_PLATFORM_H

#include "Esp32Storage.h"

#include "../../ports/esp32/Esp32WiFi.h"

#include <BlinkerV2/arduino/config/OfficialConfig.h>

#include <WiFiClient.h>
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS || BLINKER_OFFICIAL_MQTT_TLS
#include <WiFiClientSecure.h>
#endif

#if !defined(ARDUINO_ARCH_ESP32)
#error "This official platform requires an ESP32 Arduino target"
#endif

#if BLINKER_RESOURCE_PROFILE != BLINKER_RESOURCE_PROFILE_SMALL
#error "This official platform is sized for the Small resource profile"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

inline Esp32NvsWifiCredentialSinkConfig esp32WifiStorageConfig() {
    Esp32NvsWifiCredentialSinkConfig config;
    config.namespaceName = "bl_v2_net";
    config.allowPlainFlash = official::allowPlainStorage;
    return config;
}

// Minimal network/storage boundary for DeviceKey WiFi compositions. Identity
// policy, ownership and cloud enrollment do not belong
// here and therefore cannot pull crypto dependencies into the DeviceKey path.
class Esp32WifiNetworkPlatform {
public:
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS
    typedef WiFiClientSecure CloudSessionClient;
#else
    typedef WiFiClient CloudSessionClient;
#endif
#if BLINKER_OFFICIAL_MQTT_TLS
    typedef WiFiClientSecure MqttClient;
#else
    typedef WiFiClient MqttClient;
#endif

    Esp32WifiNetworkPlatform()
        : deviceInstanceBlob_(esp32BlobStorageConfig(
              "bl_v2_id",
              "instance",
              DeviceInstanceIdStore::serializedSize)),
          wifiStorage_(esp32WifiStorageConfig()),
          station_(),
          controlNetwork_(),
          mqttNetwork_() {}

    Result begin() {
        Result result = deviceInstanceBlob_.begin();
        if (result) result = wifiStorage_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        wifiStorage_.end();
        deviceInstanceBlob_.end();
    }

    void configureSecurity() {
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS
        controlNetwork_.setCACert(official::cloudSessionCaPem);
#endif
#if BLINKER_OFFICIAL_MQTT_TLS
        mqttNetwork_.setCACert(official::mqttCaPem);
#endif
    }

    IAtomicBlobStore& deviceInstanceBlob() { return deviceInstanceBlob_; }
    WifiCredentialStore& wifiCredentials() {
        return wifiStorage_.credentialStore();
    }
    IWifiStation& wifiStation() { return station_; }
    CloudSessionClient& controlNetwork() { return controlNetwork_; }
    MqttClient& mqttNetwork() { return mqttNetwork_; }

private:
    Esp32NvsAtomicBlobStore deviceInstanceBlob_;
    Esp32NvsWifiCredentialSink wifiStorage_;
    Esp32WifiStation station_;
    CloudSessionClient controlNetwork_;
    MqttClient mqttNetwork_;

    Esp32WifiNetworkPlatform(const Esp32WifiNetworkPlatform&);
    Esp32WifiNetworkPlatform& operator=(
        const Esp32WifiNetworkPlatform&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
