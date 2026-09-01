#include "ControllerHmacSha256Initiator.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {
namespace security {

namespace {

bool initiatorNonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

void initiatorWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t initiatorReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

} // namespace

ControllerHmacSha256Initiator::ControllerHmacSha256Initiator(IRandom& random)
    : random_(random),
      credential_(),
      clientNonce_(),
      deviceNonce_(),
      secureSession_(),
      state_(ControllerAuthInitiatorState::Idle) {}

ControllerHmacSha256Initiator::~ControllerHmacSha256Initiator() {
    reset();
}

void ControllerHmacSha256Initiator::clearHandshake() {
    clearControllerCredential(credential_);
    secureZero(MutableByteSpan(clientNonce_, sizeof(clientNonce_)));
    secureZero(MutableByteSpan(deviceNonce_, sizeof(deviceNonce_)));
}

void ControllerHmacSha256Initiator::reset() {
    clearHandshake();
    secureSession_.clear();
    state_ = ControllerAuthInitiatorState::Idle;
}

Result ControllerHmacSha256Initiator::fail(ErrorCode error) {
    reset();
    return Result::failure(error);
}

Result ControllerHmacSha256Initiator::begin(
    const ControllerCredential& credential,
    MutableByteSpan output,
    ByteView& requestPayload) {
    requestPayload = ByteView();
    reset();
    Result result = validateControllerCredential(credential);
    if (!result) return result;
    if (output.data == nullptr ||
        output.size < kControllerAuthInitPayloadSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    credential_ = credential;
    result = random_.fill(MutableByteSpan(
        clientNonce_, sizeof(clientNonce_)));
    if (!result) return fail(result.code());
    if (!initiatorNonZero(ByteView(clientNonce_, sizeof(clientNonce_)))) {
        return fail(ErrorCode::ProtocolError);
    }

    output.data[0] = ControllerAuthPayloadInit;
    memcpy(output.data + 1U, credential_.controllerId, kControllerIdSize);
    output.data[1U + kControllerIdSize] =
        static_cast<uint8_t>(credential_.domain);
    initiatorWriteU32(
        output.data + 1U + kControllerIdSize + 1U,
        credential_.ownershipGeneration);
    initiatorWriteU32(
        output.data + 1U + kControllerIdSize + 1U + 4U,
        credential_.credentialVersion);
    memcpy(
        output.data + 1U + kControllerIdSize + 1U + 8U,
        clientNonce_,
        sizeof(clientNonce_));
    requestPayload = ByteView(output.data, kControllerAuthInitPayloadSize);
    state_ = ControllerAuthInitiatorState::AwaitingChallenge;
    return Result::success();
}

Result ControllerHmacSha256Initiator::handleChallenge(
    const bbp2::AuthResultBody& resultBody,
    MutableByteSpan output,
    ByteView& requestPayload) {
    requestPayload = ByteView();
    if (state_ != ControllerAuthInitiatorState::AwaitingChallenge) {
        return fail(ErrorCode::SequenceConflict);
    }
    if (resultBody.method != kControllerHmacSha256Method) {
        return fail(ErrorCode::UnsupportedFeature);
    }
    if (resultBody.status == bbp2::AuthStatus::Rejected ||
        resultBody.status == bbp2::AuthStatus::Locked) {
        return fail(ErrorCode::AuthenticationRequired);
    }
    if (resultBody.status != bbp2::AuthStatus::Continue ||
        !resultBody.hasPayload ||
        resultBody.payload.data == nullptr ||
        resultBody.payload.size != kControllerAuthChallengePayloadSize ||
        resultBody.payload.data[0] != ControllerAuthPayloadChallenge) {
        return fail(ErrorCode::InvalidEncoding);
    }
    if (output.data == nullptr ||
        output.size < kControllerAuthProofPayloadSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    const ByteView deviceNonce(
        resultBody.payload.data + 1U,
        kControllerAuthNonceSize);
    const uint32_t permissions = initiatorReadU32(
        resultBody.payload.data + 1U + kControllerAuthNonceSize);
    const ByteView deviceProof(
        resultBody.payload.data + 1U + kControllerAuthNonceSize + 4U,
        kControllerAuthProofSize);
    if (!initiatorNonZero(deviceNonce) ||
        permissions != credential_.permissions) {
        return fail(ErrorCode::AuthenticationRequired);
    }

    uint8_t expected[kControllerAuthProofSize];
    Result result = computeControllerAuthProof(
        credential_.authenticationSecret(),
        credential_.id(),
        credential_.domain,
        credential_.ownershipGeneration,
        credential_.credentialVersion,
        permissions,
        ByteView(clientNonce_, sizeof(clientNonce_)),
        deviceNonce,
        ControllerAuthProofRole::Device,
        MutableByteSpan(expected, sizeof(expected)));
    const bool matches = result && constantTimeEqual(
        ByteView(expected, sizeof(expected)), deviceProof);
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    if (!result) return fail(result.code());
    if (!matches) return fail(ErrorCode::AuthenticationRequired);

    memcpy(deviceNonce_, deviceNonce.data, sizeof(deviceNonce_));
    output.data[0] = ControllerAuthPayloadProof;
    result = computeControllerAuthProof(
        credential_.authenticationSecret(),
        credential_.id(),
        credential_.domain,
        credential_.ownershipGeneration,
        credential_.credentialVersion,
        permissions,
        ByteView(clientNonce_, sizeof(clientNonce_)),
        ByteView(deviceNonce_, sizeof(deviceNonce_)),
        ControllerAuthProofRole::App,
        MutableByteSpan(output.data + 1U, kControllerAuthProofSize));
    if (!result) {
        secureZero(MutableByteSpan(
            output.data, kControllerAuthProofPayloadSize));
        return fail(result.code());
    }
    requestPayload = ByteView(output.data, kControllerAuthProofPayloadSize);
    state_ = ControllerAuthInitiatorState::AwaitingAuthorization;
    return Result::success();
}

Result ControllerHmacSha256Initiator::handleAuthorization(
    const bbp2::AuthResultBody& resultBody) {
    if (state_ != ControllerAuthInitiatorState::AwaitingAuthorization) {
        return fail(ErrorCode::SequenceConflict);
    }
    if (resultBody.method != kControllerHmacSha256Method) {
        return fail(ErrorCode::UnsupportedFeature);
    }
    if (resultBody.status == bbp2::AuthStatus::Rejected ||
        resultBody.status == bbp2::AuthStatus::Locked) {
        return fail(ErrorCode::AuthenticationRequired);
    }
    if (resultBody.status != bbp2::AuthStatus::Authorized ||
        resultBody.hasPayload || !resultBody.payload.empty()) {
        return fail(ErrorCode::InvalidEncoding);
    }

    DirectSecureContext context;
    context.controllerId = credential_.id();
    context.domain = credential_.domain;
    context.ownershipGeneration = credential_.ownershipGeneration;
    context.credentialVersion = credential_.credentialVersion;
    context.permissions = credential_.permissions;
    context.clientNonce = ByteView(clientNonce_, sizeof(clientNonce_));
    context.deviceNonce = ByteView(deviceNonce_, sizeof(deviceNonce_));
    Result result = secureSession_.begin(
        DirectSecureRole::Initiator,
        credential_.authenticationSecret(),
        context);
    if (!result) return fail(result.code());

    clearHandshake();
    state_ = ControllerAuthInitiatorState::Authorized;
    return Result::success();
}

} // namespace security
} // namespace blinker
