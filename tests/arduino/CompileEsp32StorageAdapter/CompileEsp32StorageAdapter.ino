#include <BlinkerV2Advanced.h>
#include <Esp32Storage.h>

blinker::Esp32NvsBlobStoreConfig makeAccessConfig() {
    blinker::Esp32NvsBlobStoreConfig config;
    config.namespaceName = "bl_v2_access";
    config.key = "root";
    config.maximumSize = blinker::DeviceAccessStore::serializedSize;
    return config;
}

blinker::Esp32NvsAtomicBlobStore accessBlob(makeAccessConfig());
blinker::DeviceAccessStore accessStore(accessBlob);

blinker::Esp32NvsWifiCredentialSinkConfig makeWifiConfig() {
    blinker::Esp32NvsWifiCredentialSinkConfig config;
    config.allowPlainFlash = true;
    return config;
}

blinker::Esp32NvsWifiCredentialSink wifi(makeWifiConfig());

void setup() {
    (void)accessBlob.begin();
    (void)accessStore.protection();
    (void)wifi.begin();
}

void loop() {}
