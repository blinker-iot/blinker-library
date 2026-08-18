#include "DeviceKeySessionContract.h"

#include <string.h>

#include "../core/HmacSha256.h"
#include "../core/SecureMemory.h"
#include "../core/Utf8.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {

namespace {

static const char kProofDomain[] =
    "blinker/device-key/session-proof/v1";
static const char kChallengePath[] = "/api/v2/device-auth/challenges";
static const char kSessionPath[] = "/api/v2/device-sessions";

cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = kDeviceKeyMaximumHostSize;
    value.maxByteStringLength = kDeviceKeyProofSize;
    value.maxContainerItems = 15U;
    value.maxDepth = 2U;
    return value;
}

bool exactBytes(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
}

bool nonZeroBytes(ByteView value, size_t expected) {
    if (!exactBytes(value, expected)) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validText(StringView value, size_t maximum) {
    if (value.data == nullptr || value.empty() || value.size > maximum ||
        !isValidUtf8(value)) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == '\0') return false;
    }
    return true;
}

bool validAscii(StringView value, size_t maximum) {
    if (value.data == nullptr || value.empty() || value.size > maximum) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        const uint8_t byte = static_cast<uint8_t>(value.data[index]);
        if (byte < 0x21U || byte > 0x7EU) return false;
    }
    return true;
}

bool validHost(StringView value) {
    if (!validAscii(value, kDeviceKeyMaximumHostSize)) return false;
    for (size_t index = 0U; index < value.size; ++index) {
        const char character = value.data[index];
        if (character == '/' || character == '?' || character == '#' ||
            character == '@' || character == ':') {
            return false;
        }
    }
    return true;
}

bool validTopic(StringView value) {
    if (!validAscii(value, kDeviceKeyMaximumTopicSize)) return false;
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == '+' || value.data[index] == '#') {
            return false;
        }
    }
    return true;
}

bool equal(StringView first, StringView second) {
    return first.data != nullptr && second.data != nullptr &&
           first.size == second.size &&
           memcmp(first.data, second.data, first.size) == 0;
}

bool validSecurity(MqttSecurity value) {
    return value == MqttSecurity::PlainTcp || value == MqttSecurity::Tls;
}

bool validChallengeRequest(const DeviceKeyChallengeRequest& body) {
    return nonZeroBytes(body.requestId, kDeviceKeyRequestIdSize) &&
           nonZeroBytes(body.locator, kDeviceKeyLocatorSize) &&
           nonZeroBytes(
               body.deviceInstanceId, kDeviceInstanceIdSize) &&
           nonZeroBytes(body.clientNonce, kDeviceKeyNonceSize);
}

bool validChallengeResponse(const DeviceKeyChallengeResponse& body) {
    return nonZeroBytes(body.requestId, kDeviceKeyRequestIdSize) &&
           nonZeroBytes(body.challengeId, kDeviceKeyChallengeIdSize) &&
           nonZeroBytes(body.serverNonce, kDeviceKeyNonceSize) &&
           body.credentialVersion != 0U &&
           body.expiresInSeconds != 0U &&
           body.expiresInSeconds <=
               kDeviceKeyMaximumChallengeLifetimeSeconds;
}

bool validSessionRequest(
    const DeviceKeySessionRequest& body,
    bool requireProof) {
    return nonZeroBytes(body.requestId, kDeviceKeyRequestIdSize) &&
           nonZeroBytes(body.challengeId, kDeviceKeyChallengeIdSize) &&
           body.protocolVersion == bbp2::kVersion &&
           validText(
               body.firmwareVersion,
               kDeviceKeyMaximumFirmwareVersionSize) &&
           validSecurity(body.requestedSecurity) &&
           body.proofAlgorithm == DeviceKeyProofAlgorithm::HmacSha256 &&
           (!requireProof || exactBytes(body.proof, kDeviceKeyProofSize));
}

