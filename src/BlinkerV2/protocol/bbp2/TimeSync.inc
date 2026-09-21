#include "TimeSync.h"
#include "../cbor/Cbor.h"

namespace blinker { namespace bbp2 {
namespace {
bool validTimeResponse(const TimeResponseView& value) {
    return validTimeSyncNonce(value.nonce) && value.utcAtReplyMillis >= kWallTimeMinimumMillis &&
        value.utcAtReplyMillis < kWallTimeEndMillis && value.uncertaintyMillis <= kWallTimeMaxUncertaintyMillis;
}
}

bool validTimeSyncNonce(ByteView nonce) {
    if (nonce.data == nullptr || nonce.size != kTimeSyncNonceSize) return false;
    uint8_t bits = 0U;
    for (size_t i = 0; i < nonce.size; ++i) bits |= nonce.data[i];
    return bits != 0U;
}

Result encodeTimeRequest(ByteView nonce, MutableByteSpan output, size_t& written) {
    written = 0U;
    if (!validTimeSyncNonce(nonce)) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    Result result = writer.beginArray(1U);
    if (result) result = writer.writeBytes(nonce);
    if (result) written = writer.size();
    return result;
}

Result decodeTimeRequest(ByteView body, ByteView& nonce) {
    nonce = ByteView();
    if (body.size != kTimeRequestSize) return Result::failure(ErrorCode::InvalidEncoding);
    cbor::Reader reader(body);
    size_t count = 0U;
    ByteView decoded;
    Result result = reader.readArraySize(count);
    if (result && count != 1U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = reader.readBytes(decoded);
    if (result && (!reader.finished() || !validTimeSyncNonce(decoded))) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) nonce = decoded;
    return result;
}

Result encodeTimeResponse(const TimeResponseView& value, MutableByteSpan output, size_t& written) {
    written = 0U;
    if (!validTimeResponse(value)) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    Result result = writer.beginArray(3U);
    if (result) result = writer.writeBytes(value.nonce);
    if (result) result = writer.writeUnsigned(value.utcAtReplyMillis);
    if (result) result = writer.writeUnsigned(value.uncertaintyMillis);
    if (result) written = writer.size();
    return result;
}

Result decodeTimeResponse(ByteView body, TimeResponseView& value) {
    value = TimeResponseView();
    if (body.size > kTimeResponseMaxSize) return Result::failure(ErrorCode::InvalidEncoding);
    TimeResponseView decoded;
    cbor::Reader reader(body);
    size_t count = 0U;
    uint64_t uncertainty = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 3U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = reader.readBytes(decoded.nonce);
    if (result) result = reader.readUnsigned(decoded.utcAtReplyMillis);
    if (result) result = reader.readUnsigned(uncertainty);
    if (result && uncertainty > kWallTimeMaxUncertaintyMillis) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) decoded.uncertaintyMillis = static_cast<uint32_t>(uncertainty);
    if (result && (!reader.finished() || !validTimeResponse(decoded))) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) value = decoded;
    return result;
}

} } // namespace blinker::bbp2
