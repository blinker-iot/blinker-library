#include "CloudCredentialEnrollmentCoordinator.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool same(ByteView first, ByteView second) {
    return first.size == second.size && constantTimeEqual(first, second);
}

bool same(StringView first, StringView second) {
    return first.size == second.size && first.data != nullptr &&
           second.data != nullptr &&
           memcmp(first.data, second.data, first.size) == 0;
}

} // namespace

CloudCredentialEnrollmentCoordinator::
    CloudCredentialEnrollmentCoordinator(
        const DeviceInstanceId& deviceInstanceId,
        IOwnershipSource& ownership,
        ICloudCredentialStore& credentials,
        ICloudEnrollmentRecordStore& journal,
        IRandom& random,
        CloudCredentialEnvelopeSealer& sealer,
        CloudCredentialEnrollmentGrantVerifier& grantVerifier)
    : deviceInstanceId_(deviceInstanceId),
      ownership_(ownership),
      credentials_(credentials),
      journal_(journal),
      random_(random),
      sealer_(sealer),
      grantVerifier_(grantVerifier),
      state_(CloudCredentialEnrollmentState::Unknown) {}

Result CloudCredentialEnrollmentCoordinator::loadOwnership(
    OwnershipRecord& output) const {
    Result result = ownership_.load(output);
    if (result && !output.active()) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    return result;
}

Result CloudCredentialEnrollmentCoordinator::loadMatchingRecord(
    CloudEnrollmentRecord& record,
    OwnershipRecord& ownership) const {
    Result result = loadOwnership(ownership);
    if (result) result = journal_.load(record);
    if (result &&
        record.ownershipGeneration != ownership.generation) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    return result;
}

Result CloudCredentialEnrollmentCoordinator::makeCredential(
    const CloudEnrollmentRecord& record,
    const OwnershipRecord& ownership,
    CloudCredential& credential) const {
    credential = CloudCredential();
    credential.suite = CloudCredentialSuite::HmacSha256_32;
    credential.generation = record.ownershipGeneration;
    credential.credentialVersion = record.credentialVersion;
    credential.logicalDeviceIdLength = ownership.logicalDeviceIdLength;
    memcpy(
        credential.logicalDeviceId,
        ownership.logicalDeviceId,
        ownership.logicalDeviceIdLength);
    memcpy(
        credential.secret,
        record.cloudSecret,
        sizeof(credential.secret));
    return validateCloudCredential(credential);
}

