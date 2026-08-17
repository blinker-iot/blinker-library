#include "EnrollmentTicketVerifier.h"

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool exactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected)
        return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool sameBytes(ByteView first, ByteView second) {
    return first.size == second.size && constantTimeEqual(first, second);
}

bool validContext(const EnrollmentTicketVerificationContext& context) {
    return exactNonZero(context.deviceInstanceId, kDeviceInstanceIdSize) &&
           exactNonZero(context.setupSessionId,
                        kEnrollmentTicketSetupSessionIdSize) &&
           exactNonZero(context.setupTranscriptHash,
                        kEnrollmentTicketSetupTranscriptHashSize) &&
           context.ownershipGeneration != 0U &&
           exactNonZero(context.controllerId, kControllerIdSize) &&
           exactNonZero(context.controllerSecretDigest,
                        kEnrollmentTicketControllerSecretDigestSize) &&
           validAuthorizationPermissions(context.controllerPermissions) &&
           (context.authorizedOperations == 0U ||
            context.authorizedOperations == kEnrollmentTicketAuthorizeAll) &&
           (!context.hasTrustedTime || context.nowEpochSeconds != 0U);
}

} // namespace

EnrollmentTicketVerificationContext::EnrollmentTicketVerificationContext()
    : ownershipGeneration(0U), controllerPermissions(0U),
      authorizedOperations(0U), nowEpochSeconds(0U), hasTrustedTime(false) {}

Result EnrollmentTicketVerifier::verify(
    ByteView encodedTicket,
    const EnrollmentTicketVerificationContext& context,
    MutableByteSpan transcriptWorkspace,
    EnrollmentTicket& ticket) {
    ticket = EnrollmentTicket();
    if (!validContext(context) || transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kEnrollmentTicketWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(transcriptWorkspace);

    EnrollmentTicket decoded;
    Result result = decodeEnrollmentTicket(encodedTicket, decoded);
    if (!result)
        return result;
    if (!sameBytes(decoded.deviceInstanceId, context.deviceInstanceId) ||
        !sameBytes(decoded.setupSessionId, context.setupSessionId) ||
        !sameBytes(decoded.setupTranscriptHash, context.setupTranscriptHash) ||
        decoded.ownershipGeneration != context.ownershipGeneration ||
        !sameBytes(decoded.controllerId, context.controllerId) ||
        !sameBytes(decoded.controllerSecretDigest,
                   context.controllerSecretDigest) ||
        decoded.controllerPermissions != context.controllerPermissions ||
        decoded.authorizedOperations != context.authorizedOperations) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (context.hasTrustedTime &&
        (context.nowEpochSeconds < decoded.issuedAt ||
         context.nowEpochSeconds >= decoded.expiresAt)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    ByteView transcript;
    result = encodeEnrollmentTicketTranscript(
        decoded, transcriptWorkspace, transcript);
    uint8_t digest[kSha256Size] = {};
    if (result) {
        result = sha256(transcript, MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        result =
            signatureVerifier_.verifyDigest(decoded.serverKeyId,
                                            decoded.signatureAlgorithm,
                                            ByteView(digest, sizeof(digest)),
                                            decoded.signature);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(transcriptWorkspace);
    if (result)
        ticket = decoded;
    return result;
}

} // namespace blinker
