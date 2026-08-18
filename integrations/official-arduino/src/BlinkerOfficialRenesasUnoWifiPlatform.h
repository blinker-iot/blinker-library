#ifndef BLINKER_OFFICIAL_RENESAS_UNO_WIFI_PLATFORM_H
#define BLINKER_OFFICIAL_RENESAS_UNO_WIFI_PLATFORM_H

#include <BlinkerArduinoCryptoAdapter.h>
#include "BlinkerOfficialRenesasUnoWifiNetworkPlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

// UNO R4 platform half shared by WiFi-only and WiFi+BLE. BLE ownership stays
// in a derived platform so the WiFi-only package has no ArduinoBLE dependency.
class RenesasUnoWifiPlatform : public RenesasUnoWifiNetworkPlatform {
public:
    RenesasUnoWifiPlatform()
        : RenesasUnoWifiNetworkPlatform(),
          ownershipStore_(storage().ownershipBlob()),
          cloudStore_(storage().cloudBlob()),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount) {}

    Result begin() {
        return RenesasUnoWifiNetworkPlatform::begin();
    }

    void end() {
        RenesasUnoWifiNetworkPlatform::end();
    }

    ICloudCredentialSource& cloudCredentials() { return cloudStore_; }
    IOwnershipSource& ownership() { return ownershipStore_; }
    OwnershipRecordStore& ownershipStore() { return ownershipStore_; }
    CloudCredentialStore& cloudCredentialStore() { return cloudStore_; }
    IAtomicBlobStore& ownershipClaimBlob() {
        return storage().ownershipClaimBlob();
    }
    IAtomicBlobStore& cloudEnrollmentBlob() {
        return storage().cloudEnrollmentBlob();
    }
    IAtomicBlobStore& controllerCredentialBlob() {
        return storage().controllerBlob();
    }
    IAtomicBlobStore& localSetupSagaBlob() {
        return storage().localSetupSagaBlob();
    }
    IX25519AesGcmCryptoProvider& enrollmentCrypto() { return crypto_; }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
private:
    OwnershipRecordStore ownershipStore_;
    CloudCredentialStore cloudStore_;
    ArduinoCryptoProvider crypto_;
    Ed25519ServerKeyRingVerifier signatureVerifier_;

    RenesasUnoWifiPlatform(const RenesasUnoWifiPlatform&);
    RenesasUnoWifiPlatform& operator=(const RenesasUnoWifiPlatform&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
