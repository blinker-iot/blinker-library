#ifndef BLINKER_PROVISIONING_CLOUDCREDENTIALENVELOPESEALER_H
#define BLINKER_PROVISIONING_CLOUDCREDENTIALENVELOPESEALER_H

#include "../interface/ICloudEnrollmentEncryptionKeySource.h"
#include "CloudCredentialEnrollmentContract.h"

namespace blinker {

// Deterministic for one persisted request/private-key/secret tuple. The
// server static key protects the long-term secret even when HTTP is Plain.
class CloudCredentialEnvelopeSealer {
public:
    CloudCredentialEnvelopeSealer(
        IX25519AesGcmCryptoProvider& crypto,
        ICloudEnrollmentEncryptionKeySource& keys)
        : crypto_(crypto), keys_(keys) {}

    Result activeKeyId(uint32_t& keyId) {
        return keys_.activeKeyId(keyId);
    }

    Result seal(
        CloudCredentialEnrollmentRequest& request,
        ByteView ephemeralPrivateKey,
        ByteView cloudSecret,
        MutableByteSpan transcriptWorkspace,
        MutableByteSpan ephemeralPublicKeyOutput,
        MutableByteSpan nonceOutput,
        MutableByteSpan encryptedCredentialOutput);

private:
    IX25519AesGcmCryptoProvider& crypto_;
    ICloudEnrollmentEncryptionKeySource& keys_;
};

} // namespace blinker

#endif
