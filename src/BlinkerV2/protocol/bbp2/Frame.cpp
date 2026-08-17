#include "Frame.h"

namespace blinker {
namespace bbp2 {

namespace {

uint16_t readU16Be(const uint8_t* data) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[0]) << 8) |
        static_cast<uint16_t>(data[1]));
}

void writeU16Be(uint8_t* data, uint16_t value) {
    data[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[1] = static_cast<uint8_t>(value & 0xFF);
}

} // namespace

size_t encodedFrameSize(const FrameHeader& header) {
    const size_t crcSize =
        (header.flags & FlagHasCrc32c) != 0 ? kCrc32cSize : 0;
    return static_cast<size_t>(header.headerLength) +
           static_cast<size_t>(header.bodyLength) +
           crcSize;
}

Result encodeHeader(const FrameHeader& header, MutableByteSpan output) {
    if (output.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (header.version != kVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (header.headerLength != kBaseHeaderSize) {
        return Result::failure(ErrorCode::InvalidHeader);
    }
    if ((header.flags & static_cast<uint8_t>(~kKnownFlags)) != 0) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    // CRC32C is specified but deliberately not accepted until the checksum
    // implementation and vectors land. Never emit an unverified CRC frame.
    if ((header.flags & FlagHasCrc32c) != 0) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (output.size < kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    output.data[0] = kMagic0;
    output.data[1] = kMagic1;
    output.data[2] = header.version;
    output.data[3] = header.kind;
    output.data[4] = header.flags;
    output.data[5] = header.headerLength;
    writeU16Be(output.data + 6, header.sequence);
    writeU16Be(output.data + 8, header.bodyLength);
    return Result::success();
}

Result decodeHeader(ByteView input, FrameHeader& header) {
    if (input.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (input.size < kBaseHeaderSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (input.data[0] != kMagic0 || input.data[1] != kMagic1) {
        return Result::failure(ErrorCode::InvalidMagic);
    }
    if (input.data[2] != kVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }

    header.version = input.data[2];
    header.kind = input.data[3];
    header.flags = input.data[4];
    header.headerLength = input.data[5];
    header.sequence = readU16Be(input.data + 6);
    header.bodyLength = readU16Be(input.data + 8);

    if (header.headerLength < kBaseHeaderSize) {
        return Result::failure(ErrorCode::InvalidHeader);
    }
    if ((header.flags & static_cast<uint8_t>(~kKnownFlags)) != 0) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if ((header.flags & FlagHasCrc32c) != 0) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (input.size < header.headerLength) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    return Result::success();
}

Result parseFrame(ByteView input, FrameView& frame) {
    FrameHeader header;
    const Result headerResult = decodeHeader(input, header);
    if (!headerResult) {
        return headerResult;
    }

    const size_t frameSize = encodedFrameSize(header);
    if (frameSize < header.headerLength) {
        return Result::failure(ErrorCode::InvalidHeader);
    }
    if (input.size < frameSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (input.size > frameSize) {
        return Result::failure(ErrorCode::TrailingData);
    }

    frame.header = header;
    frame.body = input.subspan(header.headerLength, header.bodyLength);
    return Result::success();
}

} // namespace bbp2
} // namespace blinker
