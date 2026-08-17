#ifndef BLINKER_CORE_UTF8_H
#define BLINKER_CORE_UTF8_H

#include "Span.h"

namespace blinker {

bool isValidUtf8(StringView text);

} // namespace blinker

#endif
