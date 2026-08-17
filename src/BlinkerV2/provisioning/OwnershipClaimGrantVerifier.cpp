#include "OwnershipClaimGrantVerifier.h"

#include "../core/SecureMemory.h"

#include <string.h>

namespace blinker {

namespace {

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool sameBytes(ByteView first, ByteView second) {
    return first.size == second.size && constantTimeEqual(first, second);
}

bool validContext(const OwnershipClaimVerificationContext& context) {
    return exactNonZero(
               context.requestId,
               kOwnershipClaimRequestIdSize) &&
           exactNonZero(
               context.deviceInstanceId,
               kDeviceInstanceIdSize) &&
           context.ownershipGeneration != 0U &&
           exactNonZero(
               context.claimNonce,
               kOwnershipClaimNonceSize) &&
           (!context.hasTrustedTime || context.nowEpochSeconds != 0U);
}

bool sameText(StringView first, StringView second) {
    return first.data != nullptr && second.data != nullptr &&
           first.size == second.size &&
           memcmp(first.data, second.data, first.size) == 0;
}

bool validContext(
    const OwnershipClaimCommitAckVerificationContext& context) {
    return exactNonZero(context.requestId, kOwnershipClaimRequestIdSize) &&
           exactNonZero(context.deviceInstanceId, kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(context.logicalDeviceId).ok() &&
           context.ownershipGeneration != 0U &&
           exactNonZero(context.receiptDigest, kSha256Size);
}

} // namespace

Result OwnershipClaimGrantVerifier::verify(
    ByteView encodedGrant,
    const OwnershipClaimVerificationContext& context,
    MutableByteSpan transcriptWorkspace,
    OwnershipClaimGrant& grant) {
    grant = OwnershipClaimGrant();
    if (!validContext(context) || transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kOwnershipClaimWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    OwnershipClaimGrant decoded;
    Result result = decodeOwnershipClaimGrant(encodedGrant, decoded);
    if (!result) return result;
    if (!sameBytes(decoded.requestId, context.requestId) ||
        !sameBytes(decoded.deviceInstanceId, context.deviceInstanceId) ||
        decoded.ownershipGeneration != context.ownershipGeneration ||
        !sameBytes(decoded.claimNonce, context.claimNonce)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (context.hasTrustedTime &&
        (context.nowEpochSeconds < decoded.issuedAt ||
         context.nowEpochSeconds >= decoded.expiresAt)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    secureZero(transcriptWorkspace);
    ByteView transcript;
    result = encodeOwnershipClaimGrantTranscript(
        decoded,
        transcriptWorkspace,
        transcript);
    uint8_t digest[kSha256Size] = {};
    if (result) {
        result = sha256(
            transcript,
            MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        result = signatureVerifier_.verifyDigest(
            decoded.serverKeyId,
            decoded.signatureAlgorithm,
            ByteView(digest, sizeof(digest)),
            decoded.signature);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(transcriptWorkspace);
    if (result) grant = decoded;
    return result;
}

Result OwnershipClaimGrantVerifier::verifyCommitAck(
    ByteView encodedAck,
    const OwnershipClaimCommitAckVerificationContext& context,
    MutableByteSpan transcriptWorkspace,
    OwnershipClaimCommitAck& ack) {
    ack = OwnershipClaimCommitAck();
    if (!validContext(context) || transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kOwnershipClaimWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    OwnershipClaimCommitAck decoded;
    Result result = decodeOwnershipClaimCommitAck(encodedAck, decoded);
    if (!result) return result;
    if (!sameBytes(decoded.requestId, context.requestId) ||
        !sameBytes(decoded.deviceInstanceId, context.deviceInstanceId) ||
        !sameText(decoded.logicalDeviceId, context.logicalDeviceId) ||
        decoded.ownershipGeneration != context.ownershipGeneration ||
        !sameBytes(decoded.receiptDigest, context.receiptDigest)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    secureZero(transcriptWorkspace);
    ByteView transcript;
    result = encodeOwnershipClaimCommitAckTranscript(
        decoded,
        transcriptWorkspace,
        transcript);
    uint8_t digest[kSha256Size] = {};
    if (result) {
        result = sha256(
            transcript,
            MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        result = signatureVerifier_.verifyDigest(
            decoded.serverKeyId,
            decoded.signatureAlgorithm,
            ByteView(digest, sizeof(digest)),
            decoded.signature);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(transcriptWorkspace);
    if (result) ack = decoded;
    return result;
}

} // namespace blinker
