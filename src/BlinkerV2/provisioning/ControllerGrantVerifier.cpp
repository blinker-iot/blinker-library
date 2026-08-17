#include "ControllerGrantVerifier.h"

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

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

bool sameText(StringView first, StringView second) {
    return sameBytes(
        ByteView(
            reinterpret_cast<const uint8_t*>(first.data),
            first.size),
        ByteView(
            reinterpret_cast<const uint8_t*>(second.data),
            second.size));
}

bool validContext(const ControllerGrantVerificationContext& context) {
    return exactNonZero(context.deviceInstanceId, kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(context.logicalDeviceId).ok() &&
           context.ownershipGeneration != 0U &&
           exactNonZero(
               context.controlNonce,
               kControllerControlNonceSize) &&
           (!context.hasTrustedTime || context.nowEpochSeconds != 0U);
}

} // namespace

Result ControllerGrantVerifier::verify(
    ByteView encodedGrant,
    ByteView controllerSecret,
    const ControllerGrantVerificationContext& context,
    MutableByteSpan transcriptWorkspace,
    ControllerGrant& grant) {
    grant = ControllerGrant();
    if (!validContext(context) || transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kControllerControlWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    ControllerGrant decoded;
    Result result = decodeControllerGrant(encodedGrant, decoded);
    if (!result) return result;
    if (!sameBytes(decoded.deviceInstanceId, context.deviceInstanceId) ||
        !sameText(decoded.logicalDeviceId, context.logicalDeviceId) ||
        decoded.ownershipGeneration != context.ownershipGeneration ||
        !sameBytes(decoded.controlNonce, context.controlNonce)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (context.hasTrustedTime &&
        (context.nowEpochSeconds < decoded.issuedAt ||
         context.nowEpochSeconds >= decoded.expiresAt)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    uint8_t secretDigest[kControllerSecretDigestSize] = {};
    if (decoded.operation == ControllerMutationOperation::Revoke) {
        if (!controllerSecret.empty()) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    } else {
        if (!exactNonZero(
                controllerSecret,
                kControllerCredentialSecretSize)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        result = sha256(
            controllerSecret,
            MutableByteSpan(secretDigest, sizeof(secretDigest)));
        if (result && !constantTimeEqual(
                ByteView(secretDigest, sizeof(secretDigest)),
                decoded.secretDigest)) {
            result = Result::failure(ErrorCode::AuthenticationRequired);
        }
    }
    secureZero(MutableByteSpan(secretDigest, sizeof(secretDigest)));
    if (!result) return result;

    secureZero(transcriptWorkspace);
    ByteView transcript;
    result = encodeControllerGrantTranscript(
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
    if (!result) return result;
    grant = decoded;
    return Result::success();
}

} // namespace blinker
