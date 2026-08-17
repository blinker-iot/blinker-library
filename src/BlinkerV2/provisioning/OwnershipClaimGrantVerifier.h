#ifndef BLINKER_PROVISIONING_OWNERSHIPCLAIMGRANTVERIFIER_H
#define BLINKER_PROVISIONING_OWNERSHIPCLAIMGRANTVERIFIER_H

#include "OwnershipClaimContract.h"

namespace blinker {

struct OwnershipClaimVerificationContext {
    ByteView requestId;
    ByteView deviceInstanceId;
    uint32_t ownershipGeneration;
    ByteView claimNonce;
    uint64_t nowEpochSeconds;
    bool hasTrustedTime;

    OwnershipClaimVerificationContext()
        : ownershipGeneration(0U),
          nowEpochSeconds(0U),
          hasTrustedTime(false) {}
};

struct OwnershipClaimCommitAckVerificationContext {
    ByteView requestId;
    ByteView deviceInstanceId;
    StringView logicalDeviceId;
    uint32_t ownershipGeneration;
    ByteView receiptDigest;

    OwnershipClaimCommitAckVerificationContext()
        : ownershipGeneration(0U) {}
};

class OwnershipClaimGrantVerifier {
public:
    explicit OwnershipClaimGrantVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    Result verify(
        ByteView encodedGrant,
        const OwnershipClaimVerificationContext& context,
        MutableByteSpan transcriptWorkspace,
        OwnershipClaimGrant& grant);
    Result verifyCommitAck(
        ByteView encodedAck,
        const OwnershipClaimCommitAckVerificationContext& context,
        MutableByteSpan transcriptWorkspace,
        OwnershipClaimCommitAck& ack);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
