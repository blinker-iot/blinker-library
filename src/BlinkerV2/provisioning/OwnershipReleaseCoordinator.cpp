#include "OwnershipReleaseCoordinator.h"

#include <stdint.h>
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

bool overlaps(ByteView input, MutableByteSpan storage) {
    if (input.empty() || storage.empty() || input.data == nullptr ||
        storage.data == nullptr) {
        return false;
    }
    const uintptr_t inputBegin = reinterpret_cast<uintptr_t>(input.data);
    const uintptr_t storageBegin = reinterpret_cast<uintptr_t>(storage.data);
    if (input.size > UINTPTR_MAX - inputBegin ||
        storage.size > UINTPTR_MAX - storageBegin) {
        return true;
    }
    return inputBegin < storageBegin + storage.size &&
           storageBegin < inputBegin + input.size;
}

bool sameText(StringView first, StringView second) {
    return first.size == second.size &&
           (first.empty() || memcmp(first.data, second.data, first.size) == 0);
}

bool validOperationPolicy(
    OwnershipReleaseOperation operation,
    OwnershipReleaseNetworkPolicy networkPolicy) {
    const bool operationValid =
        operation == OwnershipReleaseOperation::Reset ||
        operation == OwnershipReleaseOperation::Transfer;
    const bool policyValid =
        networkPolicy == OwnershipReleaseNetworkPolicy::Preserve ||
        networkPolicy == OwnershipReleaseNetworkPolicy::Clear;
    return operationValid && policyValid &&
           (operation != OwnershipReleaseOperation::Transfer ||
            networkPolicy == OwnershipReleaseNetworkPolicy::Clear);
}

} // namespace

OwnershipReleaseCoordinator::OwnershipReleaseCoordinator(
    const DeviceInstanceId& deviceInstanceId,
    IOwnershipRecordStore& ownership,
    IOwnershipReleaseRecordStore& journal,
    IRandom& random,
    OwnershipReleaseVerifier& verifier,
    OwnershipReleaseCleanup& cleanup)
    : deviceInstanceId_(deviceInstanceId),
      ownership_(ownership),
      journal_(journal),
      random_(random),
      verifier_(verifier),
      cleanup_(cleanup),
      state_(OwnershipReleaseState::Unknown) {}

Result OwnershipReleaseCoordinator::fillNonZero(MutableByteSpan output) {
    Result result = random_.fill(output);
    return result && !nonZero(ByteView(output.data, output.size))
               ? Result::failure(ErrorCode::InternalError)
               : result;
}

