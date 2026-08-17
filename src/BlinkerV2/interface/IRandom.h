#ifndef BLINKER_INTERFACE_IRANDOM_H
#define BLINKER_INTERFACE_IRANDOM_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

class IRandom {
public:
    virtual ~IRandom() {}

    // Implementations used for credentials/nonces must be cryptographically
    // secure. A platform without secure random must return NotConfigured.
    virtual Result fill(MutableByteSpan output) = 0;
};

} // namespace blinker

#endif
