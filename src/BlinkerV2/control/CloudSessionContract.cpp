#include "CloudSessionContract.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Utf8.h"
#include "../protocol/bbp2/Frame.h"

namespace blinker {

namespace {

static const char kRequestDomain[] =
    "blinker.device-cloud-session.request.v2";

cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = kCloudSessionMaxHostSize;
    value.maxByteStringLength = kCloudSessionProofSize;
    value.maxContainerItems = 14U;
    value.maxDepth = 2U;
    return value;
}

bool exactBytes(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
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
    if (!validAscii(value, kCloudSessionMaxHostSize)) return false;
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
    if (!validAscii(value, kCloudSessionMaxTopicSize)) return false;
    for (size_t index = 0U; index < value.size; ++index) {
        if (value.data[index] == '+' || value.data[index] == '#') {
            return false;
        }
    }
    return true;
}

bool equal(StringView left, StringView right) {
    return left.size == right.size && left.data != nullptr &&
           right.data != nullptr &&
           memcmp(left.data, right.data, left.size) == 0;
}

bool validRequest(const CloudSessionRequest& body, bool requireProof) {
    return exactBytes(body.requestId, kCloudSessionRequestIdSize) &&
           validText(
               body.logicalDeviceId,
               kCloudLogicalDeviceIdCapacity) &&
           body.credentialVersion != 0U && body.generation != 0U &&
           body.protocolVersion == bbp2::kVersion &&
           validText(
               body.firmwareVersion,
               kCloudSessionMaxFirmwareVersionSize) &&
           body.proofAlgorithm == CloudSessionProofAlgorithm::HmacSha256 &&
           (!requireProof ||
            exactBytes(body.proof, kCloudSessionProofSize));
}

bool validResponse(const CloudSessionResponse& body) {
    return exactBytes(body.requestId, kCloudSessionRequestIdSize) &&
           body.broker == BrokerKind::Blinker &&
           validHost(body.host) && body.port != 0U &&
           (body.security == MqttSecurity::PlainTcp ||
            body.security == MqttSecurity::Tls) &&
           validAscii(body.clientId, kCloudSessionMaxClientIdSize) &&
           validAscii(body.username, kCloudSessionMaxUsernameSize) &&
           validAscii(body.password, kCloudSessionMaxPasswordSize) &&
           validTopic(body.publishTopic) &&
           validTopic(body.subscribeTopic) &&
           !equal(body.publishTopic, body.subscribeTopic) &&
           exactBytes(body.sessionId, kCloudSessionIdSize) &&
           body.expiresInSeconds >= 60U &&
           body.expiresInSeconds <= kCloudSessionMaximumLifetimeSeconds &&
           body.refreshAfterSeconds >= 30U &&
           body.refreshAfterSeconds < body.expiresInSeconds;
}

bool validRetryClass(CloudSessionRetryClass value) {
    return static_cast<uint8_t>(value) <=
           static_cast<uint8_t>(CloudSessionRetryClass::NewRequest);
}

bool validWireError(CloudSessionWireError value) {
    switch (value) {
        case CloudSessionWireError::Malformed:
        case CloudSessionWireError::UnsupportedVersion:
        case CloudSessionWireError::UnsupportedAlgorithm:
        case CloudSessionWireError::AuthenticationRequired:
        case CloudSessionWireError::CredentialNotFound:
        case CloudSessionWireError::CredentialRevoked:
        case CloudSessionWireError::CredentialVersionConflict:
        case CloudSessionWireError::ProofInvalid:
        case CloudSessionWireError::RequestConflict:
        case CloudSessionWireError::DeviceRevoked:
        case CloudSessionWireError::RequestExpired:
        case CloudSessionWireError::RateLimited:
        case CloudSessionWireError::TemporarilyUnavailable:
            return true;
    }
    return false;
}

bool validErrorSemantics(const CloudSessionErrorBody& body) {
    if (!validWireError(body.error) || !validRetryClass(body.retryClass) ||
        body.hasRetryAfter !=
            (body.retryClass == CloudSessionRetryClass::Backoff) ||
        (body.hasRetryAfter && body.retryAfterSeconds == 0U)) {
        return false;
    }
    switch (body.error) {
        case CloudSessionWireError::RateLimited:
            return body.retryClass == CloudSessionRetryClass::Backoff;
        case CloudSessionWireError::TemporarilyUnavailable:
            return body.retryClass == CloudSessionRetryClass::SameRequest ||
                   body.retryClass == CloudSessionRetryClass::Backoff;
        case CloudSessionWireError::AuthenticationRequired:
        case CloudSessionWireError::CredentialNotFound:
        case CloudSessionWireError::CredentialRevoked:
        case CloudSessionWireError::ProofInvalid:
        case CloudSessionWireError::DeviceRevoked:
            return body.retryClass == CloudSessionRetryClass::Reenroll;
        case CloudSessionWireError::RequestExpired:
            return body.retryClass == CloudSessionRetryClass::NewRequest;
        case CloudSessionWireError::Malformed:
        case CloudSessionWireError::UnsupportedVersion:
        case CloudSessionWireError::UnsupportedAlgorithm:
        case CloudSessionWireError::CredentialVersionConflict:
        case CloudSessionWireError::RequestConflict:
            return body.retryClass == CloudSessionRetryClass::Never;
    }
    return false;
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

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (!result) return result;
    if (value > UINT32_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    output = static_cast<uint32_t>(value);
    return Result::success();
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

Result readVersion(cbor::Reader& reader) {
    Result result = readKey(reader, 0U);
    uint64_t version = 0U;
    if (result) result = reader.readUnsigned(version);
    if (result && version != kCloudSessionContractVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    return result;
}

Result finishDecode(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
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

Result encodeCloudSessionRequest(
    const CloudSessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validRequest(body, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(9U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kCloudSessionContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.generation);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.protocolVersion);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeText(body.firmwareVersion);
    if (result) result = writeKey(writer, 7U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.proofAlgorithm));
    }
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeBytes(body.proof);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeCloudSessionRequest(
    ByteView encoded,
    CloudSessionRequest& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 9U);
    CloudSessionRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readText(decoded.logicalDeviceId);
    if (result) result = readKey(reader, 3U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.generation);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.protocolVersion = static_cast<uint8_t>(value);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readText(decoded.firmwareVersion);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.proofAlgorithm =
            static_cast<CloudSessionProofAlgorithm>(value);
    }
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readBytes(decoded.proof);
    if (result) result = finishDecode(reader);
    if (result && !validRequest(decoded, true)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeCloudSessionRequestTranscript(
    const CloudSessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validRequest(body, false)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(8U);
    if (result) {
        result = writer.writeText(StringView(
            kRequestDomain,
            sizeof(kRequestDomain) - 1U));
    }
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writer.writeText(body.logicalDeviceId);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeUnsigned(body.generation);
    if (result) result = writer.writeUnsigned(body.protocolVersion);
    if (result) result = writer.writeText(body.firmwareVersion);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(body.proofAlgorithm));
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result computeCloudSessionProof(
    ByteView cloudCredentialSecret,
    const CloudSessionRequest& body,
    MutableByteSpan transcriptBuffer,
    MutableByteSpan proofOutput) {
    if (!exactBytes(cloudCredentialSecret, kCloudCredentialSecretSize) ||
        proofOutput.data == nullptr ||
        proofOutput.size < kCloudSessionProofSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ByteView transcript;
    Result result = encodeCloudSessionRequestTranscript(
        body,
        transcriptBuffer,
        transcript);
    if (result) {
        result = hmacSha256(
            cloudCredentialSecret,
            transcript,
            MutableByteSpan(proofOutput.data, kCloudSessionProofSize));
    }
    return result;
}

Result verifyCloudSessionProof(
    ByteView cloudCredentialSecret,
    const CloudSessionRequest& body,
    MutableByteSpan transcriptBuffer) {
    if (!exactBytes(body.proof, kCloudSessionProofSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint8_t expected[kCloudSessionProofSize] = {};
    Result result = computeCloudSessionProof(
        cloudCredentialSecret,
        body,
        transcriptBuffer,
        MutableByteSpan(expected, sizeof(expected)));
    if (result && !constantTimeEqual(
                      ByteView(expected, sizeof(expected)),
                      body.proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

Result encodeCloudSessionResponse(
    const CloudSessionResponse& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validResponse(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(14U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kCloudSessionContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.requestId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.broker));
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeText(body.host);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.port);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(body.security));
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeText(body.clientId);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeText(body.username);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeText(body.password);
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeText(body.publishTopic);
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeText(body.subscribeTopic);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeBytes(body.sessionId);
    if (result) result = writeKey(writer, 12U);
    if (result) result = writer.writeUnsigned(body.expiresInSeconds);
    if (result) result = writeKey(writer, 13U);
    if (result) result = writer.writeUnsigned(body.refreshAfterSeconds);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeCloudSessionResponse(
    ByteView encoded,
    CloudSessionResponse& body) {
    cbor::Reader reader(encoded, limits());
    Result result = beginDecode(encoded, reader, 14U);
    CloudSessionResponse decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.requestId);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.broker = static_cast<BrokerKind>(value);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readText(decoded.host);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.port = static_cast<uint16_t>(value);
    if (result) result = readKey(reader, 5U);
    if (result) result = reader.readUnsigned(value);
    if (result) decoded.security = static_cast<MqttSecurity>(value);
    if (result) result = readKey(reader, 6U);
    if (result) result = reader.readText(decoded.clientId);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readText(decoded.username);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readText(decoded.password);
    if (result) result = readKey(reader, 9U);
    if (result) result = reader.readText(decoded.publishTopic);
    if (result) result = readKey(reader, 10U);
    if (result) result = reader.readText(decoded.subscribeTopic);
    if (result) result = readKey(reader, 11U);
    if (result) result = reader.readBytes(decoded.sessionId);
    if (result) result = readKey(reader, 12U);
    if (result) result = readU32(reader, decoded.expiresInSeconds);
    if (result) result = readKey(reader, 13U);
    if (result) result = readU32(reader, decoded.refreshAfterSeconds);
    if (result) result = finishDecode(reader);
    if (result && !validResponse(decoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result copyCloudSessionCredentials(
    const CloudSessionResponse& response,
    MutableCharSpan credentialArena,
    SessionCredentials& credentials) {
    if (!validResponse(response)) {
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

Result encodeCloudSessionErrorBody(
    const CloudSessionErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded) {
    if (!validErrorSemantics(body)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    cbor::Writer writer(output);
    Result result = writer.beginMap(body.hasRetryAfter ? 4U : 3U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kCloudSessionContractVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeUnsigned(static_cast<uint16_t>(body.error));
    if (result) result = writeKey(writer, 2U);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(body.retryClass));
    }
    if (result && body.hasRetryAfter) result = writeKey(writer, 3U);
    if (result && body.hasRetryAfter) {
        result = writer.writeUnsigned(body.retryAfterSeconds);
    }
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeCloudSessionErrorBody(
    ByteView encoded,
    CloudSessionErrorBody& body) {
    cbor::Reader reader(encoded, limits());
    Result result = cbor::validate(encoded, limits());
    size_t pairs = 0U;
    if (result) result = reader.readMapSize(pairs);
    if (result && pairs != 3U && pairs != 4U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    CloudSessionErrorBody decoded;
    if (result) result = readVersion(reader);
    uint64_t value = 0U;
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT16_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.error = static_cast<CloudSessionWireError>(value);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readUnsigned(value);
    if (result && value > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        decoded.retryClass = static_cast<CloudSessionRetryClass>(value);
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
    if (!result) return result;
    body = decoded;
    return Result::success();
}

uint16_t cloudSessionHttpStatus(CloudSessionWireError error) {
    switch (error) {
        case CloudSessionWireError::Malformed:
        case CloudSessionWireError::UnsupportedVersion:
        case CloudSessionWireError::UnsupportedAlgorithm:
            return 400U;
        case CloudSessionWireError::AuthenticationRequired:
        case CloudSessionWireError::CredentialNotFound:
        case CloudSessionWireError::ProofInvalid:
            return 401U;
        case CloudSessionWireError::CredentialRevoked:
        case CloudSessionWireError::DeviceRevoked:
            return 403U;
        case CloudSessionWireError::CredentialVersionConflict:
        case CloudSessionWireError::RequestConflict:
        case CloudSessionWireError::RequestExpired:
            return 409U;
        case CloudSessionWireError::RateLimited:
            return 429U;
        case CloudSessionWireError::TemporarilyUnavailable:
            return 503U;
    }
    return 0U;
}

bool cloudSessionHttpStatusMatches(
    uint16_t statusCode,
    CloudSessionWireError error) {
    return statusCode == cloudSessionHttpStatus(error);
}

} // namespace blinker
