#include "Fragment.h"

namespace blinker {
namespace ble {

Result encodeFragmentHeader(
    const FragmentHeader& header,
    MutableByteSpan output) {
    if (output.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.size < kFragmentHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if ((header.flags & static_cast<uint8_t>(~kKnownFragmentFlags)) != 0U ||
        header.frameId == 0U ||
        (((header.flags & FragmentFlagStart) != 0U) !=
         (header.index == 0U))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    output.data[0] = kFragmentMagic;
    output.data[1] = static_cast<uint8_t>(
        (kFragmentVersion << kFragmentVersionShift) | header.flags);
    output.data[2] = header.frameId;
    output.data[3] = header.index;
    return Result::success();
}

Result parseFragment(ByteView packet, FragmentView& fragment) {
    if (packet.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (packet.size <= kFragmentHeaderSize) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (packet.data[0] != kFragmentMagic) {
        return Result::failure(ErrorCode::InvalidMagic);
    }
    const uint8_t version = static_cast<uint8_t>(
        (packet.data[1] & kFragmentVersionMask) >>
        kFragmentVersionShift);
    if (version != kFragmentVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    const uint8_t flags =
        static_cast<uint8_t>(packet.data[1] & kFragmentFlagsMask);
    if ((flags & static_cast<uint8_t>(~kKnownFragmentFlags)) != 0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    FragmentHeader header;
    header.flags = flags;
    header.frameId = packet.data[2];
    header.index = packet.data[3];
    const size_t payloadSize = packet.size - kFragmentHeaderSize;
    if (header.frameId == 0U || payloadSize == 0U ||
        (((flags & FragmentFlagStart) != 0U) != (header.index == 0U))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    fragment.header = header;
    fragment.payload = packet.subspan(kFragmentHeaderSize, payloadSize);
    return Result::success();
}

} // namespace ble
} // namespace blinker
