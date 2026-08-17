#include "OwnershipReleaseVerifier.h"

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool sameBytes(ByteView first, ByteView second) {
    return constantTimeEqual(first, second);
}

bool sameString(StringView first, StringView second) {
    return constantTimeEqual(
        ByteView(reinterpret_cast<const uint8_t*>(first.data), first.size),
        ByteView(reinterpret_cast<const uint8_t*>(second.data), second.size));
}

Result verifySignedDigest(
    IServerSignatureVerifier& verifier,
    uint32_t keyId,
    ServerSignatureAlgorithm algorithm,
    ByteView transcript,
    ByteView signature) {
    uint8_t digest[kSha256Size] = {};
    Result result = sha256(transcript, MutableByteSpan(digest, sizeof(digest)));
    if (result) {
        result = verifier.verifyDigest(
            keyId,
            algorithm,
            ByteView(digest, sizeof(digest)),
            signature);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

} // namespace

Result OwnershipReleaseVerifier::verifyGrant(
    ByteView encodedGrant,
    const OwnershipReleaseRequest& request,
    ByteView encodedRequest,
    uint64_t nowEpochSeconds,
    bool hasTrustedTime,
    MutableByteSpan workspace,
    OwnershipReleaseGrant& grant) {
    if (workspace.data == nullptr ||
        workspace.size < kOwnershipReleaseWorkspaceSize ||
        encodedRequest.data == nullptr || encodedRequest.empty() ||
        (hasTrustedTime && nowEpochSeconds == 0U)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseGrant decoded;
    Result result = decodeOwnershipReleaseGrant(encodedGrant, decoded);
    uint8_t requestDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedRequest,
            MutableByteSpan(requestDigest, sizeof(requestDigest)));
    }
    if (result &&
        (!sameBytes(decoded.requestId, request.requestId) ||
         !sameBytes(decoded.deviceInstanceId, request.deviceInstanceId) ||
         !sameString(decoded.logicalDeviceId, request.logicalDeviceId) ||
         decoded.currentGeneration != request.currentGeneration ||
         !sameBytes(decoded.releaseNonce, request.releaseNonce) ||
         decoded.operation != request.operation ||
         decoded.networkPolicy != request.networkPolicy ||
         !sameBytes(
             decoded.requestDigest,
             ByteView(requestDigest, sizeof(requestDigest))))) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result && hasTrustedTime &&
        (nowEpochSeconds < decoded.issuedAt ||
         nowEpochSeconds >= decoded.expiresAt)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    ByteView transcript;
    if (result) {
        result = encodeOwnershipReleaseGrantTranscript(
            decoded,
            workspace,
            transcript);
    }
    if (result) {
        result = verifySignedDigest(
            signatureVerifier_,
            decoded.serverKeyId,
            decoded.signatureAlgorithm,
            transcript,
            decoded.signature);
    }
    secureZero(MutableByteSpan(requestDigest, sizeof(requestDigest)));
    if (result) grant = decoded;
    return result;
}

Result OwnershipReleaseVerifier::verifyCommitAck(
    ByteView encodedAck,
    const OwnershipReleaseReceipt& receipt,
    ByteView encodedReceipt,
    MutableByteSpan workspace,
    OwnershipReleaseCommitAck& ack) {
    if (workspace.data == nullptr ||
        workspace.size < kOwnershipReleaseWorkspaceSize ||
        encodedReceipt.data == nullptr || encodedReceipt.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    OwnershipReleaseCommitAck decoded;
    Result result = decodeOwnershipReleaseCommitAck(encodedAck, decoded);
    uint8_t receiptDigest[kSha256Size] = {};
    if (result) {
        result = sha256(
            encodedReceipt,
            MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
    }
    if (result &&
        (!sameBytes(decoded.grantId, receipt.grantId) ||
         !sameBytes(decoded.requestId, receipt.requestId) ||
         !sameBytes(decoded.deviceInstanceId, receipt.deviceInstanceId) ||
         !sameString(decoded.logicalDeviceId, receipt.logicalDeviceId) ||
         decoded.currentGeneration != receipt.currentGeneration ||
         decoded.nextGeneration != receipt.nextGeneration ||
         decoded.operation != receipt.operation ||
         decoded.networkPolicy != receipt.networkPolicy ||
         !sameBytes(
             decoded.receiptDigest,
             ByteView(receiptDigest, sizeof(receiptDigest))))) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    ByteView transcript;
    if (result) {
        result = encodeOwnershipReleaseCommitAckTranscript(
            decoded,
            workspace,
            transcript);
    }
    if (result) {
        result = verifySignedDigest(
            signatureVerifier_,
            decoded.serverKeyId,
            decoded.signatureAlgorithm,
            transcript,
            decoded.signature);
    }
    secureZero(MutableByteSpan(receiptDigest, sizeof(receiptDigest)));
    if (result) ack = decoded;
    return result;
}

} // namespace blinker
