#include "DirectSecureSession.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../protocol/bbp2/Frame.h"
#include "ControllerHmacSha256Authorizer.h"

namespace blinker {
namespace security {

static_assert(
    kDirectSecureAuthNonceSize == kControllerAuthNonceSize,
    "Direct secure transcript must match Method 2 nonce size");

namespace {

const uint8_t kTranscriptDomain[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'D', 'I', 'R', 'E', 'C', 'T',
    '-', 'A', 'U', 'T', 'H', '/', '1'
};
const uint8_t kKdfInfo[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'D', 'I', 'R', 'E', 'C', 'T',
    '-', 'S', 'E', 'S', 'S', 'I', 'O', 'N', '/', '1'
};
const uint8_t kAadDomain[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'D', 'I', 'R', 'E', 'C', 'T',
    '-', 'R', 'E', 'C', 'O', 'R', 'D', '/', '1'
};

enum : size_t {
    kDirectSecureKeyMaterialSize =
        kAes256KeySize + kAes256KeySize +
        kDirectSecureNoncePrefixSize + kDirectSecureNoncePrefixSize +
        kDirectSecureBindingSize,
    kDirectSecureAadSize =
        sizeof(kAadDomain) + 1U + kDirectSecureBindingSize +
        kDirectSecureHeaderSize
};

bool directNonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

void directWriteU16(uint8_t* output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value >> 8U);
    output[1] = static_cast<uint8_t>(value);
}

uint16_t directReadU16(const uint8_t* input) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(input[0]) << 8U) |
        static_cast<uint16_t>(input[1]));
}

void directWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t directReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool validGeneration(
    ControllerCredentialDomain domain,
    uint32_t generation) {
    return (domain == ControllerCredentialDomain::Local && generation == 0U) ||
           (domain == ControllerCredentialDomain::PlatformAccess &&
            generation != 0U);
}

Result validateContext(
    ByteView controllerSecret,
    const DirectSecureContext& context) {
    if (controllerSecret.size != kControllerCredentialSecretSize ||
        !directNonZero(controllerSecret) ||
        context.controllerId.size != kControllerIdSize ||
        !directNonZero(context.controllerId) ||
        !validGeneration(context.domain, context.ownershipGeneration) ||
        context.credentialVersion == 0U ||
        !validAuthorizationPermissions(context.permissions) ||
        context.clientNonce.size != kDirectSecureAuthNonceSize ||
        !directNonZero(context.clientNonce) ||
        context.deviceNonce.size != kDirectSecureAuthNonceSize ||
        !directNonZero(context.deviceNonce)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

} // namespace

static_assert(
    sizeof(DirectSecureKeyMaterial) == kDirectSecureKeyMaterialSize,
    "Direct secure key material must remain packed");

DirectSecureKeyMaterial::DirectSecureKeyMaterial()
    : initiatorKey(),
      responderKey(),
      initiatorNoncePrefix(),
      responderNoncePrefix(),
      sessionBinding() {}

void DirectSecureKeyMaterial::clear() {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(this),
        sizeof(*this)));
}

size_t directSecureRecordSize(size_t plaintextSize) {
    return plaintextSize <= kDirectSecureMaximumPlaintextSize
               ? kDirectSecureOverhead + plaintextSize
               : 0U;
}

