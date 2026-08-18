#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialEsp32ProvisionedDeviceKeyWifiComposition.h"
#include "BlinkerOfficialEsp32WifiNetworkPlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

class Esp32ProvisionedWifiNetworkPlatform
    : public Esp32WifiNetworkPlatform {
public:
    Esp32ProvisionedWifiNetworkPlatform()
        : Esp32WifiNetworkPlatform(),
          deviceKeyBlob_(esp32BlobStorageConfig(
              "bl_v2_key",
              "device",
              DeviceKeyStore::serializedSize)),
          deviceKeys_(deviceKeyBlob_) {}

    Result begin() {
        Result result = Esp32WifiNetworkPlatform::begin();
        if (result) result = deviceKeyBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        deviceKeyBlob_.end();
        Esp32WifiNetworkPlatform::end();
    }

    DeviceKeyStore& deviceKeys() { return deviceKeys_; }

private:
    Esp32NvsAtomicBlobStore deviceKeyBlob_;
    DeviceKeyStore deviceKeys_;
};

typedef NoProvisionedWifiExtension<Esp32ProvisionedWifiNetworkPlatform>
    WifiOnlyExtension;
typedef Esp32ProvisionedDeviceKeyWifiComposition<
    Esp32ProvisionedWifiNetworkPlatform,
    WifiOnlyExtension,
    Esp32BleWifiProvPolicy>
    WifiOnlyComposition;

WifiOnlyComposition& composition() {
    static WifiOnlyComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& esp32ProvisionedDeviceKeyWifiLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    bool forceProvisioning) {
    WifiOnlyComposition& selected = composition();
    (void)selected.configure(
        serviceName,
        proofOfPossession,
        StringView(),
        forceProvisioning);
    return selected;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