Result OwnershipReleaseCoordinator::validatePreparedOwnership(
    const OwnershipReleaseRecord& record) const {
    OwnershipRecord ownership;
    Result result = ownership_.load(ownership);
    if (result &&
        (!ownership.active() ||
         ownership.generation != record.currentGeneration ||
         !sameText(ownership.logicalId(), record.logicalId()))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    clearOwnershipRecord(ownership);
    return result;
}

Result OwnershipReleaseCoordinator::ensureRetired(
    const OwnershipReleaseRecord& record) {
    OwnershipRecord ownership;
    Result result = ownership_.load(ownership);
    if (!result) {
        clearOwnershipRecord(ownership);
        return result;
    }
    if (ownership.active()) {
        if (ownership.generation != record.currentGeneration ||
            !sameText(ownership.logicalId(), record.logicalId())) {
            clearOwnershipRecord(ownership);
            return Result::failure(ErrorCode::SequenceConflict);
        }
        clearOwnershipRecord(ownership);
        return ownership_.retireVerified(
            record.currentGeneration,
            record.nextGeneration);
    }
    const bool retired = ownership.generation == record.nextGeneration;
    clearOwnershipRecord(ownership);
    return retired ? Result::success()
                   : Result::failure(ErrorCode::SequenceConflict);
}

Result OwnershipReleaseCoordinator::load() {
    if (!isValidDeviceInstanceId(deviceInstanceId_)) {
        state_ = OwnershipReleaseState::Fault;
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord record;
    Result result = journal_.load(record);
    if (!result && result.code() == ErrorCode::NotFound) {
        OwnershipRecord ownership;
        result = ownership_.load(ownership);
        if (result) {
            state_ = ownership.active()
                         ? OwnershipReleaseState::Ready
                         : OwnershipReleaseState::Released;
        } else if (result.code() == ErrorCode::NotFound) {
            state_ = OwnershipReleaseState::NeedsOwnership;
            result = Result::success();
        } else {
            state_ = OwnershipReleaseState::Fault;
        }
        clearOwnershipRecord(ownership);
        clearOwnershipReleaseRecord(record);
        return result;
    }
    if (!result) {
        state_ = OwnershipReleaseState::Fault;
        clearOwnershipReleaseRecord(record);
        return result;
    }
    if (record.state == OwnershipReleaseRecordState::Prepared) {
        result = validatePreparedOwnership(record);
        state_ = result ? OwnershipReleaseState::Prepared
                        : OwnershipReleaseState::Fault;
    } else {
        result = ensureRetired(record);
        if (result &&
            record.state == OwnershipReleaseRecordState::GrantAccepted) {
            state_ = OwnershipReleaseState::AwaitingCommitAck;
        } else if (result) {
            state_ = OwnershipReleaseState::Finalizing;
            result = finalizeAcknowledged(record);
        } else {
            state_ = OwnershipReleaseState::Fault;
        }
    }
    clearOwnershipReleaseRecord(record);
    return result;
}

Result OwnershipReleaseCoordinator::prepare(
    OwnershipReleaseOperation operation,
    OwnershipReleaseNetworkPolicy networkPolicy) {
    if (state_ != OwnershipReleaseState::Ready) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!validOperationPolicy(operation, networkPolicy)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipRecord ownership;
    Result result = ownership_.load(ownership);
    if (!result) {
        clearOwnershipRecord(ownership);
        return result;
    }
    if (!ownership.active() || ownership.generation == UINT32_MAX) {
        clearOwnershipRecord(ownership);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    OwnershipReleaseRecord record;
    record.operation = operation;
    record.networkPolicy = networkPolicy;
    record.currentGeneration = ownership.generation;
    record.logicalDeviceIdLength = ownership.logicalDeviceIdLength;
    memcpy(
        record.logicalDeviceId,
        ownership.logicalDeviceId,
        sizeof(record.logicalDeviceId));
    clearOwnershipRecord(ownership);
    result = fillNonZero(MutableByteSpan(record.requestId, sizeof(record.requestId)));
    if (result) {
        result = fillNonZero(MutableByteSpan(
            record.releaseNonce,
            sizeof(record.releaseNonce)));
    }
    if (result) result = journal_.stagePrepared(record);
    clearOwnershipReleaseRecord(record);
    if (result) state_ = OwnershipReleaseState::Prepared;
    return result;
}

Result OwnershipReleaseCoordinator::encodeRequest(
    const OwnershipReleaseRecord& record,
    MutableByteSpan output,
    ByteView& encodedRequest) const {
    OwnershipReleaseRequest request;
    request.requestId = ByteView(record.requestId, sizeof(record.requestId));
    request.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    request.logicalDeviceId = record.logicalId();
    request.currentGeneration = record.currentGeneration;
    request.releaseNonce = ByteView(
        record.releaseNonce,
        sizeof(record.releaseNonce));
    request.operation = record.operation;
    request.networkPolicy = record.networkPolicy;
    return encodeOwnershipReleaseRequest(request, output, encodedRequest);
}

Result OwnershipReleaseCoordinator::request(
    MutableByteSpan output,
    ByteView& encodedRequest) {
    encodedRequest = ByteView();
    if (state_ != OwnershipReleaseState::Prepared) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    OwnershipReleaseRecord record;
    Result result = journal_.load(record);
    if (result && record.state != OwnershipReleaseRecordState::Prepared) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = validatePreparedOwnership(record);
    if (result) result = encodeRequest(record, output, encodedRequest);
    clearOwnershipReleaseRecord(record);
    return result;
}

Result OwnershipReleaseCoordinator::encodeReceipt(
    const OwnershipReleaseRecord& record,
    MutableByteSpan output,
    ByteView& encodedReceipt) const {
    OwnershipReleaseReceipt receipt;
    receipt.grantId = ByteView(record.grantId, sizeof(record.grantId));
    receipt.requestId = ByteView(record.requestId, sizeof(record.requestId));
    receipt.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    receipt.logicalDeviceId = record.logicalId();
    receipt.currentGeneration = record.currentGeneration;
    receipt.nextGeneration = record.nextGeneration;
    receipt.operation = record.operation;
    receipt.networkPolicy = record.networkPolicy;
    receipt.grantDigest = ByteView(
        record.grantDigest,
        sizeof(record.grantDigest));
    return encodeOwnershipReleaseReceipt(receipt, output, encodedReceipt);
}

Result OwnershipReleaseCoordinator::applyGrant(
    ByteView encodedGrant,
    uint64_t nowEpochSeconds,
    bool hasTrustedTime,
    MutableByteSpan workspace,
    MutableByteSpan receiptOutput,
    ByteView& encodedReceipt) {
    encodedReceipt = ByteView();
    if (state_ != OwnershipReleaseState::Prepared) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (encodedGrant.data == nullptr || encodedGrant.empty() ||
        workspace.data == nullptr ||
        workspace.size < kOwnershipReleaseWorkspaceSize ||
        receiptOutput.data == nullptr ||
        overlaps(encodedGrant, workspace) ||
        overlaps(encodedGrant, receiptOutput) ||
        overlaps(ByteView(workspace.data, workspace.size), receiptOutput)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord prepared;
    Result result = journal_.load(prepared);
    if (result && prepared.state != OwnershipReleaseRecordState::Prepared) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = validatePreparedOwnership(prepared);
    ByteView encodedRequest;
    if (result) result = encodeRequest(prepared, workspace, encodedRequest);
    OwnershipReleaseRequest requestBody;
    if (result) result = decodeOwnershipReleaseRequest(encodedRequest, requestBody);
    OwnershipReleaseGrant grant;
    if (result) {
        result = verifier_.verifyGrant(
            encodedGrant,
            requestBody,
            encodedRequest,
            nowEpochSeconds,
            hasTrustedTime,
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
            prepared,
            grant.grantId,
            grant.nextGeneration,
            ByteView(grantDigest, sizeof(grantDigest)));
    }
    if (result) {
        prepared.state = OwnershipReleaseRecordState::GrantAccepted;
        prepared.nextGeneration = grant.nextGeneration;
        memcpy(prepared.grantId, grant.grantId.data, sizeof(prepared.grantId));
        memcpy(prepared.grantDigest, grantDigest, sizeof(prepared.grantDigest));
        // From this point the durable journal, not the caller's buffer or the
        // current process, owns recovery of the release transaction.
        state_ = OwnershipReleaseState::AwaitingCommitAck;
        result = ensureRetired(prepared);
    }
    if (result) {
        result = encodeReceipt(prepared, receiptOutput, encodedReceipt);
    }
    secureZero(workspace);
    secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
    clearOwnershipReleaseRecord(prepared);
    return result;
}

Result OwnershipReleaseCoordinator::receipt(
    MutableByteSpan output,
    ByteView& encodedReceipt) {
    encodedReceipt = ByteView();
    if (state_ != OwnershipReleaseState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    OwnershipReleaseRecord record;
    Result result = journal_.load(record);
    if (result &&
        record.state != OwnershipReleaseRecordState::GrantAccepted) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = ensureRetired(record);
    if (result) result = encodeReceipt(record, output, encodedReceipt);
    clearOwnershipReleaseRecord(record);
    return result;
}

Result OwnershipReleaseCoordinator::finalizeAcknowledged(
    OwnershipReleaseRecord& record) {
    Result result = cleanup_.finalize(record.networkPolicy);
    if (result) result = journal_.clearExact(record);
    state_ = result ? OwnershipReleaseState::Released
                    : OwnershipReleaseState::Finalizing;
    return result;
}

Result OwnershipReleaseCoordinator::acceptCommitAck(
    ByteView encodedAck,
    MutableByteSpan workspace) {
    if (state_ != OwnershipReleaseState::AwaitingCommitAck) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (encodedAck.data == nullptr || encodedAck.empty() ||
        workspace.data == nullptr ||
        workspace.size < kOwnershipReleaseWorkspaceSize ||
        overlaps(encodedAck, workspace)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseRecord record;
    Result result = journal_.load(record);
    if (result &&
        record.state != OwnershipReleaseRecordState::GrantAccepted) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = ensureRetired(record);
    ByteView encodedReceipt;
    if (result) result = encodeReceipt(record, workspace, encodedReceipt);
    OwnershipReleaseReceipt receiptBody;
    if (result) result = decodeOwnershipReleaseReceipt(encodedReceipt, receiptBody);
    OwnershipReleaseCommitAck ack;
    if (result) {
        result = verifier_.verifyCommitAck(
            encodedAck,
            receiptBody,
            encodedReceipt,
            workspace,
            ack);
    }
    if (result) result = journal_.acknowledgeCommit(record);
    if (result) {
        record.state = OwnershipReleaseRecordState::CommitAcknowledged;
        state_ = OwnershipReleaseState::Finalizing;
        result = finalizeAcknowledged(record);
    }
    secureZero(workspace);
    clearOwnershipReleaseRecord(record);
    return result;
}

Result OwnershipReleaseCoordinator::restartPrepared() {
    if (state_ != OwnershipReleaseState::Prepared) {
        return Result::failure(ErrorCode::StateConflict);
    }
    OwnershipReleaseRecord record;
    Result result = journal_.load(record);
    if (result && record.state != OwnershipReleaseRecordState::Prepared) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = journal_.clearExact(record);
    clearOwnershipReleaseRecord(record);
    if (result) state_ = OwnershipReleaseState::Ready;
    return result;
}

} // namespace blinker
