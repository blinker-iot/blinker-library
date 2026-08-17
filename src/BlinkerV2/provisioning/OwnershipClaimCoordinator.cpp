#include "OwnershipClaimCoordinator.h"

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
           (first.empty() ||
            memcmp(first.data, second.data, first.size) == 0);
}

} // namespace

OwnershipClaimCoordinator::OwnershipClaimCoordinator(
    const DeviceInstanceId& deviceInstanceId,
    IOwnershipRecordStore& ownership,
    IOwnershipClaimRecordStore& journal,
    IRandom& random,
    OwnershipClaimGrantVerifier& verifier)
    : deviceInstanceId_(deviceInstanceId),
      ownership_(ownership),
      journal_(journal),
      random_(random),
      verifier_(verifier) {}

Result OwnershipClaimCoordinator::encodeCurrentRequest(
    const OwnershipClaimRecord& record,
    MutableByteSpan output,
    ByteView& request) const {
    OwnershipClaimRequest body;
    body.requestId = ByteView(record.requestId, sizeof(record.requestId));
    body.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    body.ownershipGeneration = record.ownershipGeneration;
    body.claimNonce = ByteView(record.claimNonce, sizeof(record.claimNonce));
    return encodeOwnershipClaimRequest(body, output, request);
}

Result OwnershipClaimCoordinator::beginClaim(
    MutableByteSpan output,
    ByteView& request) {
    request = ByteView();
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        output.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size < kOwnershipClaimRequestMaxEncodedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    OwnershipClaimRecord record;
    Result result = journal_.load(record);
    if (result) {
        OwnershipRecord existing;
        const Result ownershipResult = ownership_.load(existing);
        if (ownershipResult) {
            if (existing.generation != record.ownershipGeneration) {
                result = Result::failure(ErrorCode::SequenceConflict);
            }
        } else if (ownershipResult.code() == ErrorCode::NotFound) {
            if (record.ownershipGeneration != 1U) {
                result = Result::failure(ErrorCode::SequenceConflict);
            }
        } else {
            result = ownershipResult;
        }
        clearOwnershipRecord(existing);
        if (result) result = encodeCurrentRequest(record, output, request);
        clearOwnershipClaimRecord(record);
        return result;
    }
    clearOwnershipClaimRecord(record);
    if (result.code() != ErrorCode::NotFound) return result;

    OwnershipRecord existing;
    result = ownership_.load(existing);
    if (result && existing.active()) {
        clearOwnershipRecord(existing);
        return Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        record.ownershipGeneration = existing.generation;
    } else if (result.code() == ErrorCode::NotFound) {
        record.ownershipGeneration = 1U;
        result = Result::success();
    }
    clearOwnershipRecord(existing);
    if (!result) return result;

    result = random_.fill(
        MutableByteSpan(record.requestId, sizeof(record.requestId)));
    if (result) {
        result = random_.fill(
            MutableByteSpan(record.claimNonce, sizeof(record.claimNonce)));
    }
    if (!result ||
        !nonZero(ByteView(record.requestId, sizeof(record.requestId))) ||
        !nonZero(ByteView(record.claimNonce, sizeof(record.claimNonce)))) {
        clearOwnershipClaimRecord(record);
        return result ? Result::failure(ErrorCode::InternalError) : result;
    }
    result = journal_.stage(record);
    if (result) result = encodeCurrentRequest(record, output, request);
    clearOwnershipClaimRecord(record);
    return result;
}

Result OwnershipClaimCoordinator::encodeReceipt(
    const OwnershipClaimGrant& grant,
    ByteView grantDigest,
    MutableByteSpan output,
    ByteView& receipt) const {
    OwnershipClaimReceipt body;
    body.grantId = grant.grantId;
    body.requestId = grant.requestId;
    body.deviceInstanceId = grant.deviceInstanceId;
    body.logicalDeviceId = grant.logicalDeviceId;
    body.ownershipGeneration = grant.ownershipGeneration;
    body.grantDigest = grantDigest;
    return encodeOwnershipClaimReceipt(body, output, receipt);
}

