#include "Control.h"

namespace blinker {
namespace local_access {
namespace control_detail {

bool exact(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t combined = 0U;
    for (size_t i = 0U; i < size; ++i) combined |= value.data[i];
    return combined != 0U;
}

Result fields(cbor::Writer& writer, const ControlView& value) {
    Result result = writer.writeUnsigned(kContractVersion);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.kind));
    if (result) result = writer.writeBytes(value.recipientDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeBytes(value.controlSessionId);
    if (result) result = writer.writeUnsigned(value.sequence);
    if (result) result = writer.writeUnsigned(value.authorityRevision);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.operation));
    if (result) result = writer.writeBytes(value.authorizationId);
    return result;
}

Result read32(cbor::Reader& reader, uint32_t& value) {
    uint64_t decoded = 0U;
    Result result = reader.readUnsigned(decoded);
    if (result && decoded > UINT32_MAX) return Result::failure(ErrorCode::InvalidEncoding);
    if (result) value = static_cast<uint32_t>(decoded);
    return result;
}

} // namespace control_detail

Result validateControlUnsigned(const ControlView& value) {
    bool valid = control_detail::exact(value.recipientDeviceInstanceId, kIdSize) &&
        value.deviceKeyVersion != 0U && control_detail::exact(value.controlSessionId, kIdSize);
    if (value.kind == ControlKind::Hello) {
        valid = valid && value.operation == ControlOperation::None && value.authorizationId.empty() &&
            ((value.sequence == 0U) == (value.authorityRevision == 0U));
    } else if (value.kind == ControlKind::Command || value.kind == ControlKind::Ack) {
        valid = valid && value.sequence != 0U && value.authorityRevision != 0U &&
            ((value.operation == ControlOperation::Retire && control_detail::exact(value.authorizationId, kIdSize)) ||
             ((value.operation == ControlOperation::Enable || value.operation == ControlOperation::Disable) &&
              value.authorizationId.empty()));
    } else valid = false;
    return valid ? Result::success() : Result::failure(ErrorCode::InvalidArgument);
}

Result validateControl(const ControlView& value) {
    Result result = validateControlUnsigned(value);
    if (result && !control_detail::exact(value.authenticator, kAuthenticatorSize)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return result;
}

Result encodeControl(const ControlView& value, MutableByteSpan output, size_t& written) {
    written = 0U;
    Result result = validateControl(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(10U);
    if (result) result = control_detail::fields(writer, value);
    if (result) result = writer.writeBytes(value.authenticator);
    if (result) written = writer.size();
    return result;
}

Result streamControlTranscript(const ControlView& value, cbor::WriteSink sink,
                               void* context, size_t& written) {
    written = 0U;
    Result result = validateControlUnsigned(value);
    if (!result) return result;
    if (sink == nullptr || context == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(sink, context);
    result = writer.beginArray(9U);
    if (result) result = control_detail::fields(writer, value);
    if (result) written = writer.size();
    return result;
}

Result decodeControl(ByteView encoded, ControlView& value) {
    value = ControlView();
    if (encoded.size > kControlMaximumEncodedSize) return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Limits limits;
    limits.maxByteStringLength = kAuthenticatorSize;
    limits.maxContainerItems = 10U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint32_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 10U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = control_detail::read32(reader, decoded);
    if (result && decoded != kContractVersion) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = control_detail::read32(reader, decoded);
    if (result && (decoded < 3U || decoded > 5U)) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value.kind = static_cast<ControlKind>(decoded);
    if (result) result = reader.readBytes(value.recipientDeviceInstanceId);
    if (result) result = control_detail::read32(reader, value.deviceKeyVersion);
    if (result) result = reader.readBytes(value.controlSessionId);
    if (result) result = control_detail::read32(reader, value.sequence);
    if (result) result = control_detail::read32(reader, value.authorityRevision);
    if (result) result = control_detail::read32(reader, decoded);
    if (result && decoded > 3U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value.operation = static_cast<ControlOperation>(decoded);
    if (result) result = reader.readBytes(value.authorizationId);
    if (result) result = reader.readBytes(value.authenticator);
    if (result && !reader.finished()) result = Result::failure(ErrorCode::TrailingData);
    if (result && !validateControl(value)) result = Result::failure(ErrorCode::InvalidEncoding);
    return result;
}

} // namespace local_access
} // namespace blinker
