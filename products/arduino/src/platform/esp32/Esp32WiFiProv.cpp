#include "WiFiProduct.h"
#include "Esp32WiFiProvProduct.h"
#include "Esp32WiFiPlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

class Esp32WifiProvPlatform
    : public Esp32WifiNetworkPlatform {
public:
    Esp32WifiProvPlatform()
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

typedef NoWifiExtension<Esp32WifiProvPlatform>
    WifiOnlyExtension;
typedef Esp32WifiProvProduct<
    Esp32WifiProvPlatform,
    WifiOnlyExtension,
    Esp32BleWifiProvPolicy>
    WifiProvProduct;

WifiProvProduct& product() {
    static WifiProvProduct instance;
    return instance;
}

} // namespace

IProductLifecycle& esp32WifiProvLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    bool forceProvisioning) {
    WifiProvProduct& selected = product();
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
