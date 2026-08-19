#ifndef BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PLATFORM_H
#define BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PLATFORM_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalWiFiPlatform requires a Wio Terminal target"
#endif

#include "../../ports/wio_terminal/WioTerminalStorage.h"
#include "../../ports/wio_terminal/WioTerminalWiFi.h"

#include <BlinkerV2/arduino/config/OfficialConfig.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/provisioning/WifiCredentialStore.h>

#include <rpcWiFiClient.h>
#include <rpcWiFiClientSecure.h>

namespace blinker {
namespace integration {
namespace official_detail {

class WioTerminalWifiNetworkPlatform {
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

    WioTerminalWifiNetworkPlatform()
        : storage_(),
          wifiStorage_(
              storage_.wifiSelectorBlob(),
              storage_.wifiSlot0Blob(),
              storage_.wifiSlot1Blob(),
              storageConfig()),
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

private:
    static WifiCredentialStoreConfig storageConfig() {
        WifiCredentialStoreConfig config;
        config.allowPlainFlash = official::allowPlainStorage;
        return config;
    }

    WioTerminalFlashBlobBank storage_;
    WifiCredentialStore wifiStorage_;
    WioTerminalWifiStation station_;
    CloudSessionClient controlNetwork_;
    MqttClient mqttNetwork_;

    WioTerminalWifiNetworkPlatform(
        const WioTerminalWifiNetworkPlatform&);
    WioTerminalWifiNetworkPlatform& operator=(
        const WioTerminalWifiNetworkPlatform&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
