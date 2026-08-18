#ifndef BLINKER_OFFICIAL_RENESAS_UNO_WIFI_NETWORK_PLATFORM_H
#define BLINKER_OFFICIAL_RENESAS_UNO_WIFI_NETWORK_PLATFORM_H

#include <BlinkerOfficialBuildConfig.h>
#include <BlinkerRenesasUnoStorageAdapter.h>
#include <BlinkerRenesasUnoWifiAdapter.h>

#include <WiFiS3.h>

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "This official platform requires an Arduino Renesas target"
#endif

#if BLINKER_RESOURCE_PROFILE != BLINKER_RESOURCE_PROFILE_SMALL
#error "This official platform is sized for the Small resource profile"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

inline WifiCredentialStoreConfig renesasUnoWifiStorageConfig() {
    WifiCredentialStoreConfig config;
    config.allowPlainFlash = official::allowPlainStorage;
    return config;
}

class RenesasUnoWifiNetworkPlatform {
public:
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS
    typedef WiFiSSLClient CloudSessionClient;
#else
    typedef WiFiClient CloudSessionClient;
#endif
#if BLINKER_OFFICIAL_MQTT_TLS
    typedef WiFiSSLClient MqttClient;
#else
    typedef WiFiClient MqttClient;
#endif

    RenesasUnoWifiNetworkPlatform()
        : storage_(),
          wifiStorage_(
              storage_.wifiSelectorBlob(),
              storage_.wifiSlot0Blob(),
              storage_.wifiSlot1Blob(),
              renesasUnoWifiStorageConfig()),
          station_(),
          controlNetwork_(),
          mqttNetwork_() {}

    Result begin() {
        Result result = storage_.begin();
        if (result) result = wifiStorage_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        wifiStorage_.end();
        storage_.end();
    }

    void configureSecurity() {
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS
        controlNetwork_.setCACert(official::cloudSessionCaPem);
#endif
#if BLINKER_OFFICIAL_MQTT_TLS
        mqttNetwork_.setCACert(official::mqttCaPem);
#endif
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return storage_.deviceInstanceBlob();
    }
    WifiCredentialStore& wifiCredentials() { return wifiStorage_; }
    IWifiStation& wifiStation() { return station_; }
    CloudSessionClient& controlNetwork() { return controlNetwork_; }
    MqttClient& mqttNetwork() { return mqttNetwork_; }

protected:
    RenesasUnoPreferencesBlobBank& storage() { return storage_; }

private:
    RenesasUnoPreferencesBlobBank storage_;
    WifiCredentialStore wifiStorage_;
    RenesasUnoWifiStation station_;
    CloudSessionClient controlNetwork_;
    MqttClient mqttNetwork_;

    RenesasUnoWifiNetworkPlatform(
        const RenesasUnoWifiNetworkPlatform&);
    RenesasUnoWifiNetworkPlatform& operator=(
        const RenesasUnoWifiNetworkPlatform&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
