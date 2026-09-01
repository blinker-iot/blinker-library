#ifndef BLINKER_PROTOCOL_DIRECTRECORD_H
#define BLINKER_PROTOCOL_DIRECTRECORD_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {
namespace direct {

// Logical record carried by a Blinker native point-to-point bearer. Before
// authentication it is a raw BBP/2 frame; afterwards it is DirectSecure. The
// bearer owns packetization (GATT, ESP-NOW, serial NCP, ...), not this codec.
Result decodeRecordSize(ByteView prefix, size_t& recordSize);
Result validateRecord(ByteView record);

} // namespace direct
} // namespace blinker

#endif
