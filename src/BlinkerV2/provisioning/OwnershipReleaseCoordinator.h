#ifndef BLINKER_PROVISIONING_OWNERSHIPRELEASECOORDINATOR_H
#define BLINKER_PROVISIONING_OWNERSHIPRELEASECOORDINATOR_H

#include "../interface/IOwnershipRecordStore.h"
#include "../interface/IOwnershipReleaseRecordStore.h"
#include "../interface/IRandom.h"
#include "OwnershipReleaseCleanup.h"
#include "OwnershipReleaseVerifier.h"

namespace blinker {

enum class OwnershipReleaseState : uint8_t {
    Unknown = 0U,
    NeedsOwnership,
    Ready,
    Prepared,
    AwaitingCommitAck,
    Finalizing,
    Released,
    Fault
};

// Transport-independent Reset/Transfer transaction. User authorization,
// EnrollmentTicket/JWT and HTTP/BLE relay remain outside this state machine.
class OwnershipReleaseCoordinator {
public:
    OwnershipReleaseCoordinator(
        const DeviceInstanceId& deviceInstanceId,
        IOwnershipRecordStore& ownership,
        IOwnershipReleaseRecordStore& journal,
        IRandom& random,
        OwnershipReleaseVerifier& verifier,
        OwnershipReleaseCleanup& cleanup);
    OwnershipReleaseCoordinator(const OwnershipReleaseCoordinator&) = delete;
    OwnershipReleaseCoordinator& operator=(
        const OwnershipReleaseCoordinator&) = delete;

    Result load();
    Result prepare(
        OwnershipReleaseOperation operation,
        OwnershipReleaseNetworkPolicy networkPolicy);
    Result request(MutableByteSpan output, ByteView& encodedRequest);
    Result applyGrant(
        ByteView encodedGrant,
        uint64_t nowEpochSeconds,
        bool hasTrustedTime,
        MutableByteSpan workspace,
        MutableByteSpan receiptOutput,
        ByteView& encodedReceipt);
    Result receipt(
        MutableByteSpan output,
        ByteView& encodedReceipt);
    Result acceptCommitAck(
        ByteView encodedAck,
        MutableByteSpan workspace);
    Result restartPrepared();

    OwnershipReleaseState state() const { return state_; }

private:
    Result fillNonZero(MutableByteSpan output);
    Result validatePreparedOwnership(
        const OwnershipReleaseRecord& record) const;
    Result ensureRetired(const OwnershipReleaseRecord& record);
    Result encodeRequest(
        const OwnershipReleaseRecord& record,
        MutableByteSpan output,
        ByteView& encodedRequest) const;
    Result encodeReceipt(
        const OwnershipReleaseRecord& record,
        MutableByteSpan output,
        ByteView& encodedReceipt) const;
    Result finalizeAcknowledged(OwnershipReleaseRecord& record);

    const DeviceInstanceId& deviceInstanceId_;
    IOwnershipRecordStore& ownership_;
    IOwnershipReleaseRecordStore& journal_;
    IRandom& random_;
    OwnershipReleaseVerifier& verifier_;
    OwnershipReleaseCleanup& cleanup_;
    OwnershipReleaseState state_;
};

} // namespace blinker

#endif
