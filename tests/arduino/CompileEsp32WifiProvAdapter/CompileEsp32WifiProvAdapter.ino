#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32Storage.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32WiFiProv.h>

blinker::Esp32NvsBlobStoreConfig makeDeviceKeyConfig() {
    blinker::Esp32NvsBlobStoreConfig config;
    config.namespaceName = "bl_v2_key";
    config.key = "device";
    config.maximumSize = blinker::DeviceAccessStore::serializedSize;
    return config;
}

blinker::Esp32NvsWifiCredentialSinkConfig makeWifiConfig() {
    blinker::Esp32NvsWifiCredentialSinkConfig config;
    config.allowPlainFlash = true;
    return config;
}

blinker::Esp32NvsBlobStoreConfig deviceKeyConfig = makeDeviceKeyConfig();
blinker::Esp32NvsAtomicBlobStore deviceKeyBlob(deviceKeyConfig);
blinker::DeviceAccessStore deviceKeys(deviceKeyBlob);
blinker::Esp32NvsWifiCredentialSinkConfig wifiConfig = makeWifiConfig();
blinker::Esp32NvsWifiCredentialSink wifiCredentials(wifiConfig);
blinker::DeviceInstanceId deviceInstance;
blinker::DeviceKeyProvisioningEndpoint deviceKeyEndpoint(
    deviceInstance,
    deviceKeys);
blinker::Esp32WifiProvAdapter wifiProv(
    deviceKeyEndpoint,
    wifiCredentials);

volatile bool blinkerRunWifiProvProbe = false;

void setup() {
    for (size_t index = 0U; index < sizeof(deviceInstance.bytes); ++index) {
        deviceInstance.bytes[index] = static_cast<uint8_t>(index + 1U);
    }
    (void)deviceKeyBlob.begin();
    (void)wifiCredentials.begin();

    if (blinkerRunWifiProvProbe) {
        blinker::Esp32WifiProvConfig config;
        config.serviceName = "BLINKER_TEST";
        config.proofOfPossession = "12345678";
        (void)wifiProv.begin(config);
        wifiProv.poll();
        (void)wifiProv.state();
        (void)wifiProv.lastError();
        (void)wifiProv.deviceKeyInstalled();
        (void)wifiProv.wifiCredentialsCommitted();
        wifiProv.end();
    }
}

void loop() {}