Result OwnershipClaimCoordinator::applyGrant(
    ByteView encodedGrant,
    uint64_t nowEpochSeconds,
    bool hasTrustedTime,
    MutableByteSpan operationWorkspace,
    MutableByteSpan output,
    ByteView& receipt) {
    receipt = ByteView();
    if (!isValidDeviceInstanceId(deviceInstanceId_) ||
        encodedGrant.data == nullptr || operationWorkspace.data == nullptr ||
        operationWorkspace.size < kOwnershipClaimWorkspaceSize ||
        output.data == nullptr ||
        overlaps(encodedGrant, operationWorkspace) ||
        overlaps(encodedGrant, output) ||
        overlaps(
            ByteView(operationWorkspace.data, operationWorkspace.size),
            output)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t grantDigest[kOwnershipClaimGrantDigestSize] = {};
    OwnershipClaimRecord claim;
    Result result = journal_.load(claim);
    if (!result) {
        clearOwnershipClaimRecord(claim);
        return result.code() == ErrorCode::NotFound
                   ? Result::failure(ErrorCode::AuthenticationRequired)
                   : result;
    }

    result = sha256(
        encodedGrant,
        MutableByteSpan(grantDigest, sizeof(grantDigest)));
    if (!result) {
        clearOwnershipClaimRecord(claim);
        return result;
    }

    OwnershipRecord existing;
    result = ownership_.load(existing);
    if (result && existing.active()) {
        OwnershipClaimGrant replay;
        result = decodeOwnershipClaimGrant(encodedGrant, replay);
        if (result &&
            (!constantTimeEqual(
                 ByteView(
                     existing.grantFingerprint,
                     sizeof(existing.grantFingerprint)),
                 ByteView(grantDigest, sizeof(grantDigest))) ||
             !constantTimeEqual(
                 replay.deviceInstanceId,
                 ByteView(
                     deviceInstanceId_.bytes,
                     sizeof(deviceInstanceId_.bytes))) ||
             !constantTimeEqual(
                 replay.requestId,
                 ByteView(claim.requestId, sizeof(claim.requestId))) ||
             !constantTimeEqual(
                 replay.claimNonce,
                 ByteView(claim.claimNonce, sizeof(claim.claimNonce))) ||
             replay.ownershipGeneration != existing.generation ||
             replay.ownershipGeneration != claim.ownershipGeneration ||
             !sameText(replay.logicalDeviceId, existing.logicalId()))) {
            result = Result::failure(ErrorCode::StateConflict);
        }
        if (result) {
            result = encodeReceipt(
                replay,
                ByteView(grantDigest, sizeof(grantDigest)),
                output,
                receipt);
        }
        clearOwnershipRecord(existing);
        clearOwnershipClaimRecord(claim);
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        return result;
    }
    if (!result && result.code() != ErrorCode::NotFound) {
        clearOwnershipRecord(existing);
        clearOwnershipClaimRecord(claim);
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        return result;
    }
    if ((result && existing.generation != claim.ownershipGeneration) ||
        (!result && claim.ownershipGeneration != 1U)) {
        clearOwnershipRecord(existing);
        clearOwnershipClaimRecord(claim);
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        return Result::failure(ErrorCode::SequenceConflict);
    }
    clearOwnershipRecord(existing);

    OwnershipClaimVerificationContext context;
    context.requestId = ByteView(claim.requestId, sizeof(claim.requestId));
    context.deviceInstanceId = ByteView(
        deviceInstanceId_.bytes,
        sizeof(deviceInstanceId_.bytes));
    context.ownershipGeneration = claim.ownershipGeneration;
    context.claimNonce = ByteView(claim.claimNonce, sizeof(claim.claimNonce));
    context.nowEpochSeconds = nowEpochSeconds;
    context.hasTrustedTime = hasTrustedTime;
    OwnershipClaimGrant grant;
    result = verifier_.verify(
        encodedGrant,
        context,
        operationWorkspace,
        grant);
    if (!result) {
        clearOwnershipClaimRecord(claim);
        secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
        return result;
    }

    OwnershipRecord record;
    record.state = OwnershipState::Active;
    record.generation = grant.ownershipGeneration;
    record.logicalDeviceIdLength =
        static_cast<uint8_t>(grant.logicalDeviceId.size);
    memcpy(
        record.logicalDeviceId,
        grant.logicalDeviceId.data,
        grant.logicalDeviceId.size);
    memcpy(
        record.grantFingerprint,
        grantDigest,
        sizeof(record.grantFingerprint));
    result = ownership_.installVerifiedActive(record);
    clearOwnershipRecord(record);
    if (result) {
        result = encodeReceipt(
            grant,
            ByteView(grantDigest, sizeof(grantDigest)),
            output,
            receipt);
    }
    clearOwnershipClaimRecord(claim);
    secureZero(operationWorkspace);
    secureZero(MutableByteSpan(grantDigest, sizeof(grantDigest)));
    return result;
}

Result OwnershipClaimCoordinator::verifyCommitAck(
    ByteView encodedCommitAck,
    MutableByteSpan operationWorkspace) {
    if (encodedCommitAck.data == nullptr || encodedCommitAck.empty() ||
        operationWorkspace.data == nullptr ||
        operationWorkspace.size < kOwnershipClaimWorkspaceSize ||
        overlaps(encodedCommitAck, operationWorkspace)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipClaimRecord claim;
    Result result = journal_.load(claim);
    if (!result) {
        clearOwnershipClaimRecord(claim);
        return result.code() == ErrorCode::NotFound
                   ? Result::failure(ErrorCode::StateConflict)
                   : result;
    }

    OwnershipRecord ownership;
    result = ownership_.load(ownership);
    if (!result || !ownership.active()) {
        clearOwnershipRecord(ownership);
        clearOwnershipClaimRecord(claim);
        return result
                   ? Result::failure(ErrorCode::StateConflict)
                   : result;
    }
    if (ownership.generation != claim.ownershipGeneration) {
        clearOwnershipRecord(ownership);
        clearOwnershipClaimRecord(claim);
        return Result::failure(ErrorCode::SequenceConflict);
    }

    OwnershipClaimCommitAck decoded;
    result = decodeOwnershipClaimCommitAck(encodedCommitAck, decoded);
    OwnershipClaimReceipt receipt;
    if (result) {
        receipt.grantId = decoded.grantId;
        receipt.requestId = ByteView(claim.requestId, sizeof(claim.requestId));
        receipt.deviceInstanceId = ByteView(
            deviceInstanceId_.bytes,
            sizeof(deviceInstanceId_.bytes));
        receipt.logicalDeviceId = ownership.logicalId();
        receipt.ownershipGeneration = ownership.generation;
        receipt.grantDigest = ByteView(
            ownership.grantFingerprint,
            sizeof(ownership.grantFingerprint));
        ByteView encodedReceipt;
        result = encodeOwnershipClaimReceipt(
            receipt,
            operationWorkspace,
            encodedReceipt);
        if (result) {
            uint8_t receiptDigest[kSha256Size] = {};
            result = sha256(
                encodedReceipt,
                MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
            OwnershipClaimCommitAckVerificationContext context;
            context.requestId = receipt.requestId;
            context.deviceInstanceId = receipt.deviceInstanceId;
            context.logicalDeviceId = receipt.logicalDeviceId;
            context.ownershipGeneration = receipt.ownershipGeneration;
            context.receiptDigest =
                ByteView(receiptDigest, sizeof(receiptDigest));
            if (result) {
                result = verifier_.verifyCommitAck(
                    encodedCommitAck,
                    context,
                    operationWorkspace,
                    decoded);
            }
            secureZero(
                MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
        }
    }
    secureZero(operationWorkspace);
    clearOwnershipRecord(ownership);
    clearOwnershipClaimRecord(claim);
    return result;
}

Result OwnershipClaimCoordinator::finishVerifiedClaim() {
    OwnershipClaimRecord claim;
    Result result = journal_.load(claim);
    if (result) result = journal_.clearExact(claim);
    if (!result && result.code() == ErrorCode::NotFound) {
        result = Result::success();
    }
    clearOwnershipClaimRecord(claim);
    return result;
}

Result OwnershipClaimCoordinator::completeClaim(
    ByteView encodedCommitAck,
    MutableByteSpan operationWorkspace) {
    Result result = verifyCommitAck(encodedCommitAck, operationWorkspace);
    if (result) result = finishVerifiedClaim();
    return result;
}

Result OwnershipClaimCoordinator::cancelClaim() {
    OwnershipClaimRecord claim;
    Result result = journal_.load(claim);
    if (!result) {
        clearOwnershipClaimRecord(claim);
        return result.code() == ErrorCode::NotFound
                   ? Result::success()
                   : result;
    }

    OwnershipRecord ownership;
    const Result ownershipResult = ownership_.load(ownership);
    if (ownershipResult && ownership.active()) {
        clearOwnershipRecord(ownership);
        clearOwnershipClaimRecord(claim);
        return Result::failure(ErrorCode::StateConflict);
    }
    if ((ownershipResult &&
         ownership.generation != claim.ownershipGeneration) ||
        (!ownershipResult &&
         ownershipResult.code() == ErrorCode::NotFound &&
         claim.ownershipGeneration != 1U) ||
        (!ownershipResult &&
         ownershipResult.code() != ErrorCode::NotFound)) {
        clearOwnershipRecord(ownership);
        clearOwnershipClaimRecord(claim);
        return ownershipResult
                   ? Result::failure(ErrorCode::SequenceConflict)
                   : ownershipResult;
    }
    clearOwnershipRecord(ownership);
    result = journal_.clearExact(claim);
    clearOwnershipClaimRecord(claim);
    return result;
}

} // namespace blinker
