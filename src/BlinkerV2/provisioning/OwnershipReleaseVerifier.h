#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASEVERIFIER_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASEVERIFIER_H

#include "OwnershipReleaseContract.h"

namespace blinker {

class OwnershipReleaseVerifier {
public:
    explicit OwnershipReleaseVerifier(
        IServerSignatureVerifier& signatureVerifier)
        : signatureVerifier_(signatureVerifier) {}

    Result verifyGrant(
        ByteView encodedGrant,
        const OwnershipReleaseRequest& request,
        ByteView encodedRequest,
        uint64_t nowEpochSeconds,
        bool hasTrustedTime,
        MutableByteSpan workspace,
        OwnershipReleaseGrant& grant);
    Result verifyCommitAck(
        ByteView encodedAck,
        const OwnershipReleaseReceipt& receipt,
        ByteView encodedReceipt,
        MutableByteSpan workspace,
        OwnershipReleaseCommitAck& ack);

private:
    IServerSignatureVerifier& signatureVerifier_;
};

} // namespace blinker

#endif
