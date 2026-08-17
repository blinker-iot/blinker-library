#ifndef BLINKER_INTERFACE_ITRANSPORTLIFECYCLE_H
#define BLINKER_INTERFACE_ITRANSPORTLIFECYCLE_H

#include "../core/Result.h"

namespace blinker {

enum class TransportState : uint8_t {
    Stopped = 0,
    Starting,
    Online,
    Backoff,
    Error
};

// Common lifecycle shared by frame transports and local secure channels.
// Implementations must release all callbacks and physical resources in stop().
class ITransportLifecycle {
public:
    virtual ~ITransportLifecycle() {}

    virtual Result start() = 0;
    virtual void stop() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual TransportState state() const = 0;
};

} // namespace blinker

#endif