bool validProofContext(const DeviceKeySessionProofContext& context) {
    return nonZeroBytes(context.requestId, kDeviceKeyRequestIdSize) &&
           nonZeroBytes(context.locator, kDeviceKeyLocatorSize) &&
           nonZeroBytes(
               context.deviceInstanceId, kDeviceInstanceIdSize) &&
           nonZeroBytes(context.clientNonce, kDeviceKeyNonceSize) &&
           nonZeroBytes(context.challengeId, kDeviceKeyChallengeIdSize) &&
           nonZeroBytes(context.serverNonce, kDeviceKeyNonceSize) &&
           context.credentialVersion != 0U &&
           context.protocolVersion == bbp2::kVersion &&
           validText(
               context.firmwareVersion,
               kDeviceKeyMaximumFirmwareVersionSize) &&
           validSecurity(context.requestedSecurity) &&
           context.proofAlgorithm == DeviceKeyProofAlgorithm::HmacSha256;
}

bool validSessionResponse(const DeviceKeySessionResponse& body) {
    return nonZeroBytes(body.requestId, kDeviceKeyRequestIdSize) &&
           body.credentialVersion != 0U &&
           body.broker == BrokerKind::Blinker && validHost(body.host) &&
           body.port != 0U && validSecurity(body.security) &&
           validAscii(body.clientId, kDeviceKeyMaximumClientIdSize) &&
           validAscii(body.username, kDeviceKeyMaximumUsernameSize) &&
           validAscii(body.password, kDeviceKeyMaximumPasswordSize) &&
           validTopic(body.publishTopic) &&
           validTopic(body.subscribeTopic) &&
           !equal(body.publishTopic, body.subscribeTopic) &&
           nonZeroBytes(body.sessionId, kDeviceKeySessionIdSize) &&
           body.expiresInSeconds >= 60U &&
           body.expiresInSeconds <=
               kDeviceKeyMaximumSessionLifetimeSeconds &&
           body.refreshAfterSeconds >= 30U &&
           body.refreshAfterSeconds < body.expiresInSeconds;
}

Result writeKey(cbor::Writer& writer, uint8_t key) {
    return writer.writeUnsigned(key);
}

Result readKey(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value == expected
               ? Result::success()
               : (result ? Result::failure(ErrorCode::InvalidEncoding)
                         : result);
}

Result readVersion(cbor::Reader& reader) {
    Result result = readKey(reader, 0U);
    uint64_t version = 0U;
    if (result) result = reader.readUnsigned(version);
    if (result && version != kDeviceKeySessionContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    return result;
}

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result beginDecode(
    ByteView encoded,
    cbor::Reader& reader,
    size_t expectedPairs) {
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != expectedPairs) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    return result;
}

Result finishDecode(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result writeProofTranscript(
    cbor::Writer& writer,
    const DeviceKeySessionProofContext& context) {
    Result result = writer.beginArray(13U);
    if (result) result = writer.writeText(StringView(kProofDomain));
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writer.writeBytes(context.requestId);
    if (result) result = writer.writeBytes(context.locator);
    if (result) result = writer.writeBytes(context.deviceInstanceId);
    if (result) result = writer.writeBytes(context.clientNonce);
    if (result) result = writer.writeBytes(context.challengeId);
    if (result) result = writer.writeBytes(context.serverNonce);
    if (result) result = writer.writeUnsigned(context.credentialVersion);
    if (result) result = writer.writeUnsigned(context.protocolVersion);
    if (result) result = writer.writeText(context.firmwareVersion);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(context.requestedSecurity));
    }
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(context.proofAlgorithm));
    }
    return result;
}

