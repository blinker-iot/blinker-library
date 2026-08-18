#ifndef BLINKER_OFFICIAL_ESP32_WIFI_PLATFORM_H
#define BLINKER_OFFICIAL_ESP32_WIFI_PLATFORM_H

#include "BlinkerOfficialEsp32WifiNetworkPlatform.h"

#include <BlinkerEsp32CryptoAdapter.h>

namespace blinker {
namespace integration {
namespace official_detail {

// ESP32 platform half shared by WiFi-only and WiFi+BLE official candidates.
// BLE-specific storage/link members stay in a derived platform so WiFi-only
// firmware does not open unused NVS handles or reserve radio queues.
class Esp32WifiPlatform : public Esp32WifiNetworkPlatform {
public:
    Esp32WifiPlatform()
        : Esp32WifiNetworkPlatform(),
          ownershipBlob_(esp32BlobStorageConfig(
              "bl_v2_owner",
              "binding",
              OwnershipRecordStore::serializedSize)),
          ownershipStore_(ownershipBlob_),
          claimBlob_(esp32BlobStorageConfig(
              "bl_v2_claim",
              "journal",
              OwnershipClaimRecordStore::serializedSize)),
          cloudBlob_(esp32BlobStorageConfig(
              "bl_v2_cloud",
              "credential",
              CloudCredentialStore::serializedSize)),
          cloudStore_(cloudBlob_),
          enrollmentBlob_(esp32BlobStorageConfig(
              "bl_v2_enroll",
              "journal",
              CloudEnrollmentRecordStore::serializedSize)),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount) {}

    Result begin() {
        Result result = Esp32WifiNetworkPlatform::begin();
        if (result) result = ownershipBlob_.begin();
        if (result) result = claimBlob_.begin();
        if (result) result = cloudBlob_.begin();
        if (result) result = enrollmentBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        enrollmentBlob_.end();
        cloudBlob_.end();
        claimBlob_.end();
        ownershipBlob_.end();
        Esp32WifiNetworkPlatform::end();
    }

    ICloudCredentialSource& cloudCredentials() { return cloudStore_; }
    IOwnershipSource& ownership() { return ownershipStore_; }
    OwnershipRecordStore& ownershipStore() { return ownershipStore_; }
    CloudCredentialStore& cloudCredentialStore() { return cloudStore_; }
    IAtomicBlobStore& ownershipClaimBlob() { return claimBlob_; }
    IAtomicBlobStore& cloudEnrollmentBlob() { return enrollmentBlob_; }
    IX25519AesGcmCryptoProvider& enrollmentCrypto() { return crypto_; }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
private:
    Esp32NvsAtomicBlobStore ownershipBlob_;
    OwnershipRecordStore ownershipStore_;
    Esp32NvsAtomicBlobStore claimBlob_;
    Esp32NvsAtomicBlobStore cloudBlob_;
    CloudCredentialStore cloudStore_;
    Esp32NvsAtomicBlobStore enrollmentBlob_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier signatureVerifier_;

    Esp32WifiPlatform(const Esp32WifiPlatform&);
    Esp32WifiPlatform& operator=(const Esp32WifiPlatform&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
