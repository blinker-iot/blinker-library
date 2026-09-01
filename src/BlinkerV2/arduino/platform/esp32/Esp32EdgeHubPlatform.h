#ifndef BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PLATFORM_H
#define BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PLATFORM_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32EdgeHubPlatform requires an ESP32 Arduino target"
#endif

#include "../../config/EdgeHubConfig.h"
#include "../../config/OfficialConfig.h"
#include "../../ports/esp32/Esp32Security.h"
#include "../../ports/esp32/Esp32Storage.h"
#include "../../ports/esp32/Esp32WiFi.h"

#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/identity/DeviceKeyStore.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>

#include <WiFiClientSecure.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline Esp32NvsBlobStoreConfig edgeHubBlobConfig(
    const char* namespaceName,
    const char* key,
    size_t maximumSize,
    bool protectedStorage) {
    Esp32NvsBlobStoreConfig config;
    config.namespaceName = namespaceName;
    config.key = key;
    config.partitionLabel = official::edgeHubNvsPartition;
    config.maximumSize = maximumSize;
    config.encrypted = protectedStorage;
    config.encryptionVerifier = protectedStorage
                                    ? &esp32EncryptedNvsPartitionAvailable
                                    : nullptr;
    return config;
}

inline Esp32NvsWifiCredentialSinkConfig edgeHubWifiStorageConfig(
    bool protectedStorage) {
    Esp32NvsWifiCredentialSinkConfig config;
    config.namespaceName = "bl_eh_net";
    config.partitionLabel = official::edgeHubNvsPartition;
    config.encrypted = protectedStorage;
    config.allowPlainFlash = !protectedStorage;
    config.encryptionVerifier = protectedStorage
                                    ? &esp32EncryptedNvsPartitionAvailable
                                    : nullptr;
    return config;
}

// Narrow production platform. GatewayAccessBlob is a dependency-injection
// seam for internal tests; the production alias below always uses the real
// encrypted-NVS adapter.
template <
    bool ProtectedStorage,
    typename GatewayAccessBlob = Esp32NvsAtomicBlobStore>
class BasicEsp32EdgeHubPlatform {
public:
    typedef WiFiClientSecure CloudSessionClient;
    typedef WiFiClientSecure MqttClient;

    BasicEsp32EdgeHubPlatform()
        : deviceInstanceBlob_(edgeHubBlobConfig(
              "bl_eh_id", "instance",
              DeviceInstanceIdStore::serializedSize,
              ProtectedStorage)),
          deviceKeyBlob_(edgeHubBlobConfig(
              "bl_eh_key", "root",
              DeviceKeyStore::serializedSize,
              ProtectedStorage)),
          wifiStorage_(edgeHubWifiStorageConfig(ProtectedStorage)),
          gatewayAccessBlob_(edgeHubBlobConfig(
              "bl_eh_access", "child",
              GatewayAccessStore::serializedSize,
              ProtectedStorage)),
          station_(), controlNetwork_(), mqttNetwork_() {}

    Result begin() {
        Result result = deviceInstanceBlob_.begin();
        if (result) result = deviceKeyBlob_.begin();
        if (result) result = wifiStorage_.begin();
        if (result) result = gatewayAccessBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        gatewayAccessBlob_.end();
        wifiStorage_.end();
        deviceKeyBlob_.end();
        deviceInstanceBlob_.end();
    }

    void configureSecurity() {
        controlNetwork_.setCACert(official::cloudSessionCaPem);
        mqttNetwork_.setCACert(official::mqttCaPem);
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return deviceInstanceBlob_;
    }
    IAtomicBlobStore& deviceKeyBlob() { return deviceKeyBlob_; }
    IWifiCredentialSink& wifiCredentialSink() { return wifiStorage_; }
    WifiCredentialStore& wifiCredentials() {
        return wifiStorage_.credentialStore();
    }
    IAtomicBlobStore& gatewayAccessBlob() {
        return gatewayAccessBlob_;
    }
    IWifiStation& wifiStation() { return station_; }
    CloudSessionClient& controlNetwork() { return controlNetwork_; }
    MqttClient& mqttNetwork() { return mqttNetwork_; }

private:
    Esp32NvsAtomicBlobStore deviceInstanceBlob_;
    Esp32NvsAtomicBlobStore deviceKeyBlob_;
    Esp32NvsWifiCredentialSink wifiStorage_;
    GatewayAccessBlob gatewayAccessBlob_;
    Esp32WifiStation station_;
    CloudSessionClient controlNetwork_;
    MqttClient mqttNetwork_;

    BasicEsp32EdgeHubPlatform(const BasicEsp32EdgeHubPlatform&);
    BasicEsp32EdgeHubPlatform& operator=(
        const BasicEsp32EdgeHubPlatform&);
};

// The product alias is intentionally fail-closed. Plain storage is selected
// only by internal test compositions which name BasicEsp32EdgeHubPlatform<false>
// explicitly; it is never reachable from the public Arduino facade.
typedef BasicEsp32EdgeHubPlatform<true> Esp32EdgeHubPlatform;

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
