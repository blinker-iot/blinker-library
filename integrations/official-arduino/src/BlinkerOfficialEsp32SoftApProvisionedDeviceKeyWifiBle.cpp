#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialDirectBleExtension.h"
#include "BlinkerOfficialEsp32ProvisionedDeviceKeyWifiComposition.h"
#include "BlinkerOfficialEsp32WifiNetworkPlatform.h"

#include <BlinkerEsp32CryptoAdapter.h>
#include <BlinkerEsp32NimBleAdapter.h>

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

class Esp32SoftApProvisionedWifiBlePlatform
    : public Esp32WifiNetworkPlatform {
public:
    enum : size_t {
        maximumBlePacketSize = BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE
    };

    Esp32SoftApProvisionedWifiBlePlatform()
        : Esp32WifiNetworkPlatform(),
          accessBlob_(esp32BlobStorageConfig(
              "bl_v2_access",
              "root",
              DeviceAccessStore::serializedSize)),
          accessStore_(accessBlob_),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          ble_() {}

    Result begin() {
        Result result = Esp32WifiNetworkPlatform::begin();
        if (result) result = accessBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        ble_.stop();
        accessBlob_.end();
        Esp32WifiNetworkPlatform::end();
    }

    DeviceAccessStore& deviceKeys() { return accessStore_; }
    IDeviceAccessStore& deviceAccessStore() {
        return accessStore_;
    }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    Esp32NimBleLink& bleLink() { return ble_; }

private:
    Esp32NvsAtomicBlobStore accessBlob_;
    DeviceAccessStore accessStore_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier signatureVerifier_;
    Esp32NimBleLink ble_;
};

typedef DirectBleExtension<Esp32SoftApProvisionedWifiBlePlatform>
    DirectBle;
typedef Esp32ProvisionedDeviceKeyWifiComposition<
    Esp32SoftApProvisionedWifiBlePlatform,
    DirectBle,
    Esp32SoftApWifiProvPolicy>
    SoftApProvisionedWifiBleComposition;

SoftApProvisionedWifiBleComposition& composition() {
    static SoftApProvisionedWifiBleComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& esp32SoftApProvisionedDeviceKeyWifiBleLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    StringView serviceKey,
    bool forceProvisioning) {
    SoftApProvisionedWifiBleComposition& selected = composition();
    (void)selected.configure(
        serviceName,
        proofOfPossession,
        serviceKey,
        forceProvisioning);
    return selected;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
