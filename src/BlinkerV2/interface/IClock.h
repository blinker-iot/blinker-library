#ifndef BLINKER_INTERFACE_ICLOCK_H
#define BLINKER_INTERFACE_ICLOCK_H

#include <stdint.h>

namespace blinker {

class IClock {
public:
    virtual ~IClock() {}

    virtual uint32_t monotonicMillis() const = 0;

    // Returns false when wall-clock time has not been synchronized.
    virtual bool unixTime(uint64_t& seconds) const = 0;
};

} // namespace blinker

#endif
