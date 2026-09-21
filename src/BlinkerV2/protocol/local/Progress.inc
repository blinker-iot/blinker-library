#include "Progress.h"

namespace blinker { namespace local_access {
namespace progress_detail {
bool exact(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t bits = 0U;
    for (size_t i = 0U; i < value.size; ++i) bits |= value.data[i];
    return bits != 0U;
}
Result fields(cbor::Writer& writer, const ProgressView& value) {
    Result result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.kind));
    if (result) result = writer.writeBytes(value.recipientDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeBytes(value.controlSessionId);
    if (result) result = writer.writeBytes(value.requestNonce);
    if (result) result = writer.writeUnsigned(value.sequence);
    if (result) result = writer.writeUnsigned(value.authorityRevision);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.state));
    if (result && value.version == 2U) result = writer.writeUnsigned(static_cast<uint8_t>(value.securityProfile));
    return result;
}
Result read32(cbor::Reader& reader, uint32_t& value) {
    uint64_t decoded = 0U;
    Result result = reader.readUnsigned(decoded);
    if (result && decoded > UINT32_MAX) return Result::failure(ErrorCode::InvalidEncoding);
    if (result) value = static_cast<uint32_t>(decoded);
    return result;
}
}

Result validateProgressUnsigned(const ProgressView& value) {
    const bool direction = (value.kind == ProgressKind::Query && value.state == ProgressState::Query) ||
        (value.kind == ProgressKind::Status && value.state >= ProgressState::Synchronized && value.state <= ProgressState::Refresh);
    const bool profile = (value.version == 1U && value.securityProfile == SecurityProfile::NoiseNnPsk0AesGcmSha256) ||
        (value.version == 2U && (value.securityProfile == SecurityProfile::NoiseNnPsk0AesGcmSha256 ||
            value.securityProfile == SecurityProfile::PlainHmacSha256));
    return profile && direction && progress_detail::exact(value.recipientDeviceInstanceId, kIdSize) && value.deviceKeyVersion != 0U &&
        progress_detail::exact(value.controlSessionId, kIdSize) && progress_detail::exact(value.requestNonce, kIdSize) &&
        ((value.sequence == 0U) == (value.authorityRevision == 0U))
        ? Result::success() : Result::failure(ErrorCode::InvalidArgument);
}
Result encodeProgress(const ProgressView& value, MutableByteSpan output, size_t& written) {
    written = 0U;
    Result result = validateProgressUnsigned(value);
    if (!result) return result;
    if (!progress_detail::exact(value.authenticator, kAuthenticatorSize)) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    result = writer.beginArray(value.version == 2U ? 11U : 10U);
    if (result) result = progress_detail::fields(writer, value);
    if (result) result = writer.writeBytes(value.authenticator);
    if (result) written = writer.size();
    return result;
}
Result streamProgressTranscript(const ProgressView& value, cbor::WriteSink sink, void* context, size_t& written) {
    written = 0U;
    Result result = validateProgressUnsigned(value);
    if (!result) return result;
    if (!sink || !context) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(sink, context);
    result = writer.beginArray(value.version == 2U ? 10U : 9U);
    if (result) result = progress_detail::fields(writer, value);
    if (result) written = writer.size();
    return result;
}
Result decodeProgress(ByteView encoded, ProgressView& value) {
    value = ProgressView();
    if (encoded.size > kControlMaximumEncodedSize) return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Limits limits;
    limits.maxByteStringLength = kAuthenticatorSize; limits.maxContainerItems = 11U; limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U; uint32_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 10U && count != 11U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = progress_detail::read32(reader, decoded);
    if (result && !((decoded == 1U && count == 10U) || (decoded == 2U && count == 11U)))
        result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = progress_detail::read32(reader, decoded);
    if (result && (decoded < 6U || decoded > 7U)) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value.kind = static_cast<ProgressKind>(decoded);
    if (result) result = reader.readBytes(value.recipientDeviceInstanceId);
    if (result) result = progress_detail::read32(reader, value.deviceKeyVersion);
    if (result) result = reader.readBytes(value.controlSessionId);
    if (result) result = reader.readBytes(value.requestNonce);
    if (result) result = progress_detail::read32(reader, value.sequence);
    if (result) result = progress_detail::read32(reader, value.authorityRevision);
    if (result) result = progress_detail::read32(reader, decoded);
    if (result && decoded > 3U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value.state = static_cast<ProgressState>(decoded);
    if (result && value.version == 2U) {
        result = progress_detail::read32(reader, decoded);
        if (result && (decoded < 1U || decoded > 2U)) result = Result::failure(ErrorCode::InvalidEncoding);
        if (result) value.securityProfile = static_cast<SecurityProfile>(decoded);
    }
    if (result) result = reader.readBytes(value.authenticator);
    if (result && (!reader.finished() || !progress_detail::exact(value.authenticator, kAuthenticatorSize) || !validateProgressUnsigned(value)))
        result = Result::failure(ErrorCode::InvalidEncoding);
    return result;
}
} } // namespace blinker::local_access
