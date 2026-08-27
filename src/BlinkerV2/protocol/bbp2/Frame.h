#ifndef BLINKER_PROTOCOL_BBP2_FRAME_H
#define BLINKER_PROTOCOL_BBP2_FRAME_H

#include "../../core/Result.h"
#include "../../core/Span.h"

namespace blinker {
namespace bbp2 {

static const uint8_t kMagic0 = 0x42;
static const uint8_t kMagic1 = 0x4B;
static const uint8_t kVersion = 0x02;
static const uint8_t kBaseHeaderSize = 10;
static const uint8_t kCrc32cSize = 4;

enum class MessageKind : uint8_t {
    Hello = 0x01,
    ManifestRequest = 0x02,
    Manifest = 0x03,
    Authenticate = 0x04,
    AuthResult = 0x05,
    ManifestAccept = 0x06,
    // 0x07 and 0x08 were used by the pre-freeze device WidgetCatalog
    // experiment. They are reserved and must not be reused in BBP/2 v2.
    Route = 0x09,
    Delivery = 0x0A,
    StateRequest = 0x10,
    Patch = 0x11,
    Command = 0x12,
    Event = 0x13,
    Ack = 0x14,
    Error = 0x15,
    StatePage = 0x16,
    TelemetryControl = 0x17,
    TelemetryStatus = 0x18,
    TelemetryData = 0x19,
    // Broker/App account metadata. Device runtimes do not advertise or store it.
    PresenceControl = 0x1A,
    Presence = 0x1B,
    HistoryRequest = 0x20,
    HistoryBatch = 0x21,
    ControllerControlOpen = 0x30,
    ControllerControlChallenge = 0x31,
    ControllerMutation = 0x32,
    ControllerMutationReceipt = 0x33
};

enum FrameFlag : uint8_t {
    FlagNone = 0,
    FlagAckRequired = 1U << 0,
    FlagIsResponse = 1U << 1,
    FlagIdMode = 1U << 2,
    FlagHasCrc32c = 1U << 3
};

static const uint8_t kKnownFlags =
    FlagAckRequired | FlagIsResponse | FlagIdMode | FlagHasCrc32c;

struct FrameHeader {
    uint8_t version;
    uint8_t kind;
    uint8_t flags;
    uint8_t headerLength;
    uint16_t sequence;
    uint16_t bodyLength;

    FrameHeader()
        : version(kVersion),
          kind(static_cast<uint8_t>(MessageKind::Patch)),
          flags(FlagNone),
          headerLength(kBaseHeaderSize),
          sequence(0),
          bodyLength(0) {}
};

struct FrameView {
    FrameHeader header;
    ByteView body;
};

size_t encodedFrameSize(const FrameHeader& header);
Result encodeHeader(const FrameHeader& header, MutableByteSpan output);
Result decodeHeader(ByteView input, FrameHeader& header);
Result parseFrame(ByteView input, FrameView& frame);

} // namespace bbp2
} // namespace blinker

#endif
