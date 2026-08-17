#ifndef BLINKER_PROVISIONING_CLOUDENROLLMENTRECORD_H
#define BLINKER_PROVISIONING_CLOUDENROLLMENTRECORD_H

#include "../core/Sha256.h"
#include "../identity/CloudCredential.h"

namespace blinker {

enum class CloudEnrollmentRecordState : uint8_t {
    Prepared = 1U,
    GrantAccepted = 2U
};

enum : size_t {
    kCloudEnrollmentRequestIdSize = 16U,
    kCloudEnrollmentEphemeralPrivateKeySize = 32U,
    kCloudEnrollmentGrantDigestSize = kSha256Size
};

// Durable transaction-only secret state. It is separate from the Active
// CloudCredential so a consumed authorization and a lost response can resume
// the same exact enrollment after power loss.
struct CloudEnrollmentRecord {
    CloudEnrollmentRecordState state;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    uint32_t encryptionKeyId;
    uint8_t requestId[kCloudEnrollmentRequestIdSize];
    uint8_t ephemeralPrivateKey[kCloudEnrollmentEphemeralPrivateKeySize];
    uint8_t cloudSecret[kCloudCredentialSecretSize];
    uint8_t grantDigest[kCloudEnrollmentGrantDigestSize];

    CloudEnrollmentRecord();
};

Result validateCloudEnrollmentRecord(const CloudEnrollmentRecord& record);
bool sameCloudEnrollmentRecord(
    const CloudEnrollmentRecord& first,
    const CloudEnrollmentRecord& second);
void clearCloudEnrollmentRecord(CloudEnrollmentRecord& record);

} // namespace blinker

#endif