Result CloudCredentialEnrollmentCoordinator::installGranted(
    const CloudEnrollmentRecord& record,
    const OwnershipRecord& ownership) {
    if (record.state != CloudEnrollmentRecordState::GrantAccepted) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudCredential credential;
    Result result = makeCredential(record, ownership, credential);
    if (result) result = credentials_.installFresh(credential);
    clearCloudCredential(credential);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::load() {
    state_ = CloudCredentialEnrollmentState::Unknown;
    if (!isValidDeviceInstanceId(deviceInstanceId_)) {
        state_ = CloudCredentialEnrollmentState::Fault;
        return Result::failure(ErrorCode::InvalidArgument);
    }

    OwnershipRecord ownership;
    Result ownershipResult = ownership_.load(ownership);
    if (!ownershipResult || !ownership.active()) {
        clearOwnershipRecord(ownership);
        if (!ownershipResult && ownershipResult.code() != ErrorCode::NotFound) {
            state_ = CloudCredentialEnrollmentState::Fault;
            return ownershipResult;
        }
        state_ = CloudCredentialEnrollmentState::NeedsOwnership;
        return Result::success();
    }

    CloudCredential credential;
    Result credentialResult = credentials_.load(credential);
    if (credentialResult &&
        (credential.generation != ownership.generation ||
         !same(credential.logicalId(), ownership.logicalId()))) {
        clearCloudCredential(credential);
        clearOwnershipRecord(ownership);
        state_ = CloudCredentialEnrollmentState::Fault;
        return Result::failure(ErrorCode::SequenceConflict);
    }
    if (!credentialResult &&
        credentialResult.code() != ErrorCode::NotFound) {
        clearCloudCredential(credential);
        clearOwnershipRecord(ownership);
        state_ = CloudCredentialEnrollmentState::Fault;
        return credentialResult;
    }

    CloudEnrollmentRecord record;
    Result journalResult = journal_.load(record);
    if (!journalResult && journalResult.code() == ErrorCode::NotFound) {
        state_ = credentialResult
                     ? CloudCredentialEnrollmentState::Active
                     : CloudCredentialEnrollmentState::Ready;
        clearCloudEnrollmentRecord(record);
        clearCloudCredential(credential);
        clearOwnershipRecord(ownership);
        return Result::success();
    }
    if (!journalResult ||
        record.ownershipGeneration != ownership.generation) {
        clearCloudEnrollmentRecord(record);
        clearCloudCredential(credential);
        clearOwnershipRecord(ownership);
        state_ = CloudCredentialEnrollmentState::Fault;
        return journalResult
                   ? Result::failure(ErrorCode::SequenceConflict)
                   : journalResult;
    }

    Result result = Result::success();
    if (record.state == CloudEnrollmentRecordState::Prepared) {
        if (credentialResult) {
            result = Result::failure(ErrorCode::StateConflict);
            state_ = CloudCredentialEnrollmentState::Fault;
        } else {
            state_ = CloudCredentialEnrollmentState::Prepared;
        }
    } else {
        result = installGranted(record, ownership);
        state_ = result
                     ? CloudCredentialEnrollmentState::AwaitingCommitAck
                     : CloudCredentialEnrollmentState::Fault;
    }
    clearCloudEnrollmentRecord(record);
    clearCloudCredential(credential);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::fillNonZero(
    MutableByteSpan output) {
    for (uint8_t attempt = 0U; attempt < 4U; ++attempt) {
        Result result = random_.fill(output);
        if (!result) return result;
        if (nonZero(ByteView(output.data, output.size))) {
            return Result::success();
        }
    }
    return Result::failure(ErrorCode::InternalError);
}

Result CloudCredentialEnrollmentCoordinator::prepare() {
    if (state_ == CloudCredentialEnrollmentState::Unknown) {
        Result result = load();
        if (!result) return result;
    }
    if (state_ == CloudCredentialEnrollmentState::Prepared) {
        return Result::success();
    }
    if (state_ != CloudCredentialEnrollmentState::Ready) {
        return Result::failure(ErrorCode::StateConflict);
    }

    OwnershipRecord ownership;
    Result result = loadOwnership(ownership);
    CloudEnrollmentRecord record;
    if (result) {
        record.ownershipGeneration = ownership.generation;
        record.credentialVersion = 1U;
        result = sealer_.activeKeyId(record.encryptionKeyId);
    }
    if (result && record.encryptionKeyId == 0U) {
        result = Result::failure(ErrorCode::NotConfigured);
    }
    if (result) {
        result = fillNonZero(MutableByteSpan(
            record.requestId,
            sizeof(record.requestId)));
    }
    if (result) {
        result = fillNonZero(MutableByteSpan(
            record.ephemeralPrivateKey,
            sizeof(record.ephemeralPrivateKey)));
    }
    if (result) {
        result = fillNonZero(MutableByteSpan(
            record.cloudSecret,
            sizeof(record.cloudSecret)));
    }
    if (result) result = journal_.stagePrepared(record);
    if (result) state_ = CloudCredentialEnrollmentState::Prepared;
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::encodeRequest(
    const CloudEnrollmentRecord& record,
    const OwnershipRecord& ownership,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& encodedRequest) {
    uint8_t ephemeralPublic[kCloudEnrollmentEphemeralPublicKeySize] = {};
    uint8_t nonce[kCloudEnrollmentNonceSize] = {};
    uint8_t encrypted[kCloudEnrollmentEncryptedCredentialSize] = {};
    CloudCredentialEnrollmentRequest request;
    request.requestId = ByteView(record.requestId, sizeof(record.requestId));
    request.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    request.logicalDeviceId = ownership.logicalId();
    request.ownershipGeneration = record.ownershipGeneration;
    request.credentialVersion = record.credentialVersion;
    request.encryptionKeyId = record.encryptionKeyId;
    Result result = sealer_.seal(
        request,
        ByteView(
            record.ephemeralPrivateKey,
            sizeof(record.ephemeralPrivateKey)),
        ByteView(record.cloudSecret, sizeof(record.cloudSecret)),
        workspace,
        MutableByteSpan(ephemeralPublic, sizeof(ephemeralPublic)),
        MutableByteSpan(nonce, sizeof(nonce)),
        MutableByteSpan(encrypted, sizeof(encrypted)));
    if (result) {
        result = encodeCloudCredentialEnrollmentRequest(
            request,
            output,
            encodedRequest);
    }
    secureZero(MutableByteSpan(ephemeralPublic, sizeof(ephemeralPublic)));
    secureZero(MutableByteSpan(nonce, sizeof(nonce)));
    secureZero(MutableByteSpan(encrypted, sizeof(encrypted)));
    return result;
}

Result CloudCredentialEnrollmentCoordinator::request(
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& encodedRequest) {
    encodedRequest = ByteView();
    if (state_ != CloudCredentialEnrollmentState::Prepared &&
        state_ != CloudCredentialEnrollmentState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudEnrollmentRecord record;
    OwnershipRecord ownership;
    Result result = loadMatchingRecord(record, ownership);
    if (result) {
        result = encodeRequest(
            record,
            ownership,
            workspace,
            output,
            encodedRequest);
    }
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::encodeReceipt(
    const CloudEnrollmentRecord& record,
    const OwnershipRecord& ownership,
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& encodedReceipt) const {
    if (record.state != CloudEnrollmentRecordState::GrantAccepted) {
        return Result::failure(ErrorCode::StateConflict);
    }
    uint8_t proof[kCloudEnrollmentProofSize] = {};
    CloudCredentialEnrollmentReceipt receipt;
    receipt.requestId = ByteView(record.requestId, sizeof(record.requestId));
    receipt.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    receipt.logicalDeviceId = ownership.logicalId();
    receipt.ownershipGeneration = record.ownershipGeneration;
    receipt.credentialVersion = record.credentialVersion;
    receipt.grantDigest = ByteView(
        record.grantDigest,
        sizeof(record.grantDigest));
    Result result = computeCloudCredentialEnrollmentReceiptProof(
        ByteView(record.cloudSecret, sizeof(record.cloudSecret)),
        receipt,
        workspace,
        MutableByteSpan(proof, sizeof(proof)));
    if (result) {
        receipt.proof = ByteView(proof, sizeof(proof));
        result = encodeCloudCredentialEnrollmentReceipt(
            receipt,
            output,
            encodedReceipt);
    }
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

Result CloudCredentialEnrollmentCoordinator::applyGrant(
    ByteView encodedGrant,
    uint64_t nowEpochSeconds,
    bool hasTrustedTime,
    MutableByteSpan workspace,
    MutableByteSpan receiptOutput,
    ByteView& encodedReceipt) {
    encodedReceipt = ByteView();
    if (state_ != CloudCredentialEnrollmentState::Prepared &&
        state_ != CloudCredentialEnrollmentState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (workspace.data == nullptr ||
        workspace.size < kCloudEnrollmentWorkspaceSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    CloudEnrollmentRecord record;
    OwnershipRecord ownership;
    Result result = loadMatchingRecord(record, ownership);
    if (!result) {
        clearCloudEnrollmentRecord(record);
        clearOwnershipRecord(ownership);
        return result;
    }

    if (record.state == CloudEnrollmentRecordState::GrantAccepted) {
        uint8_t digest[kSha256Size] = {};
        result = sha256(
            encodedGrant,
            MutableByteSpan(digest, sizeof(digest)));
        if (result && !same(
                          ByteView(digest, sizeof(digest)),
                          ByteView(
                              record.grantDigest,
                              sizeof(record.grantDigest)))) {
            result = Result::failure(ErrorCode::StateConflict);
        }
        if (result) {
            result = encodeReceipt(
                record,
                ownership,
                workspace,
                receiptOutput,
                encodedReceipt);
        }
        secureZero(MutableByteSpan(digest, sizeof(digest)));
        clearCloudEnrollmentRecord(record);
        clearOwnershipRecord(ownership);
        return result;
    }

    ByteView encodedRequest;
    result = encodeRequest(
        record,
        ownership,
        workspace,
        workspace,
        encodedRequest);
    uint8_t requestDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedRequest,
            MutableByteSpan(requestDigest, sizeof(requestDigest)));
    }
    CloudCredentialEnrollmentVerificationContext context;
    context.requestId = ByteView(record.requestId, sizeof(record.requestId));
    context.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    context.logicalDeviceId = ownership.logicalId();
    context.ownershipGeneration = record.ownershipGeneration;
    context.credentialVersion = record.credentialVersion;
    context.requestDigest = ByteView(requestDigest, sizeof(requestDigest));
    context.nowEpochSeconds = nowEpochSeconds;
    context.hasTrustedTime = hasTrustedTime;
    CloudCredentialEnrollmentGrant grant;
    if (result) {
        result = grantVerifier_.verify(
            encodedGrant,
            context,
            workspace,
            grant);
    }
    uint8_t grantDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedGrant,
            MutableByteSpan(grantDigest, sizeof(grantDigest)));
    }
    if (result) {
        result = journal_.acceptGrant(
            record,
            ByteView(grantDigest, sizeof(grantDigest)));
    }
    if (result) {
        record.state = CloudEnrollmentRecordState::GrantAccepted;
        memcpy(record.grantDigest, grantDigest, sizeof(record.grantDigest));
        result = installGranted(record, ownership);
        if (!result) state_ = CloudCredentialEnrollmentState::Fault;
    }
    if (result) {
        state_ = CloudCredentialEnrollmentState::AwaitingCommitAck;
        result = encodeReceipt(
            record,
            ownership,
            workspace,
            receiptOutput,
            encodedReceipt);
    }
    secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
    secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::receipt(
    MutableByteSpan workspace,
    MutableByteSpan output,
    ByteView& encodedReceipt) {
    encodedReceipt = ByteView();
    if (state_ != CloudCredentialEnrollmentState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudEnrollmentRecord record;
    OwnershipRecord ownership;
    Result result = loadMatchingRecord(record, ownership);
    if (result) {
        result = encodeReceipt(
            record,
            ownership,
            workspace,
            output,
            encodedReceipt);
    }
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::verifyCommitAck(
    ByteView encodedAck,
    MutableByteSpan workspace) {
    if (state_ != CloudCredentialEnrollmentState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudEnrollmentRecord record;
    OwnershipRecord ownership;
    Result result = loadMatchingRecord(record, ownership);
    uint8_t receiptBuffer[kCloudEnrollmentReceiptMaxEncodedSize] = {};
    ByteView encodedReceipt;
    if (result) {
        result = encodeReceipt(
            record,
            ownership,
            workspace,
            MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)),
            encodedReceipt);
    }
    uint8_t receiptDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedReceipt,
            MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
    }
    CloudCredentialEnrollmentCommitAck ack;
    if (result) {
        result = decodeCloudCredentialEnrollmentCommitAck(encodedAck, ack);
    }
    if (result &&
        (!same(ack.requestId, ByteView(record.requestId, sizeof(record.requestId))) ||
         !same(
             ack.deviceInstanceId,
             ByteView(deviceInstanceId_.bytes, sizeof(deviceInstanceId_.bytes))) ||
         !same(ack.logicalDeviceId, ownership.logicalId()) ||
         ack.ownershipGeneration != record.ownershipGeneration ||
         ack.credentialVersion != record.credentialVersion ||
         !same(
             ack.receiptDigest,
             ByteView(receiptDigest, sizeof(receiptDigest))))) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result) {
        result = verifyCloudCredentialEnrollmentCommitAckProof(
            ByteView(record.cloudSecret, sizeof(record.cloudSecret)),
            ack,
            workspace);
    }
    secureZero(MutableByteSpan(receiptBuffer, sizeof(receiptBuffer)));
    secureZero(MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::finishVerifiedCommit() {
    if (state_ == CloudCredentialEnrollmentState::Active) {
        return Result::success();
    }
    if (state_ != CloudCredentialEnrollmentState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudEnrollmentRecord record;
    Result result = journal_.load(record);
    if (result) result = journal_.clearExact(record);
    if (!result && result.code() == ErrorCode::NotFound) {
        result = Result::success();
    }
    if (result) state_ = CloudCredentialEnrollmentState::Active;
    clearCloudEnrollmentRecord(record);
    return result;
}

Result CloudCredentialEnrollmentCoordinator::acceptCommitAck(
    ByteView encodedAck,
    MutableByteSpan workspace) {
    Result result = verifyCommitAck(encodedAck, workspace);
    if (result) result = finishVerifiedCommit();
    return result;
}

Result CloudCredentialEnrollmentCoordinator::restartPrepared() {
    if (state_ != CloudCredentialEnrollmentState::Prepared) {
        return Result::failure(ErrorCode::StateConflict);
    }
    CloudEnrollmentRecord record;
    OwnershipRecord ownership;
    Result result = loadMatchingRecord(record, ownership);
    if (result && record.state != CloudEnrollmentRecordState::Prepared) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = journal_.clearExact(record);
    if (result) state_ = CloudCredentialEnrollmentState::Ready;
    clearCloudEnrollmentRecord(record);
    clearOwnershipRecord(ownership);
    return result;
}

} // namespace blinker
