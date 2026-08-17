#ifndef BLINKER_IDENTITY_LOGICALDEVICEID_H
#define BLINKER_IDENTITY_LOGICALDEVICEID_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum : size_t { kLogicalDeviceIdCapacity = 64U };

Result validateLogicalDeviceId(StringView value);

} // namespace blinker

#endif