Result hmacSink(void* context, ByteView bytes) {
    if (context == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return static_cast<HmacSha256*>(context)->update(bytes);
}

bool validWireError(DeviceKeySessionWireError value) {
    switch (value) {
        case DeviceKeySessionWireError::Malformed:
        case DeviceKeySessionWireError::UnsupportedVersion:
        case DeviceKeySessionWireError::UnsupportedAlgorithm:
        case DeviceKeySessionWireError::DeviceKeyInvalid:
        case DeviceKeySessionWireError::DeviceKeyRotated:
        case DeviceKeySessionWireError::CredentialRevoked:
        case DeviceKeySessionWireError::ChallengeExpired:
        case DeviceKeySessionWireError::ChallengeConflict:
        case DeviceKeySessionWireError::SessionRevoked:
        case DeviceKeySessionWireError::DeviceRevoked:
        case DeviceKeySessionWireError::RateLimited:
        case DeviceKeySessionWireError::TemporarilyUnavailable:
            return true;
    }
    return false;
}

bool validRetryClass(DeviceKeySessionRetryClass value) {
    return static_cast<uint8_t>(value) <=
           static_cast<uint8_t>(
               DeviceKeySessionRetryClass::AuthorizationRequired);
}

bool validErrorSemantics(const DeviceKeySessionErrorBody& body) {
    if (!validWireError(body.error) || !validRetryClass(body.retryClass) ||
        body.hasRetryAfter !=
            (body.retryClass == DeviceKeySessionRetryClass::Backoff) ||
        (body.hasRetryAfter && body.retryAfterSeconds == 0U)) {
        return false;
    }
    switch (body.error) {
        case DeviceKeySessionWireError::RateLimited:
            return body.retryClass == DeviceKeySessionRetryClass::Backoff;
        case DeviceKeySessionWireError::TemporarilyUnavailable:
            return body.retryClass ==
                       DeviceKeySessionRetryClass::SameRequest ||
                   body.retryClass == DeviceKeySessionRetryClass::Backoff;
        case DeviceKeySessionWireError::ChallengeExpired:
            return body.retryClass ==
                   DeviceKeySessionRetryClass::NewChallenge;
        case DeviceKeySessionWireError::DeviceKeyInvalid:
        case DeviceKeySessionWireError::DeviceKeyRotated:
        case DeviceKeySessionWireError::CredentialRevoked:
        case DeviceKeySessionWireError::SessionRevoked:
        case DeviceKeySessionWireError::DeviceRevoked:
            return body.retryClass ==
                   DeviceKeySessionRetryClass::AuthorizationRequired;
        case DeviceKeySessionWireError::Malformed:
        case DeviceKeySessionWireError::UnsupportedVersion:
        case DeviceKeySessionWireError::UnsupportedAlgorithm:
        case DeviceKeySessionWireError::ChallengeConflict:
            return body.retryClass == DeviceKeySessionRetryClass::Never;
    }
    return false;
}

bool addSize(size_t value, size_t& total) {
    if (value > SIZE_MAX - total) return false;
    total += value;
    return true;
}

StringView copyText(
    MutableCharSpan arena,
    size_t& offset,
    StringView source) {
    char* destination = arena.data + offset;
    memcpy(destination, source.data, source.size);
    destination[source.size] = '\0';
    offset += source.size + 1U;
    return StringView(destination, source.size);
}

} // namespace

StringView deviceKeyChallengeTarget() {
    return StringView(kChallengePath);
}

StringView deviceKeySessionTarget() {
    return StringView(kSessionPath);
}

