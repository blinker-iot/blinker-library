#include "NoiseNn.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {
namespace security {

namespace {

const uint8_t kProtocolNameNn[] = {
    'N', 'o', 'i', 's', 'e', '_', 'N', 'N', '_',
    '2', '5', '5', '1', '9', '_', 'A', 'E', 'S', 'G', 'C', 'M', '_',
    'S', 'H', 'A', '2', '5', '6'
};

const uint8_t kProtocolNameNnPsk0[] = {
    'N', 'o', 'i', 's', 'e', '_', 'N', 'N', 'p', 's', 'k', '0', '_',
    '2', '5', '5', '1', '9', '_', 'A', 'E', 'S', 'G', 'C', 'M', '_',
    'S', 'H', 'A', '2', '5', '6'
};

bool validView(ByteView value) {
    return value.data != nullptr || value.empty();
}

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

void writeNoiseNonce(uint64_t value, uint8_t output[kNoiseNonceSize]) {
    memset(output, 0, kNoiseNonceSize);
    for (uint8_t index = 0U; index < 8U; ++index) {
        output[11U - index] = static_cast<uint8_t>(value >> (index * 8U));
    }
}

Result noiseHkdf(
    ByteView chainingKey,
    ByteView input,
    uint8_t* outputs,
    size_t outputCount) {
    if (chainingKey.size != kNoiseKeySize || !validView(input) ||
        outputs == nullptr || (outputCount != 2U && outputCount != 3U)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return hkdfSha256(
        chainingKey,
        input,
        ByteView(),
        MutableByteSpan(outputs, outputCount * kNoiseKeySize));
}

} // namespace

NoiseNnSession::NoiseNnSession(
    INoiseCryptoProvider& crypto,
    IRandom& random)
    : crypto_(crypto),
      random_(random),
      role_(NoiseRole::Responder),
      state_(NoiseSessionState::Empty),
      pattern_(noise::NoiseNnPattern::Nn),
      chainingKey_(),
      handshakeHash_(),
      handshakeKey_(),
      handshakeNonce_(0U),
      localPrivate_(),
      remotePublic_(),
      sendKey_(),
      receiveKey_(),
      sendNonce_(0U),
      receiveNonce_(0U),
      handshakeHasKey_(false) {}

NoiseNnSession::~NoiseNnSession() {
    clear();
}

Result NoiseNnSession::begin(
    NoiseRole role,
    noise::NoiseNnPattern pattern,
    ByteView psk,
    ByteView prologue) {
    clear();
    const bool validRole =
        role == NoiseRole::Initiator || role == NoiseRole::Responder;
    const bool validPsk = noise::noiseNnPatternRequiresPsk(pattern)
                              ? psk.size == kNoiseKeySize && nonZero(psk)
                              : psk.empty();
    if (!validRole || !noise::validNoiseNnPattern(pattern) || !validPsk ||
        !validView(prologue)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    role_ = role;
    pattern_ = pattern;
    Result result = initializeSymmetric();
    if (result) result = mixHash(prologue);
    if (result && noise::noiseNnPatternRequiresPsk(pattern_)) {
        result = mixKeyAndHash(psk);
    }
    if (!result) return fail(result);
    state_ = role == NoiseRole::Initiator
                 ? NoiseSessionState::WriteInitiatorHandshake
                 : NoiseSessionState::ReadInitiatorHandshake;
    return Result::success();
}

void NoiseNnSession::clear() {
    clearSecrets();
    role_ = NoiseRole::Responder;
    state_ = NoiseSessionState::Empty;
    pattern_ = noise::NoiseNnPattern::Nn;
}

Result NoiseNnSession::initializeSymmetric() {
    static_assert(
        sizeof(kProtocolNameNn) <= kNoiseKeySize &&
            sizeof(kProtocolNameNnPsk0) <= kNoiseKeySize,
        "Noise protocol names no longer use the padded initialization rule");
    const uint8_t* name = pattern_ == noise::NoiseNnPattern::Nn
                              ? kProtocolNameNn
                              : kProtocolNameNnPsk0;
    const size_t nameSize = pattern_ == noise::NoiseNnPattern::Nn
                                ? sizeof(kProtocolNameNn)
                                : sizeof(kProtocolNameNnPsk0);
    memset(handshakeHash_, 0, sizeof(handshakeHash_));
    memcpy(handshakeHash_, name, nameSize);
    memcpy(chainingKey_, handshakeHash_, sizeof(chainingKey_));
    handshakeHasKey_ = false;
    handshakeNonce_ = 0U;
    return Result::success();
}

Result NoiseNnSession::mixHash(ByteView input) {
    if (!validView(input)) return Result::failure(ErrorCode::InvalidArgument);
    uint8_t mixed[kNoiseKeySize] = {};
    Sha256 hash;
    Result result = hash.update(ByteView(handshakeHash_, sizeof(handshakeHash_)));
    if (result) result = hash.update(input);
    if (result) result = hash.finish(MutableByteSpan(mixed, sizeof(mixed)));
    if (result) memcpy(handshakeHash_, mixed, sizeof(handshakeHash_));
    secureZero(MutableByteSpan(mixed, sizeof(mixed)));
    return result;
}

Result NoiseNnSession::mixKey(ByteView input) {
    uint8_t outputs[kNoiseKeySize * 2U] = {};
    Result result = noiseHkdf(
        ByteView(chainingKey_, sizeof(chainingKey_)), input, outputs, 2U);
    if (result) {
        memcpy(chainingKey_, outputs, kNoiseKeySize);
        memcpy(handshakeKey_, outputs + kNoiseKeySize, kNoiseKeySize);
        handshakeNonce_ = 0U;
        handshakeHasKey_ = true;
    }
    secureZero(MutableByteSpan(outputs, sizeof(outputs)));
    return result;
}

Result NoiseNnSession::mixKeyAndHash(ByteView input) {
    uint8_t outputs[kNoiseKeySize * 3U] = {};
    Result result = noiseHkdf(
        ByteView(chainingKey_, sizeof(chainingKey_)), input, outputs, 3U);
    if (result) memcpy(chainingKey_, outputs, kNoiseKeySize);
    if (result) {
        result = mixHash(ByteView(outputs + kNoiseKeySize, kNoiseKeySize));
    }
    if (result) {
        memcpy(handshakeKey_, outputs + kNoiseKeySize * 2U, kNoiseKeySize);
        handshakeNonce_ = 0U;
        handshakeHasKey_ = true;
    }
    secureZero(MutableByteSpan(outputs, sizeof(outputs)));
    return result;
}

Result NoiseNnSession::handshakeMessageSize(
    size_t payloadSize,
    size_t& messageSize) const {
    messageSize = 0U;
    const bool initiatorMessage =
        state_ == NoiseSessionState::WriteInitiatorHandshake;
    const bool responderMessage =
        state_ == NoiseSessionState::WriteResponderHandshake;
    if (!initiatorMessage && !responderMessage) {
        return Result::failure(ErrorCode::ProtocolError);
    }
    const size_t tagSize =
        responderMessage || noise::noiseNnPatternRequiresPsk(pattern_)
            ? kNoiseTagSize
            : 0U;
    const size_t overhead = kNoiseDhSize + tagSize;
    if (payloadSize > SIZE_MAX - overhead) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    messageSize = overhead + payloadSize;
    return Result::success();
}

Result NoiseNnSession::writeHandshake(
    ByteView payload,
    MutableByteSpan output,
    ByteView& message) {
    message = ByteView();
    const bool initiatorMessage =
        state_ == NoiseSessionState::WriteInitiatorHandshake;
    const bool responderMessage =
        state_ == NoiseSessionState::WriteResponderHandshake;
    if (!validView(payload)) return Result::failure(ErrorCode::InvalidArgument);
    size_t required = 0U;
    Result result = handshakeMessageSize(payload.size, required);
    if (!result) return result.code() == ErrorCode::ProtocolError
                            ? fail(result)
                            : result;
    if (required > UINT16_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    result = generateEphemeral(MutableByteSpan(output.data, kNoiseDhSize));
    if (result) result = mixEphemeral(ByteView(output.data, kNoiseDhSize));
    if (result && responderMessage) result = mixDh();
    ByteView encodedPayload;
    if (result) {
        result = encryptAndHash(
            payload,
            MutableByteSpan(
                output.data + kNoiseDhSize,
                output.size - kNoiseDhSize),
            encodedPayload);
    }
    if (!result) return fail(result);
    if (encodedPayload.size != required - kNoiseDhSize) {
        return fail(Result::failure(ErrorCode::InternalError));
    }
    message = ByteView(output.data, required);
    if (initiatorMessage) {
        state_ = NoiseSessionState::ReadResponderHandshake;
        return Result::success();
    }
    result = split();
    if (!result) {
        message = ByteView();
        return fail(result);
    }
    return Result::success();
}

Result NoiseNnSession::readHandshake(
    ByteView message,
    MutableByteSpan payloadOutput,
    ByteView& payload) {
    payload = ByteView();
    const bool initiatorMessage =
        state_ == NoiseSessionState::ReadInitiatorHandshake;
    const bool responderMessage =
        state_ == NoiseSessionState::ReadResponderHandshake;
    if (!initiatorMessage && !responderMessage) {
        return fail(Result::failure(ErrorCode::ProtocolError));
    }
    const size_t tagSize =
        responderMessage || noise::noiseNnPatternRequiresPsk(pattern_)
            ? kNoiseTagSize
            : 0U;
    const size_t overhead = kNoiseDhSize + tagSize;
    if (message.data == nullptr || message.size < overhead) {
        return fail(Result::failure(ErrorCode::TruncatedInput));
    }
    if (message.size > UINT16_MAX) {
        return fail(Result::failure(ErrorCode::CapacityExceeded));
    }
    const size_t plaintextSize = message.size - overhead;
    if (plaintextSize != 0U &&
        (payloadOutput.data == nullptr || payloadOutput.size < plaintextSize)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (!nonZero(ByteView(message.data, kNoiseDhSize))) {
        return fail(Result::failure(ErrorCode::AuthenticationRequired));
    }
    memcpy(remotePublic_, message.data, sizeof(remotePublic_));
    Result result = mixEphemeral(ByteView(remotePublic_, sizeof(remotePublic_)));
    if (result && responderMessage) result = mixDh();
    if (result) {
        result = decryptAndHash(
            ByteView(message.data + kNoiseDhSize, message.size - kNoiseDhSize),
            payloadOutput,
            payload);
    }
    if (!result) return fail(result);
    if (initiatorMessage) {
        state_ = NoiseSessionState::WriteResponderHandshake;
        return Result::success();
    }
    result = split();
    if (!result) {
        payload = ByteView();
        return fail(result);
    }
    return Result::success();
}

Result NoiseNnSession::encryptTransport(
    ByteView plaintext,
    MutableByteSpan output,
    ByteView& ciphertext) {
    ciphertext = ByteView();
    if (state_ != NoiseSessionState::Transport) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!validView(plaintext)) return Result::failure(ErrorCode::InvalidArgument);
    if (plaintext.size > SIZE_MAX - kNoiseTagSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    const size_t required = plaintext.size + kNoiseTagSize;
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = encryptWithKey(
        ByteView(sendKey_, sizeof(sendKey_)), sendNonce_, ByteView(),
        plaintext, output, ciphertext);
    return result ? result : fail(result);
}

Result NoiseNnSession::decryptTransport(
    ByteView ciphertext,
    MutableByteSpan output,
    ByteView& plaintext) {
    plaintext = ByteView();
    if (state_ != NoiseSessionState::Transport) {
        return fail(Result::failure(ErrorCode::ProtocolError));
    }
    if (ciphertext.data == nullptr) {
        return fail(Result::failure(ErrorCode::InvalidArgument));
    }
    if (ciphertext.size < kNoiseTagSize) {
        return fail(Result::failure(ErrorCode::TruncatedInput));
    }
    const size_t required = ciphertext.size - kNoiseTagSize;
    if (required != 0U &&
        (output.data == nullptr || output.size < required)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = decryptWithKey(
        ByteView(receiveKey_, sizeof(receiveKey_)), receiveNonce_, ByteView(),
        ciphertext, output, plaintext);
    return result ? result : fail(result);
}

Result NoiseNnSession::handshakeHash(ByteView& output) const {
    output = ByteView();
    if (state_ != NoiseSessionState::Transport) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    output = ByteView(handshakeHash_, sizeof(handshakeHash_));
    return Result::success();
}

Result NoiseNnSession::mixEphemeral(ByteView publicKey) {
    Result result = mixHash(publicKey);
    if (result && noise::noiseNnPatternRequiresPsk(pattern_)) {
        result = mixKey(publicKey);
    }
    return result;
}

Result NoiseNnSession::encryptAndHash(
    ByteView plaintext,
    MutableByteSpan output,
    ByteView& ciphertext) {
    ciphertext = ByteView();
    Result result;
    if (handshakeHasKey_) {
        result = encryptWithKey(
            ByteView(handshakeKey_, sizeof(handshakeKey_)),
            handshakeNonce_,
            ByteView(handshakeHash_, sizeof(handshakeHash_)),
            plaintext,
            output,
            ciphertext);
    } else {
        if (output.size < plaintext.size ||
            (!plaintext.empty() && output.data == nullptr)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
        if (!plaintext.empty()) memmove(output.data, plaintext.data, plaintext.size);
        ciphertext = ByteView(output.data, plaintext.size);
        result = Result::success();
    }
    if (result) result = mixHash(ciphertext);
    return result;
}

Result NoiseNnSession::decryptAndHash(
    ByteView ciphertext,
    MutableByteSpan output,
    ByteView& plaintext) {
    plaintext = ByteView();
    Result result;
    if (handshakeHasKey_) {
        result = decryptWithKey(
            ByteView(handshakeKey_, sizeof(handshakeKey_)),
            handshakeNonce_,
            ByteView(handshakeHash_, sizeof(handshakeHash_)),
            ciphertext,
            output,
            plaintext);
    } else {
        if (output.size < ciphertext.size ||
            (!ciphertext.empty() && output.data == nullptr)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
        if (!ciphertext.empty()) {
            memmove(output.data, ciphertext.data, ciphertext.size);
        }
        plaintext = ByteView(output.data, ciphertext.size);
        result = Result::success();
    }
    if (result) result = mixHash(ciphertext);
    return result;
}

Result NoiseNnSession::split() {
    uint8_t outputs[kNoiseKeySize * 2U] = {};
    Result result = noiseHkdf(
        ByteView(chainingKey_, sizeof(chainingKey_)), ByteView(), outputs, 2U);
    if (result) {
        const uint8_t* initiatorToResponder = outputs;
        const uint8_t* responderToInitiator = outputs + kNoiseKeySize;
        const uint8_t* send = role_ == NoiseRole::Initiator
                                  ? initiatorToResponder
                                  : responderToInitiator;
        const uint8_t* receive = role_ == NoiseRole::Initiator
                                     ? responderToInitiator
                                     : initiatorToResponder;
        memcpy(sendKey_, send, sizeof(sendKey_));
        memcpy(receiveKey_, receive, sizeof(receiveKey_));
        sendNonce_ = 0U;
        receiveNonce_ = 0U;
        secureZero(MutableByteSpan(chainingKey_, sizeof(chainingKey_)));
        secureZero(MutableByteSpan(handshakeKey_, sizeof(handshakeKey_)));
        secureZero(MutableByteSpan(localPrivate_, sizeof(localPrivate_)));
        secureZero(MutableByteSpan(remotePublic_, sizeof(remotePublic_)));
        handshakeNonce_ = 0U;
        handshakeHasKey_ = false;
        state_ = NoiseSessionState::Transport;
    }
    secureZero(MutableByteSpan(outputs, sizeof(outputs)));
    return result;
}

Result NoiseNnSession::generateEphemeral(MutableByteSpan publicKey) {
    if (publicKey.data == nullptr || publicKey.size < kNoiseDhSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = random_.fill(MutableByteSpan(localPrivate_, sizeof(localPrivate_)));
    if (!result) return result;
    localPrivate_[0] &= 248U;
    localPrivate_[31] &= 127U;
    localPrivate_[31] |= 64U;
    result = crypto_.x25519PublicKey(
        ByteView(localPrivate_, sizeof(localPrivate_)),
        MutableByteSpan(publicKey.data, kNoiseDhSize));
    if (result && !nonZero(ByteView(publicKey.data, kNoiseDhSize))) {
        result = Result::failure(ErrorCode::InternalError);
    }
    return result;
}

Result NoiseNnSession::mixDh() {
    uint8_t shared[kNoiseDhSize] = {};
    Result result = crypto_.x25519(
        ByteView(localPrivate_, sizeof(localPrivate_)),
        ByteView(remotePublic_, sizeof(remotePublic_)),
        MutableByteSpan(shared, sizeof(shared)));
    if (result && !nonZero(ByteView(shared, sizeof(shared)))) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result) result = mixKey(ByteView(shared, sizeof(shared)));
    secureZero(MutableByteSpan(shared, sizeof(shared)));
    return result;
}

Result NoiseNnSession::encryptWithKey(
    ByteView key,
    uint64_t& nonce,
    ByteView associatedData,
    ByteView plaintext,
    MutableByteSpan output,
    ByteView& ciphertext) {
    ciphertext = ByteView();
    if (key.size != kNoiseKeySize || !nonZero(key) ||
        !validView(associatedData) || !validView(plaintext)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (nonce == UINT64_MAX) return Result::failure(ErrorCode::CapacityExceeded);
    if (plaintext.size > SIZE_MAX - kNoiseTagSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    const size_t required = plaintext.size + kNoiseTagSize;
    if (output.size < required || (required != 0U && output.data == nullptr)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    uint8_t encodedNonce[kNoiseNonceSize] = {};
    writeNoiseNonce(nonce, encodedNonce);
    size_t written = 0U;
    Result result = crypto_.aes256GcmEncrypt(
        key, ByteView(encodedNonce, sizeof(encodedNonce)), associatedData,
        plaintext, output, written);
    secureZero(MutableByteSpan(encodedNonce, sizeof(encodedNonce)));
    if (!result) return result;
    if (written != required) return Result::failure(ErrorCode::InternalError);
    ++nonce;
    ciphertext = ByteView(output.data, written);
    return Result::success();
}

Result NoiseNnSession::decryptWithKey(
    ByteView key,
    uint64_t& nonce,
    ByteView associatedData,
    ByteView ciphertext,
    MutableByteSpan output,
    ByteView& plaintext) {
    plaintext = ByteView();
    if (key.size != kNoiseKeySize || !nonZero(key) ||
        !validView(associatedData) || ciphertext.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (nonce == UINT64_MAX) return Result::failure(ErrorCode::CapacityExceeded);
    if (ciphertext.size < kNoiseTagSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    const size_t required = ciphertext.size - kNoiseTagSize;
    if (output.size < required ||
        (required != 0U && output.data == nullptr)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    uint8_t encodedNonce[kNoiseNonceSize] = {};
    writeNoiseNonce(nonce, encodedNonce);
    size_t written = 0U;
    Result result = crypto_.aes256GcmDecrypt(
        key, ByteView(encodedNonce, sizeof(encodedNonce)), associatedData,
        ciphertext, output, written);
    secureZero(MutableByteSpan(encodedNonce, sizeof(encodedNonce)));
    if (!result) return result;
    if (written != required) return Result::failure(ErrorCode::InternalError);
    ++nonce;
    plaintext = ByteView(output.data, written);
    return Result::success();
}

Result NoiseNnSession::fail(Result result) {
    clearSecrets();
    state_ = NoiseSessionState::Failed;
    return result;
}

void NoiseNnSession::clearSecrets() {
    secureZero(MutableByteSpan(chainingKey_, sizeof(chainingKey_)));
    secureZero(MutableByteSpan(handshakeHash_, sizeof(handshakeHash_)));
    secureZero(MutableByteSpan(handshakeKey_, sizeof(handshakeKey_)));
    secureZero(MutableByteSpan(localPrivate_, sizeof(localPrivate_)));
    secureZero(MutableByteSpan(remotePublic_, sizeof(remotePublic_)));
    secureZero(MutableByteSpan(sendKey_, sizeof(sendKey_)));
    secureZero(MutableByteSpan(receiveKey_, sizeof(receiveKey_)));
    handshakeNonce_ = 0U;
    sendNonce_ = 0U;
    receiveNonce_ = 0U;
    handshakeHasKey_ = false;
}

} // namespace security
} // namespace blinker