Result decodeDirectSecureRecordSize(ByteView prefix, size_t& recordSize) {
    recordSize = 0U;
    if (prefix.data == nullptr || prefix.size < kDirectSecureHeaderSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (prefix.data[0] != kDirectSecureMagic) {
        return Result::failure(ErrorCode::InvalidMagic);
    }
    const uint8_t version = static_cast<uint8_t>(
        (prefix.data[1] & kDirectSecureVersionMask) >>
        kDirectSecureVersionShift);
    if (version != kDirectSecureVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    if ((prefix.data[1] & kDirectSecureFlagsMask) != 0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    const size_t plaintextSize = directReadU16(prefix.data + 2U);
    if (plaintextSize < bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::InvalidHeader);
    }
    recordSize = directSecureRecordSize(plaintextSize);
    return recordSize != 0U
               ? Result::success()
               : Result::failure(ErrorCode::CapacityExceeded);
}

Result parseDirectSecureRecord(
    ByteView encoded,
    DirectSecureRecordView& record) {
    record = DirectSecureRecordView();
    size_t expected = 0U;
    Result result = decodeDirectSecureRecordSize(encoded, expected);
    if (!result) return result;
    if (encoded.size < expected) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (encoded.size != expected) {
        return Result::failure(ErrorCode::TrailingData);
    }
    record.sequence = directReadU32(encoded.data + 4U);
    record.ciphertextAndTag = ByteView(
        encoded.data + kDirectSecureHeaderSize,
        encoded.size - kDirectSecureHeaderSize);
    return Result::success();
}

Result deriveDirectSecureKeyMaterial(
    ByteView controllerSecret,
    const DirectSecureContext& context,
    DirectSecureKeyMaterial& output) {
    output.clear();
    Result result = validateContext(controllerSecret, context);
    if (!result) return result;

    uint8_t transcriptHash[kSha256Size];
    const uint8_t prefix[] = {
        kControllerAuthTranscriptVersion,
        static_cast<uint8_t>(kControllerHmacSha256Method >> 8U),
        static_cast<uint8_t>(kControllerHmacSha256Method),
        static_cast<uint8_t>(ControllerCredentialSuite::HmacSha256_32)
    };
    uint8_t numbers[13U];
    numbers[0] = static_cast<uint8_t>(context.domain);
    directWriteU32(numbers + 1U, context.ownershipGeneration);
    directWriteU32(numbers + 5U, context.credentialVersion);
    directWriteU32(numbers + 9U, context.permissions);
    Sha256 hash;
    result = hash.update(ByteView(kTranscriptDomain, sizeof(kTranscriptDomain)));
    if (result) result = hash.update(ByteView(prefix, sizeof(prefix)));
    if (result) result = hash.update(context.controllerId);
    if (result) result = hash.update(ByteView(numbers, sizeof(numbers)));
    if (result) result = hash.update(context.clientNonce);
    if (result) result = hash.update(context.deviceNonce);
    if (result) {
        result = hash.finish(MutableByteSpan(
            transcriptHash,
            sizeof(transcriptHash)));
    }
    if (result) {
        result = hkdfSha256(
            ByteView(transcriptHash, sizeof(transcriptHash)),
            controllerSecret,
            ByteView(kKdfInfo, sizeof(kKdfInfo)),
            MutableByteSpan(
                reinterpret_cast<uint8_t*>(&output),
                sizeof(output)));
    }
    secureZero(MutableByteSpan(numbers, sizeof(numbers)));
    secureZero(MutableByteSpan(transcriptHash, sizeof(transcriptHash)));
    if (!result) output.clear();
    return result;
}

DirectSecureSession::DirectSecureSession()
    : role_(DirectSecureRole::Responder),
      sendKey_(),
      receiveKey_(),
      sendNoncePrefix_(),
      receiveNoncePrefix_(),
      sessionBinding_(),
      sendSequence_(0U),
      receiveSequence_(0U),
      sendExhausted_(false),
      receiveExhausted_(false),
      ready_(false) {}

DirectSecureSession::~DirectSecureSession() {
    clear();
}

Result DirectSecureSession::begin(
    DirectSecureRole role,
    ByteView controllerSecret,
    const DirectSecureContext& context) {
    clear();
    if (role != DirectSecureRole::Initiator &&
        role != DirectSecureRole::Responder) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    DirectSecureKeyMaterial material;
    Result result = deriveDirectSecureKeyMaterial(
        controllerSecret,
        context,
        material);
    if (!result) return result;

    role_ = role;
    const bool initiator = role == DirectSecureRole::Initiator;
    memcpy(
        sendKey_,
        initiator ? material.initiatorKey : material.responderKey,
        sizeof(sendKey_));
    memcpy(
        receiveKey_,
        initiator ? material.responderKey : material.initiatorKey,
        sizeof(receiveKey_));
    memcpy(
        sendNoncePrefix_,
        initiator ? material.initiatorNoncePrefix
                  : material.responderNoncePrefix,
        sizeof(sendNoncePrefix_));
    memcpy(
        receiveNoncePrefix_,
        initiator ? material.responderNoncePrefix
                  : material.initiatorNoncePrefix,
        sizeof(receiveNoncePrefix_));
    memcpy(
        sessionBinding_,
        material.sessionBinding,
        sizeof(sessionBinding_));
    material.clear();
    ready_ = true;
    return Result::success();
}

void DirectSecureSession::clear() {
    secureZero(MutableByteSpan(sendKey_, sizeof(sendKey_)));
    secureZero(MutableByteSpan(receiveKey_, sizeof(receiveKey_)));
    secureZero(MutableByteSpan(
        sendNoncePrefix_,
        sizeof(sendNoncePrefix_)));
    secureZero(MutableByteSpan(
        receiveNoncePrefix_,
        sizeof(receiveNoncePrefix_)));
    secureZero(MutableByteSpan(sessionBinding_, sizeof(sessionBinding_)));
    role_ = DirectSecureRole::Responder;
    sendSequence_ = 0U;
    receiveSequence_ = 0U;
    sendExhausted_ = false;
    receiveExhausted_ = false;
    ready_ = false;
}

DirectSecureDirection DirectSecureSession::sendDirection() const {
    return role_ == DirectSecureRole::Initiator
               ? DirectSecureDirection::InitiatorToResponder
               : DirectSecureDirection::ResponderToInitiator;
}

DirectSecureDirection DirectSecureSession::receiveDirection() const {
    return role_ == DirectSecureRole::Initiator
               ? DirectSecureDirection::ResponderToInitiator
               : DirectSecureDirection::InitiatorToResponder;
}

Result DirectSecureSession::buildNonce(
    ByteView prefix,
    uint32_t sequence,
    MutableByteSpan output) const {
    if (prefix.size != kDirectSecureNoncePrefixSize ||
        output.data == nullptr || output.size < kAesGcmNonceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    memcpy(output.data, prefix.data, prefix.size);
    directWriteU32(output.data + prefix.size, sequence);
    return Result::success();
}

Result DirectSecureSession::buildAad(
    DirectSecureDirection direction,
    ByteView header,
    MutableByteSpan output,
    ByteView& aad) const {
    aad = ByteView();
    if (header.data == nullptr || header.size != kDirectSecureHeaderSize ||
        output.data == nullptr || output.size < kDirectSecureAadSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    size_t offset = 0U;
    memcpy(output.data + offset, kAadDomain, sizeof(kAadDomain));
    offset += sizeof(kAadDomain);
    output.data[offset++] = static_cast<uint8_t>(direction);
    memcpy(
        output.data + offset,
        sessionBinding_,
        sizeof(sessionBinding_));
    offset += sizeof(sessionBinding_);
    memcpy(output.data + offset, header.data, header.size);
    offset += header.size;
    aad = ByteView(output.data, offset);
    return Result::success();
}

Result DirectSecureSession::seal(
    IX25519AesGcmCryptoProvider& crypto,
    ByteView plaintext,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (sendExhausted_) return Result::failure(ErrorCode::SequenceConflict);
    if (plaintext.data == nullptr ||
        plaintext.size < bbp2::kBaseHeaderSize ||
        plaintext.size > kDirectSecureMaximumPlaintextSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t required = directSecureRecordSize(plaintext.size);
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    output.data[0] = kDirectSecureMagic;
    output.data[1] = static_cast<uint8_t>(
        kDirectSecureVersion << kDirectSecureVersionShift);
    directWriteU16(output.data + 2U, static_cast<uint16_t>(plaintext.size));
    directWriteU32(output.data + 4U, sendSequence_);

    uint8_t nonce[kAesGcmNonceSize];
    uint8_t aadStorage[kDirectSecureAadSize];
    ByteView aad;
    Result result = buildNonce(
        ByteView(sendNoncePrefix_, sizeof(sendNoncePrefix_)),
        sendSequence_,
        MutableByteSpan(nonce, sizeof(nonce)));
    if (result) {
        result = buildAad(
            sendDirection(),
            ByteView(output.data, kDirectSecureHeaderSize),
            MutableByteSpan(aadStorage, sizeof(aadStorage)),
            aad);
    }
    size_t written = 0U;
    if (result) {
        result = crypto.aes256GcmEncrypt(
            ByteView(sendKey_, sizeof(sendKey_)),
            ByteView(nonce, sizeof(nonce)),
            aad,
            plaintext,
            MutableByteSpan(
                output.data + kDirectSecureHeaderSize,
                output.size - kDirectSecureHeaderSize),
            written);
    }
    secureZero(MutableByteSpan(nonce, sizeof(nonce)));
    secureZero(MutableByteSpan(aadStorage, sizeof(aadStorage)));
    if (!result || written != plaintext.size + kDirectSecureTagSize) {
        if (result) result = Result::failure(ErrorCode::InternalError);
        clear();
        return result;
    }
    encoded = ByteView(output.data, required);
    if (sendSequence_ == UINT32_MAX) {
        sendExhausted_ = true;
    } else {
        ++sendSequence_;
    }
    return Result::success();
}

Result DirectSecureSession::open(
    IX25519AesGcmCryptoProvider& crypto,
    ByteView encoded,
    MutableByteSpan output,
    ByteView& plaintext) {
    plaintext = ByteView();
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (receiveExhausted_) {
        clear();
        return Result::failure(ErrorCode::SequenceConflict);
    }
    DirectSecureRecordView record;
    Result result = parseDirectSecureRecord(encoded, record);
    if (!result) {
        clear();
        return result;
    }
    const size_t plaintextSize =
        record.ciphertextAndTag.size - kDirectSecureTagSize;
    if (output.data == nullptr || output.size < plaintextSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (record.sequence != receiveSequence_) {
        clear();
        return Result::failure(ErrorCode::SequenceConflict);
    }

    uint8_t nonce[kAesGcmNonceSize];
    uint8_t aadStorage[kDirectSecureAadSize];
    ByteView aad;
    result = buildNonce(
        ByteView(receiveNoncePrefix_, sizeof(receiveNoncePrefix_)),
        record.sequence,
        MutableByteSpan(nonce, sizeof(nonce)));
    if (result) {
        result = buildAad(
            receiveDirection(),
            ByteView(encoded.data, kDirectSecureHeaderSize),
            MutableByteSpan(aadStorage, sizeof(aadStorage)),
            aad);
    }
    size_t written = 0U;
    if (result) {
        result = crypto.aes256GcmDecrypt(
            ByteView(receiveKey_, sizeof(receiveKey_)),
            ByteView(nonce, sizeof(nonce)),
            aad,
            record.ciphertextAndTag,
            output,
            written);
    }
    secureZero(MutableByteSpan(nonce, sizeof(nonce)));
    secureZero(MutableByteSpan(aadStorage, sizeof(aadStorage)));
    if (!result || written != plaintextSize) {
        if (result) result = Result::failure(ErrorCode::InternalError);
        clear();
        return result;
    }
    plaintext = ByteView(output.data, written);
    if (receiveSequence_ == UINT32_MAX) {
        receiveExhausted_ = true;
    } else {
        ++receiveSequence_;
    }
    return Result::success();
}

} // namespace security
} // namespace blinker
