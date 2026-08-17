#ifndef BLINKER_PROVISIONING_ENROLLMENTTICKETVERIFIER_H
#define BLINKER_PROVISIONING_ENROLLMENTTICKETVERIFIER_H

#include "EnrollmentTicketContract.h"

namespace blinker {

// Values retained by the current provisional BLE setup session. Matching the
// transcript hash binds the signed ticket to both ephemeral keys without
// exposing platform BLE or Noise types to this reusable verifier.
struct EnrollmentTicketVerificationContext {
    ByteView deviceInstanceId;
    ByteView setupSessionId;
    ByteView setupTranscriptHash;
    uint32_t ownershipGeneration;
    ByteView controllerId;
    ByteView controllerSecretDigest;
    uint32_t controllerPermissions;
    uint32_t authorizedOperations;
    uint64_t nowEpochSeconds;
    bool hasTrustedTime;

    EnrollmentTicketVerificationContext();
};

class EnrollmentTicketVerifier {
public:
    explicit EnrollmentTicketVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    Result verify(
        ByteView encodedTicket,
        const EnrollmentTicketVerificationContext& context,
        MutableByteSpan transcriptWorkspace,
        EnrollmentTicket& ticket);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
