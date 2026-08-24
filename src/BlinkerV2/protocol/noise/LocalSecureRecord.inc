#include "LocalSecureRecord.h"

#include <string.h>

namespace blinker {
namespace noise {

namespace {

const uint8_t kNoiseNnPrologue[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'L', 'O', 'C', 'A', 'L', '-',
    'S', 'E', 'T', 'U', 'P', '-', 'V', '1',
    kLocalSecureRecordMagic,
    kLocalSecureRecordVersion,
    static_cast<uint8_t>(NoiseNnPattern::Nn)
};

const uint8_t kNoiseNnPsk0Prologue[] = {
    'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'L', 'O', 'C', 'A', 'L', '-',
    'S', 'E', 'T', 'U', 'P', '-', 'V', '1',
    kLocalSecureRecordMagic,
    kLocalSecureRecordVersion,
    static_cast<uint8_t>(NoiseNnPattern::NnPsk0)
};

bool validType(LocalSecureRecordType type) {
    return type == LocalSecureRecordType::InitiatorHandshake ||
           type == LocalSecureRecordType::ResponderHandshake ||
           type == LocalSecureRecordType::Transport;
}

} // namespace

ByteView localSecureNoisePrologue(NoiseNnPattern pattern) {
    if (pattern == NoiseNnPattern::Nn) {
        return ByteView(kNoiseNnPrologue, sizeof(kNoiseNnPrologue));
    }
    if (pattern == NoiseNnPattern::NnPsk0) {
        return ByteView(kNoiseNnPsk0Prologue, sizeof(kNoiseNnPsk0Prologue));
    }
    return ByteView();
}

Result encodeLocalSecureRecord(
    LocalSecureRecordType type,
    ByteView body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    if (!validType(type) || body.data == nullptr || body.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (body.size > kLocalSecureRecordMaxBodySize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    const size_t total = kLocalSecureRecordHeaderSize + body.size;
    if (output.data == nullptr || output.size < total) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    output.data[0] = kLocalSecureRecordMagic;
    output.data[1] = static_cast<uint8_t>(
        (kLocalSecureRecordVersion << kLocalSecureRecordVersionShift) |
        static_cast<uint8_t>(type));
    output.data[2] = static_cast<uint8_t>(body.size >> 8U);
    output.data[3] = static_cast<uint8_t>(body.size);
    memmove(output.data + kLocalSecureRecordHeaderSize, body.data, body.size);
    encoded = ByteView(output.data, total);
    return Result::success();
}

Result parseLocalSecureRecord(
    ByteView encoded,
    LocalSecureRecordView& record) {
    record = LocalSecureRecordView();
    size_t total = 0U;
    Result result = decodeLocalSecureRecordSize(encoded, total);
    if (!result) return result;
    if (encoded.size < total) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (encoded.size != total) {
        return Result::failure(ErrorCode::TrailingData);
    }
    const LocalSecureRecordType type =
        static_cast<LocalSecureRecordType>(
            encoded.data[1] & kLocalSecureRecordTypeMask);
    const size_t bodySize = total - kLocalSecureRecordHeaderSize;
    record.type = type;
    record.body = ByteView(
        encoded.data + kLocalSecureRecordHeaderSize,
        bodySize);
    return Result::success();
}

Result decodeLocalSecureRecordSize(
    ByteView prefix,
    size_t& recordSize) {
    recordSize = 0U;
    if (prefix.data == nullptr ||
        prefix.size < kLocalSecureRecordHeaderSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (prefix.data[0] != kLocalSecureRecordMagic) {
        return Result::failure(ErrorCode::InvalidMagic);
    }
    const uint8_t version = static_cast<uint8_t>(
        (prefix.data[1] & kLocalSecureRecordVersionMask) >>
        kLocalSecureRecordVersionShift);
    if (version != kLocalSecureRecordVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    const LocalSecureRecordType type =
        static_cast<LocalSecureRecordType>(
            prefix.data[1] & kLocalSecureRecordTypeMask);
    if (!validType(type)) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    const size_t bodySize =
        (static_cast<size_t>(prefix.data[2]) << 8U) |
        static_cast<size_t>(prefix.data[3]);
    if (bodySize == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (bodySize > kLocalSecureRecordMaxBodySize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    recordSize = kLocalSecureRecordHeaderSize + bodySize;
    return Result::success();
}

} // namespace noise
} // namespace blinker
