#ifndef BLINKER_V2_ARDUINO_ESP32_BLE_PLATFORM_H
#define BLINKER_V2_ARDUINO_ESP32_BLE_PLATFORM_H

#include "Esp32Storage.h"

#include "../../ports/esp32/Esp32Crypto.h"
#include "../../ports/esp32/Esp32NimBle.h"

#include <BlinkerV2/security/P256ServerKeyRingVerifier.h>
#include <BlinkerV2/arduino/config/Esp32ServerKeys.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/provisioning/BleEnrollmentContract.h>

#if !defined(ARDUINO_ARCH_ESP32)
#error "This official platform requires an ESP32 Arduino target"
#endif

#if BLINKER_RESOURCE_PROFILE != BLINKER_RESOURCE_PROFILE_SMALL
#error "This official platform is sized for the Small resource profile"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

class Esp32NimBleRadio {
public:
    enum : size_t {
        maximumPacketSize = BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE
    };

    Esp32NimBleLink& link() { return link_; }
    void stop() { link_.stop(); }

private:
    Esp32NimBleLink link_;
};

#if BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE
class Esp32PlatformBlePlatform {
public:
    enum : size_t {
        maximumBlePacketSize = Esp32NimBleRadio::maximumPacketSize
    };

    Esp32PlatformBlePlatform()
        : deviceInstanceBlob_(esp32BlobStorageConfig(
              "bl_v2_id",
              "instance",
              DeviceInstanceIdStore::serializedSize)),
          accessBlob_(esp32BlobStorageConfig(
              "bl_v2_access",
              "root",
              DeviceAccessStore::serializedSize)),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          radio_() {}

    Result begin() {
        Result result = deviceInstanceBlob_.begin();
        if (result) result = accessBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        radio_.stop();
        accessBlob_.end();
        deviceInstanceBlob_.end();
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return deviceInstanceBlob_;
    }
    IAtomicBlobStore& deviceAccessBlob() { return accessBlob_; }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    BleEnrollmentApplicationConfig bleEnrollmentConfig() const {
        BleEnrollmentApplicationConfig config;
        config.serverKeyId = official::serverSigningKeys[0].keyId;
        config.signatureAlgorithm =
            ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
        return config;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    Esp32NimBleLink& bleLink() { return radio_.link(); }

private:
    Esp32NvsAtomicBlobStore deviceInstanceBlob_;
    Esp32NvsAtomicBlobStore accessBlob_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier signatureVerifier_;
    Esp32NimBleRadio radio_;
};
#endif

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
