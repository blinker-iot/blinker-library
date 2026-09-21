#include "Access.h"

#include <string.h>

namespace blinker {
namespace local_access {

namespace {

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool exactBytes(ByteView value, size_t size) {
    return value.size == size && nonZero(value);
}

bool validLogicalId(StringView value) {
    if (value.data == nullptr || value.empty() ||
        value.size > kMaximumLogicalDeviceIdSize) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        const uint8_t character = static_cast<uint8_t>(value.data[index]);
        if (character < 0x21U || character > 0x7EU || character == '/' ||
            character == '\\' || character == '#' || character == '+') {
            return false;
        }
    }
    return true;
}

bool validProfile(SecurityProfile profile) {
    return profile == SecurityProfile::NoiseNnPsk0AesGcmSha256 || profile == SecurityProfile::PlainHmacSha256;
}

bool validPermissions(uint32_t permissions) {
    const uint32_t allowed =
        kAuthorizationPermissionObserve | kAuthorizationPermissionControl;
    return permissions != 0U && (permissions & ~allowed) == 0U;
}

Result invalidArgument() {
    return Result::failure(ErrorCode::InvalidArgument);
}

Result invalidEncoding() {
    return Result::failure(ErrorCode::InvalidEncoding);
}

Result normalizedDecodeResult(Result result) {
    if (result || result.code() == ErrorCode::TruncatedInput ||
        result.code() == ErrorCode::TrailingData ||
        result.code() == ErrorCode::NonCanonicalEncoding) {
        return result;
    }
    return invalidEncoding();
}

Result readUnsigned32(cbor::Reader& reader, uint32_t& output) {
    uint64_t decoded = 0U;
    Result result = reader.readUnsigned(decoded);
    if (result && decoded > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(decoded);
    return result;
}

Result readExactBytes(
    cbor::Reader& reader,
    size_t expected,
    ByteView& output) {
    Result result = reader.readBytes(output);
    if (result && !exactBytes(output, expected)) result = invalidEncoding();
    return result;
}

Result writeGrantFields(cbor::Writer& writer, const GrantView& value) {
    Result result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGrantKind);
    if (result) result = writer.writeBytes(value.grantId);
    if (result) result = writer.writeBytes(value.authorizationId);
    if (result) result = writer.writeBytes(value.recipientDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeBytes(value.receiverSessionId);
    if (result) result = writer.writeBytes(value.challenge);
    if (result) result = writer.writeBytes(value.callerSessionId);
    if (result) result = writer.writeUnsigned(value.authorityRevision);
    if (result) result = writer.writeUnsigned(
        static_cast<uint8_t>(value.targetKind));
    if (result) result = writer.writeText(value.targetLogicalDeviceId);
    if (result) result = writer.writeBytes(value.targetDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeUnsigned(value.permissions);
    if (result) result = writer.writeUnsigned(value.lifetimeMillis);
    if (result) result = writer.writeUnsigned(
        static_cast<uint8_t>(value.securityProfile));
    return result;
}

} // namespace

ChallengeView::ChallengeView()
    : version(kContractVersion), recipientDeviceInstanceId(),
      deviceKeyVersion(0U), receiverSessionId(), challenge(),
      authorityRevision(0U), responseWindowMillis(kChallengeResponseMillis),
      maximumLifetimeMillis(kMaximumLifetimeMillis),
      securityProfile(SecurityProfile::NoiseNnPsk0AesGcmSha256) {}

GrantView::GrantView()
    : version(kContractVersion), grantId(), authorizationId(),
      recipientDeviceInstanceId(), deviceKeyVersion(0U), receiverSessionId(),
      challenge(), callerSessionId(), authorityRevision(0U),
      targetKind(TargetKind::Self), targetLogicalDeviceId(),
      targetDeviceInstanceId(), accessEpoch(0U), topologyVersion(0U),
      permissions(0U), lifetimeMillis(0U),
      securityProfile(SecurityProfile::NoiseNnPsk0AesGcmSha256),
      authenticator() {}

Result validateChallenge(const ChallengeView& value) {
    if (value.version != kContractVersion ||
        !exactBytes(value.recipientDeviceInstanceId, kIdSize) ||
        value.deviceKeyVersion == 0U ||
        !exactBytes(value.receiverSessionId, kIdSize) ||
        !exactBytes(value.challenge, kChallengeSize) ||
        value.authorityRevision == 0U ||
        value.responseWindowMillis != kChallengeResponseMillis ||
        value.maximumLifetimeMillis != kMaximumLifetimeMillis ||
        !validProfile(value.securityProfile)) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeChallenge(
    const ChallengeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateChallenge(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(10U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kChallengeKind);
    if (result) result = writer.writeBytes(value.recipientDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeBytes(value.receiverSessionId);
    if (result) result = writer.writeBytes(value.challenge);
    if (result) result = writer.writeUnsigned(value.authorityRevision);
    if (result) result = writer.writeUnsigned(value.responseWindowMillis);
    if (result) result = writer.writeUnsigned(value.maximumLifetimeMillis);
    if (result) result = writer.writeUnsigned(
        static_cast<uint8_t>(value.securityProfile));
    if (result) written = writer.size();
    return result;
}

Result decodeChallenge(ByteView encoded, ChallengeView& value) {
    value = ChallengeView();
    cbor::Limits limits;
    limits.maxByteStringLength = kChallengeSize;
    limits.maxContainerItems = 10U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 10U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kContractVersion) result = invalidEncoding();
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kChallengeKind) result = invalidEncoding();
    if (result) result = readExactBytes(
        reader, kIdSize, value.recipientDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.deviceKeyVersion);
    if (result) result = readExactBytes(
        reader, kIdSize, value.receiverSessionId);
    if (result) result = readExactBytes(
        reader, kChallengeSize, value.challenge);
    if (result) result = readUnsigned32(reader, value.authorityRevision);
    if (result) result = readUnsigned32(reader, value.responseWindowMillis);
    if (result) result = readUnsigned32(reader, value.maximumLifetimeMillis);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded > UINT8_MAX) result = invalidEncoding();
    if (result) value.securityProfile =
        static_cast<SecurityProfile>(static_cast<uint8_t>(decoded));
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateChallenge(value);
    return normalizedDecodeResult(result);
}

Result validateGrantUnsigned(const GrantView& value) {
    if (value.version != kContractVersion ||
        !exactBytes(value.grantId, kIdSize) ||
        !exactBytes(value.authorizationId, kIdSize) ||
        !exactBytes(value.recipientDeviceInstanceId, kIdSize) ||
        value.deviceKeyVersion == 0U ||
        !exactBytes(value.receiverSessionId, kIdSize) ||
        !exactBytes(value.challenge, kChallengeSize) ||
        !exactBytes(value.callerSessionId, kIdSize) ||
        value.authorityRevision == 0U ||
        !exactBytes(value.targetDeviceInstanceId, kIdSize) ||
        !validPermissions(value.permissions) ||
        value.lifetimeMillis < kMinimumLifetimeMillis ||
        value.lifetimeMillis > kMaximumLifetimeMillis ||
        !validProfile(value.securityProfile)) {
        return invalidArgument();
    }
    if (value.targetKind == TargetKind::Self) {
        if (!value.targetLogicalDeviceId.empty() || value.accessEpoch != 0U ||
            value.topologyVersion != 0U ||
            memcmp(value.targetDeviceInstanceId.data,
                   value.recipientDeviceInstanceId.data, kIdSize) != 0) {
            return invalidArgument();
        }
    } else if (value.targetKind == TargetKind::Child) {
        if (!validLogicalId(value.targetLogicalDeviceId) ||
            value.accessEpoch == 0U || value.topologyVersion == 0U) {
            return invalidArgument();
        }
    } else {
        return invalidArgument();
    }
    return Result::success();
}

Result validateGrant(const GrantView& value) {
    Result result = validateGrantUnsigned(value);
    if (result && !exactBytes(value.authenticator, kAuthenticatorSize)) {
        result = invalidArgument();
    }
    return result;
}

Result streamGrantTranscript(
    const GrantView& value,
    cbor::WriteSink sink,
    void* context,
    size_t& written) {
    written = 0U;
    Result result = validateGrantUnsigned(value);
    if (!result) return result;
    if (sink == nullptr || context == nullptr) return invalidArgument();
    cbor::Writer writer(sink, context);
    result = writer.beginArray(18U);
    if (result) result = writeGrantFields(writer, value);
    if (result) written = writer.size();
    return result;
}

Result encodeGrant(
    const GrantView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGrant(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(19U);
    if (result) result = writeGrantFields(writer, value);
    if (result) result = writer.writeBytes(value.authenticator);
    if (result) written = writer.size();
    return result;
}

Result decodeGrant(ByteView encoded, GrantView& value) {
    value = GrantView();
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kAuthenticatorSize;
    limits.maxContainerItems = 19U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 19U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kContractVersion) result = invalidEncoding();
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGrantKind) result = invalidEncoding();
    if (result) result = readExactBytes(reader, kIdSize, value.grantId);
    if (result) result = readExactBytes(
        reader, kIdSize, value.authorizationId);
    if (result) result = readExactBytes(
        reader, kIdSize, value.recipientDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.deviceKeyVersion);
    if (result) result = readExactBytes(
        reader, kIdSize, value.receiverSessionId);
    if (result) result = readExactBytes(
        reader, kChallengeSize, value.challenge);
    if (result) result = readExactBytes(
        reader, kIdSize, value.callerSessionId);
    if (result) result = readUnsigned32(reader, value.authorityRevision);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded > UINT8_MAX) result = invalidEncoding();
    if (result) value.targetKind =
        static_cast<TargetKind>(static_cast<uint8_t>(decoded));
    if (result) result = reader.readText(value.targetLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kIdSize, value.targetDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = readUnsigned32(reader, value.permissions);
    if (result) result = readUnsigned32(reader, value.lifetimeMillis);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded > UINT8_MAX) result = invalidEncoding();
    if (result) value.securityProfile =
        static_cast<SecurityProfile>(static_cast<uint8_t>(decoded));
    if (result) result = readExactBytes(
        reader, kAuthenticatorSize, value.authenticator);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGrant(value);
    return normalizedDecodeResult(result);
}

Result encodePlainProof(uint8_t kind, ByteView authenticator, MutableByteSpan output, size_t& written) {
    written = 0U;
    if ((kind != kCallerProofKind && kind != kRecipientProofKind) ||
        authenticator.data == nullptr || authenticator.size != kAuthenticatorSize) return invalidArgument();
    cbor::Writer writer(output);
    Result result = writer.beginArray(3U);
    if (result) result = writer.writeUnsigned(kContractVersion);
    if (result) result = writer.writeUnsigned(kind);
    if (result) result = writer.writeBytes(authenticator);
    if (result) written = writer.size();
    return result;
}

Result decodePlainProof(ByteView encoded, uint8_t expectedKind, ByteView& authenticator) {
    authenticator = ByteView();
    if ((expectedKind != kCallerProofKind && expectedKind != kRecipientProofKind) ||
        encoded.data == nullptr || encoded.size != kPlainProofEncodedSize) return invalidEncoding();
    cbor::Limits limits; limits.maxByteStringLength = kAuthenticatorSize;
    limits.maxContainerItems = 3U; limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U; uint64_t value = 0U; ByteView proof;
    Result result = reader.readArraySize(count);
    if (result && count != 3U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(value);
    if (result && value != kContractVersion) result = invalidEncoding();
    if (result) result = reader.readUnsigned(value);
    if (result && value != expectedKind) result = invalidEncoding();
    if (result) result = reader.readBytes(proof);
    if (result && (proof.size != kAuthenticatorSize || !reader.finished())) result = invalidEncoding();
    if (result) authenticator = proof;
    return normalizedDecodeResult(result);
}

} // namespace local_access
} // namespace blinker
