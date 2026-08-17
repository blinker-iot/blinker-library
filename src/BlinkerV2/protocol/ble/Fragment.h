#ifndef BLINKER_PROTOCOL_BLE_FRAGMENT_H
#define BLINKER_PROTOCOL_BLE_FRAGMENT_H

#include "../../core/Result.h"
#include "../../core/Span.h"

namespace blinker {
namespace ble {

static const uint8_t kFragmentMagic = 0xB2;
static const uint8_t kFragmentVersion = 0x01;
static const uint8_t kFragmentVersionShift = 4;
static const uint8_t kFragmentVersionMask = 0xF0;
static const uint8_t kFragmentFlagsMask = 0x0F;
static const uint8_t kFragmentHeaderSize = 4;
static const uint8_t kMinimumPacketSize = 20;
static const size_t kMaximumFragmentCount = 256U;
static const size_t kMaximumRecordSizeAtMinimumPacket =
    kMaximumFragmentCount *
    (kMinimumPacketSize - kFragmentHeaderSize);

enum FragmentFlag : uint8_t {
    FragmentFlagNone = 0,
    FragmentFlagStart = 1U << 0,
    FragmentFlagEnd = 1U << 1
};

static const uint8_t kKnownFragmentFlags =
    FragmentFlagStart | FragmentFlagEnd;

struct FragmentHeader {
    uint8_t flags;
    uint8_t frameId;
    uint8_t index;

    FragmentHeader()
        : flags(FragmentFlagNone), frameId(0), index(0) {}
};

struct FragmentView {
    FragmentHeader header;
    ByteView payload;
};

Result encodeFragmentHeader(
    const FragmentHeader& header,
    MutableByteSpan output);
Result parseFragment(ByteView packet, FragmentView& fragment);

} // namespace ble
} // namespace blinker

#endif
