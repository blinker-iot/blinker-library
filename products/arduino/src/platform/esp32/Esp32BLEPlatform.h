#ifndef BLINKER_V2_ARDUINO_ESP32_BLE_PLATFORM_H
#define BLINKER_V2_ARDUINO_ESP32_BLE_PLATFORM_H

#include "Esp32Storage.h"

#include <BlinkerEsp32CryptoAdapter.h>
#include <BlinkerEsp32NimBleAdapter.h>
#include <BlinkerOfficialBuildConfig.h>
#include <BlinkerV2/identity/ControllerCredentialStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/identity/OwnershipRecordStore.h>
#include <BlinkerV2/provisioning/LocalSetupSagaStore.h>
#include <BlinkerV2/provisioning/OwnershipClaimRecordStore.h>

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

class Esp32LocalBlePlatform {
public:
    enum : size_t {
        maximumBlePacketSize = Esp32NimBleRadio::maximumPacketSize
    };

    Esp32LocalBlePlatform()
        : deviceInstanceBlob_(esp32BlobStorageConfig(
              "bl_v2_id",
              "instance",
              DeviceInstanceIdStore::serializedSize)),
          controllerBlob_(esp32BlobStorageConfig(
              "bl_v2_ctl",
              "table",
              ControllerCredentialStore::serializedSize)),
          crypto_(),
          radio_() {}

    Result begin() {
        Result result = deviceInstanceBlob_.begin();
        if (result) result = controllerBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        radio_.stop();
        controllerBlob_.end();
        deviceInstanceBlob_.end();
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return deviceInstanceBlob_;
    }
    IAtomicBlobStore& controllerCredentialBlob() {
        return controllerBlob_;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    Esp32NimBleLink& bleLink() { return radio_.link(); }

private:
    Esp32NvsAtomicBlobStore deviceInstanceBlob_;
    Esp32NvsAtomicBlobStore controllerBlob_;
    Esp32MbedTlsCryptoProvider crypto_;
    Esp32NimBleRadio radio_;
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
          ownershipBlob_(esp32BlobStorageConfig(
              "bl_v2_owner",
              "binding",
              OwnershipRecordStore::serializedSize)),
          ownershipStore_(ownershipBlob_),
          claimBlob_(esp32BlobStorageConfig(
              "bl_v2_claim",
              "journal",
              OwnershipClaimRecordStore::serializedSize)),
          controllerBlob_(esp32BlobStorageConfig(
              "bl_v2_ctl",
              "table",
              ControllerCredentialStore::serializedSize)),
          setupSagaBlob_(esp32BlobStorageConfig(
              "bl_v2_setup",
              "saga",
              LocalSetupSagaStore::serializedSize)),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          radio_() {}

    Result begin() {
        Result result = deviceInstanceBlob_.begin();
        if (result) result = ownershipBlob_.begin();
        if (result) result = claimBlob_.begin();
        if (result) result = controllerBlob_.begin();
        if (result) result = setupSagaBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        radio_.stop();
        setupSagaBlob_.end();
        controllerBlob_.end();
        claimBlob_.end();
        ownershipBlob_.end();
        deviceInstanceBlob_.end();
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return deviceInstanceBlob_;
    }
    OwnershipRecordStore& ownershipStore() {
        return ownershipStore_;
    }
    IAtomicBlobStore& ownershipClaimBlob() { return claimBlob_; }
    IAtomicBlobStore& controllerCredentialBlob() {
        return controllerBlob_;
    }
    IAtomicBlobStore& localSetupSagaBlob() {
        return setupSagaBlob_;
    }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    Esp32NimBleLink& bleLink() { return radio_.link(); }

private:
    Esp32NvsAtomicBlobStore deviceInstanceBlob_;
    Esp32NvsAtomicBlobStore ownershipBlob_;
    OwnershipRecordStore ownershipStore_;
    Esp32NvsAtomicBlobStore claimBlob_;
    Esp32NvsAtomicBlobStore controllerBlob_;
    Esp32NvsAtomicBlobStore setupSagaBlob_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier signatureVerifier_;
    Esp32NimBleRadio radio_;
};
#endif

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
