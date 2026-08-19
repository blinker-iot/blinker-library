#ifndef BLINKER_INTERFACE_IBLECONNECTIONSECURITYSOURCE_H
#define BLINKER_INTERFACE_IBLECONNECTIONSECURITYSOURCE_H

#include "../core/Span.h"

namespace blinker {

// Platform fact source for the exact active BLE peer. Implementations must
// not answer from an any-peer bonded/paired query.
class IBleConnectionSecuritySource {
public:
    virtual ~IBleConnectionSecuritySource() {}
    virtual void connected(StringView peerAddress) = 0;
    virtual void disconnected() = 0;
    virtual bool encrypted() const = 0;
    virtual bool bonded() const = 0;
};

} // namespace blinker

#endif
