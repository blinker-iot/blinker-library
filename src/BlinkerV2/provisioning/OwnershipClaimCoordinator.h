#ifndef BLINKER_PROVISIONING_OWNERSHIPCLAIMCOORDINATOR_H
#define BLINKER_PROVISIONING_OWNERSHIPCLAIMCOORDINATOR_H

#include "OwnershipClaimGrantVerifier.h"
#include "../interface/IOwnershipClaimRecordStore.h"
#include "../interface/IOwnershipRecordStore.h"
#include "../interface/IRandom.h"

namespace blinker {

// Transport-independent Unclaimed -> Active transaction. The small request
// journal survives power loss until the server acknowledges the receipt.
class OwnershipClaimCoordinator {
public:
    OwnershipClaimCoordinator(
        const DeviceInstanceId& deviceInstanceId,
        IOwnershipRecordStore& ownership,
        IOwnershipClaimRecordStore& journal,
        IRandom& random,
        OwnershipClaimGrantVerifier& verifier);
    OwnershipClaimCoordinator(const OwnershipClaimCoordinator&) = delete;
    OwnershipClaimCoordinator& operator=(
        const OwnershipClaimCoordinator&) = delete;

    Result beginClaim(
        MutableByteSpan output,
        ByteView& request);

    Result applyGrant(
        ByteView encodedGrant,
        uint64_t nowEpochSeconds,
        bool hasTrustedTime,
        MutableByteSpan operationWorkspace,
        MutableByteSpan output,
        ByteView& receipt);
    Result completeClaim(
        ByteView encodedCommitAck,
        MutableByteSpan operationWorkspace);
    Result verifyCommitAck(
        ByteView encodedCommitAck,
        MutableByteSpan operationWorkspace);
    Result finishVerifiedClaim();
    Result cancelClaim();

private:
    Result encodeCurrentRequest(
        const OwnershipClaimRecord& record,
        MutableByteSpan output,
        ByteView& request) const;
    Result encodeReceipt(
        const OwnershipClaimGrant& grant,
        ByteView grantDigest,
        MutableByteSpan output,
        ByteView& receipt) const;

    const DeviceInstanceId& deviceInstanceId_;
    IOwnershipRecordStore& ownership_;
    IOwnershipClaimRecordStore& journal_;
    IRandom& random_;
    OwnershipClaimGrantVerifier& verifier_;
};

} // namespace blinker

#endif
