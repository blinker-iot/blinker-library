#ifndef BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTGRANTVERIFIER_H
#define BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTGRANTVERIFIER_H

#include "CloudCredentialEnrollmentContract.h"

namespace blinker {

struct CloudCredentialEnrollmentVerificationContext {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    ByteView requestDigest;
    uint64_t nowEpochSeconds;
    bool hasTrustedTime;

    CloudCredentialEnrollmentVerificationContext()
        : ownershipGeneration(0U),
          credentialVersion(0U),
          nowEpochSeconds(0U),
          hasTrustedTime(false) {}
};

class CloudCredentialEnrollmentGrantVerifier {
public:
    explicit CloudCredentialEnrollmentGrantVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    Result verify(
        ByteView encodedGrant,
        const CloudCredentialEnrollmentVerificationContext& context,
        MutableByteSpan transcriptWorkspace,
        CloudCredentialEnrollmentGrant& grant);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
