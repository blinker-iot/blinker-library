#ifndef BLINKER_CORE_SECUREMEMORY_H
#define BLINKER_CORE_SECUREMEMORY_H

#include "Span.h"

namespace blinker {

void secureZero(MutableByteSpan memory);
bool constantTimeEqual(ByteView first, ByteView second);

} // namespace blinker

#endif
