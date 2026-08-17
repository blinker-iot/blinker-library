#ifndef BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTCOORDINATOR_H
#define BLINKER_PROVISIONING_CLOUDCREDENTIALENROLLMENTCOORDINATOR_H

#include "../interface/ICloudCredentialStore.h"
#include "../interface/ICloudEnrollmentRecordStore.h"
#include "../interface/IOwnershipSource.h"
#include "../interface/IRandom.h"
#include "CloudCredentialEnvelopeSealer.h"
#include "CloudCredentialEnrollmentGrantVerifier.h"

namespace blinker {

enum class CloudCredentialEnrollmentState : uint8_t {
    Unknown = 0U,
    NeedsOwnership,
    Ready,
    Prepared,
    AwaitingCommitAck,
    Active,
    Fault
};

// Transport-independent cloud credential transaction. EnrollmentKey/Ticket
// authorization and HTTP/BLE relay live in profile drivers, not here.
class CloudCredentialEnrollmentCoordinator {
public:
    CloudCredentialEnrollmentCoordinator(
        const DeviceInstanceId& deviceInstanceId,
        IOwnershipSource& ownership,
        ICloudCredentialStore& credentials,
        ICloudEnrollmentRecordStore& journal,
        IRandom& random,
        CloudCredentialEnvelopeSealer& sealer,
        CloudCredentialEnrollmentGrantVerifier& grantVerifier);
    CloudCredentialEnrollmentCoordinator(
        const CloudCredentialEnrollmentCoordinator&) = delete;
    CloudCredentialEnrollmentCoordinator& operator=(
        const CloudCredentialEnrollmentCoordinator&) = delete;

    Result load();
    Result prepare();
    Result request(
        MutableByteSpan workspace,
        MutableByteSpan output,
        ByteView& encodedRequest);
    Result applyGrant(
        ByteView encodedGrant,
        uint64_t nowEpochSeconds,
        bool hasTrustedTime,
        MutableByteSpan workspace,
        MutableByteSpan receiptOutput,
        ByteView& encodedReceipt);
    Result receipt(
        MutableByteSpan workspace,
        MutableByteSpan output,
        ByteView& encodedReceipt);
    Result acceptCommitAck(
        ByteView encodedAck,
        MutableByteSpan workspace);
    Result verifyCommitAck(
        ByteView encodedAck,
        MutableByteSpan workspace);
    Result finishVerifiedCommit();
    Result restartPrepared();

    CloudCredentialEnrollmentState state() const { return state_; }

private:
    Result loadOwnership(OwnershipRecord& output) const;
    Result loadMatchingRecord(
        CloudEnrollmentRecord& record,
        OwnershipRecord& ownership) const;
    Result makeCredential(
        const CloudEnrollmentRecord& record,
        const OwnershipRecord& ownership,
        CloudCredential& credential) const;
    Result installGranted(
        const CloudEnrollmentRecord& record,
        const OwnershipRecord& ownership);
    Result encodeRequest(
        const CloudEnrollmentRecord& record,
        const OwnershipRecord& ownership,
        MutableByteSpan workspace,
        MutableByteSpan output,
        ByteView& encodedRequest);
    Result encodeReceipt(
        const CloudEnrollmentRecord& record,
        const OwnershipRecord& ownership,
        MutableByteSpan workspace,
        MutableByteSpan output,
        ByteView& encodedReceipt) const;
    Result fillNonZero(MutableByteSpan output);

    const DeviceInstanceId& deviceInstanceId_;
    IOwnershipSource& ownership_;
    ICloudCredentialStore& credentials_;
    ICloudEnrollmentRecordStore& journal_;
    IRandom& random_;
    CloudCredentialEnvelopeSealer& sealer_;
    CloudCredentialEnrollmentGrantVerifier& grantVerifier_;
    CloudCredentialEnrollmentState state_;
};

} // namespace blinker

#endif
