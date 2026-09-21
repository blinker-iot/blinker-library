#ifndef BLINKER_INTERFACE_IWALLTIMESOURCE_H
#define BLINKER_INTERFACE_IWALLTIMESOURCE_H

#include "../core/WallClock.h"

namespace blinker {
// Read-only business snapshot. Reading advances age/uncertainty and can latch
// expiry inside the owner; it grants no access to synchronize/reset or IClock.
class IWallTimeSource {
public:
    virtual ~IWallTimeSource() {}
    virtual bool readTime(WallClockSnapshot& snapshot) = 0;
};
}
#endif
