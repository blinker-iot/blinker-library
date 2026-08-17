#ifndef BLINKER_INTERFACE_ISETUPPSKPROVIDER_H
#define BLINKER_INTERFACE_ISETUPPSKPROVIDER_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../protocol/SetupSession.h"

namespace blinker {

static const size_t kSetupPskSize = 32U;

// Enhanced/manufactured product boundary for one high-entropy setup secret.
// The product-owned setup-window coordinator creates and expires the 8-byte
// locator; this narrow provider only resolves an active locator to exactly
// kSetupPskSize bytes. It may not log or persist raw PSK scratch outside the
// protected factory domain.
class ISetupPskProvider {
public:
    virtual ~ISetupPskProvider() {}

    virtual Result loadSetupPsk(
        ByteView setupSessionLocator,
        MutableByteSpan output,
        size_t& written) = 0;
};

} // namespace blinker

#endif
