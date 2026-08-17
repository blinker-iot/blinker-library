#ifndef BLINKER_CORE_CRC32_H
#define BLINKER_CORE_CRC32_H

#include "Span.h"

namespace blinker {

// IEEE CRC-32 for persistent-record corruption detection. This is not a MAC
// and does not provide authenticity or confidentiality.
uint32_t computeCrc32(ByteView value);

} // namespace blinker

#endif
