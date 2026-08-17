#include "CloudCredentialEnrollmentGrantVerifier.h"

#include <string.h>

#include "../core/SecureMemory.h"

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

bool same(ByteView first, ByteView second) {
    return first.size == second.size &&
           constantTimeEqual(first, second);
}

bool same(StringView first, StringView second) {
    return first.size == second.size && first.data != nullptr &&
           second.data != nullptr &&
           memcmp(first.data, second.data, first.size) == 0;
}

bool validContext(
    const CloudCredentialEnrollmentVerificationContext& context) {
    return exactNonZero(
               context.requestId,
               kCloudEnrollmentRequestIdSize) &&
           exactNonZero(
               context.deviceInstanceId,
               kDeviceInstanceIdSize) &&
           validateLogicalDeviceId(context.logicalDeviceId).ok() &&
           context.ownershipGeneration != 0U &&
           context.credentialVersion != 0U &&
           exactNonZero(context.requestDigest, kSha256Size) &&
           (!context.hasTrustedTime || context.nowEpochSeconds != 0U);
}

} // namespace

Result CloudCredentialEnrollmentGrantVerifier::verify(
    ByteView encodedGrant,
    const CloudCredentialEnrollmentVerificationContext& context,
    MutableByteSpan transcriptWorkspace,
    CloudCredentialEnrollmentGrant& grant) {
    grant = CloudCredentialEnrollmentGrant();
    if (!validContext(context) || transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kCloudEnrollmentWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    CloudCredentialEnrollmentGrant decoded;
    Result result = decodeCloudCredentialEnrollmentGrant(
        encodedGrant,
        decoded);
    if (!result) return result;
    if (!same(decoded.requestId, context.requestId) ||
        !same(decoded.deviceInstanceId, context.deviceInstanceId) ||
        !same(decoded.logicalDeviceId, context.logicalDeviceId) ||
        decoded.ownershipGeneration != context.ownershipGeneration ||
        decoded.credentialVersion != context.credentialVersion ||
        !same(decoded.requestDigest, context.requestDigest)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (context.hasTrustedTime &&
        (context.nowEpochSeconds < decoded.issuedAt ||
         context.nowEpochSeconds >= decoded.expiresAt)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    ByteView transcript;
    result = encodeCloudCredentialEnrollmentGrantTranscript(
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

} // namespace blinker