Result encodeDeviceKeyChallengeRequest(
    const DeviceKeyChallengeRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validChallengeRequest(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(5U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.locator);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeBytes(body.clientNonce);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeDeviceKeyChallengeRequest(
    ByteView encoded,
    DeviceKeyChallengeRequest& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 5U);
    DeviceKeyChallengeRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.locator);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readBytes(decoded.clientNonce);
    if (result) result = finishDecode(reader);
    if (result && !validChallengeRequest(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result encodeDeviceKeyChallengeResponse(
    const DeviceKeyChallengeResponse& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validChallengeResponse(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(6U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.challengeId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.serverNonce);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.expiresInSeconds);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeDeviceKeyChallengeResponse(
    ByteView encoded,
    DeviceKeyChallengeResponse& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 6U);
    DeviceKeyChallengeResponse decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.challengeId);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.serverNonce);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.expiresInSeconds);
    if (result) result = finishDecode(reader);
    if (result && !validChallengeResponse(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result encodeDeviceKeySessionRequest(
    const DeviceKeySessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validSessionRequest(body, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(8U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.challengeId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeUnsigned(body.protocolVersion);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.firmwareVersion);
    if (result) result = writeKey(writer, 5U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.requestedSecurity));
    }
    if (result) result = writeKey(writer, 6U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.proofAlgorithm));
    }
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.proof);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeDeviceKeySessionRequest(
    ByteView encoded,
    DeviceKeySessionRequest& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 8U);
    DeviceKeySessionRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.challengeId);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.protocolVersion = static_cast<uint8_t>(value);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.firmwareVersion);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.requestedSecurity = static_cast<MqttSecurity>(value);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.proofAlgorithm =
            static_cast<DeviceKeyProofAlgorithm>(value);
    }
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.proof);
    if (result) result = finishDecode(reader);
    if (result && !validSessionRequest(decoded, true)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result encodeDeviceKeySessionProofTranscript(
    const DeviceKeySessionProofContext& context,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validProofContext(context)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    const Result result = writeProofTranscript(writer, context);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result computeDeviceKeySessionProof(
    const DeviceKey& key,
    const DeviceKeySessionProofContext& context,
    MutableByteSpan proofOutput) {
    Result result = validateDeviceKey(key);
    if (result && !validProofContext(context)) {
        result = Result::failure(ErrorCode::InvalidArgument);
    }
    if (result &&
        (proofOutput.data == nullptr ||
         proofOutput.size < kDeviceKeyProofSize)) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }

    uint8_t authKey[kDeviceAuthKeySize] = {};
    HmacSha256 hmac;
    if (result) {
        result = deriveDeviceAuthKey(
            key, MutableByteSpan(authKey, sizeof(authKey)));
    }
    if (result) result = hmac.begin(ByteView(authKey, sizeof(authKey)));
    secureZero(MutableByteSpan(authKey, sizeof(authKey)));
    if (result) {
        cbor::Writer writer(hmacSink, &hmac);
        result = writeProofTranscript(writer, context);
    }
    if (result) {
        result = hmac.finish(MutableByteSpan(
            proofOutput.data, kDeviceKeyProofSize));
    }
    hmac.clear();
    return result;
}

Result verifyDeviceKeySessionProof(
    const DeviceKey& key,
    const DeviceKeySessionProofContext& context,
    ByteView proof) {
    if (!exactBytes(proof, kDeviceKeyProofSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t expected[kDeviceKeyProofSize] = {};
    Result result = computeDeviceKeySessionProof(
        key,
        context,
        MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(
                      ByteView(expected, sizeof(expected)), proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result encodeDeviceKeySessionResponse(
    const DeviceKeySessionResponse& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validSessionResponse(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(15U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.broker));
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeText(body.host);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.port);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.security));
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeText(body.clientId);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeText(body.username);
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeText(body.password);
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeText(body.publishTopic);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeText(body.subscribeTopic);
    if (result) result = writeKey(writer, 12U);
    if (result) result = writer.writeBytes(body.sessionId);
    if (result) result = writeKey(writer, 13U);
    if (result) result = writer.writeUnsigned(body.expiresInSeconds);
    if (result) result = writeKey(writer, 14U);
    if (result) result = writer.writeUnsigned(body.refreshAfterSeconds);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeDeviceKeySessionResponse(
    ByteView encoded,
    DeviceKeySessionResponse& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 15U);
    DeviceKeySessionResponse decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.broker = static_cast<BrokerKind>(value);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readText(decoded.host);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.port = static_cast<uint16_t>(value);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.security = static_cast<MqttSecurity>(value);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readText(decoded.clientId);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readText(decoded.username);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readText(decoded.password);
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readText(decoded.publishTopic);
    if (result) result = readKey(reader, 11U);
    if (result) result = reader.readText(decoded.subscribeTopic);
    if (result) result = readKey(reader, 12U);
    if (result) result = reader.readBytes(decoded.sessionId);
    if (result) result = readKey(reader, 13U);
    if (result) result = readU32(reader, decoded.expiresInSeconds);
    if (result) result = readKey(reader, 14U);
    if (result) result = readU32(reader, decoded.refreshAfterSeconds);
    if (result) result = finishDecode(reader);
    if (result && !validSessionResponse(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

Result copyDeviceKeySessionCredentials(
    const DeviceKeySessionResponse& response,
    MutableCharSpan credentialArena,
    SessionCredentials& credentials) {
    if (!validSessionResponse(response)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const StringView values[] = {
        response.host,
        response.clientId,
        response.username,
        response.password,
        response.publishTopic,
        response.subscribeTopic};
    size_t required = 0U;
    for (size_t index = 0U;
         index < sizeof(values) / sizeof(values[0]);
         ++index) {
        if (!addSize(values[index].size + 1U, required)) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
    }
    if (credentialArena.data == nullptr || required > credentialArena.size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(credentialArena.data),
        credentialArena.size));
    size_t offset = 0U;
    SessionCredentials copied;
    copied.broker = response.broker;
    copied.host = copyText(credentialArena, offset, response.host);
    copied.port = response.port;
    copied.security = response.security;
    copied.clientId = copyText(credentialArena, offset, response.clientId);
    copied.deviceName = copied.clientId;
    copied.username = copyText(credentialArena, offset, response.username);
    copied.password = copyText(credentialArena, offset, response.password);
    copied.publishTopic =
        copyText(credentialArena, offset, response.publishTopic);
    copied.subscribeTopic =
        copyText(credentialArena, offset, response.subscribeTopic);
    credentials = copied;
    return Result::success();
}

Result encodeDeviceKeySessionErrorBody(
    const DeviceKeySessionErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validErrorSemantics(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(body.hasRetryAfter ? 4U : 3U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(kDeviceKeySessionContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint16_t>(body.error));
    if (result) result = writeKey(writer, 2U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.retryClass));
    }
    if (result && body.hasRetryAfter) result = writeKey(writer, 3U);
    if (result && body.hasRetryAfter) {
        result = writer.writeUnsigned(body.retryAfterSeconds);
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeDeviceKeySessionErrorBody(
    ByteView encoded,
    DeviceKeySessionErrorBody& body) {
    cbor::Reader reader(encoded, limits());
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 3U && pairs != 4U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceKeySessionErrorBody decoded;
    if (result) result = readVersion(reader);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.error = static_cast<DeviceKeySessionWireError>(value);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.retryClass = static_cast<DeviceKeySessionRetryClass>(value);
    }
    decoded.hasRetryAfter = pairs == 4U;
    if (result && decoded.hasRetryAfter) result = readKey(reader, 3U);
    if (result && decoded.hasRetryAfter) {
        result = readU32(reader, decoded.retryAfterSeconds);
    }
    if (result) result = finishDecode(reader);
    if (result && !validErrorSemantics(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) body = decoded;
    return result;
}

uint16_t deviceKeySessionHttpStatus(DeviceKeySessionWireError error) {
    switch (error) {
        case DeviceKeySessionWireError::Malformed:
        case DeviceKeySessionWireError::UnsupportedVersion:
        case DeviceKeySessionWireError::UnsupportedAlgorithm:
            return 400U;
        case DeviceKeySessionWireError::DeviceKeyInvalid:
        case DeviceKeySessionWireError::SessionRevoked:
            return 401U;
        case DeviceKeySessionWireError::DeviceKeyRotated:
        case DeviceKeySessionWireError::CredentialRevoked:
        case DeviceKeySessionWireError::DeviceRevoked:
            return 403U;
        case DeviceKeySessionWireError::ChallengeExpired:
        case DeviceKeySessionWireError::ChallengeConflict:
            return 409U;
        case DeviceKeySessionWireError::RateLimited:
            return 429U;
        case DeviceKeySessionWireError::TemporarilyUnavailable:
            return 503U;
    }
    return 0U;
}

bool deviceKeySessionHttpStatusMatches(
    uint16_t statusCode,
    DeviceKeySessionWireError error) {
    return statusCode == deviceKeySessionHttpStatus(error);
}

} // namespace blinker
