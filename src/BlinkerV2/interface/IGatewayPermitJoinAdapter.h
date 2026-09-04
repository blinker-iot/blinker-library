#ifndef BLINKER_INTERFACE_IGATEWAYPERMITJOINADAPTER_H
#define BLINKER_INTERFACE_IGATEWAYPERMITJOINADAPTER_H

#include "../core/Result.h"

namespace blinker {

enum class GatewayPermitJoinAdapterState : uint8_t {
    Stopped = 0U,
    Ready,
    Fault
};

// The Edge Hub owns one physical southbound radio. A permit-join adapter must
// acquire it before scanning and release it when the bounded window ends.
class IGatewayPermitJoinPortLease {
public:
    virtual ~IGatewayPermitJoinPortLease() {}
    virtual Result acquirePermitJoinPort() = 0;
    virtual void releasePermitJoinPort() = 0;
};

// Adapter-scoped window control only. Enrollment bytes remain private to the
// concrete adapter and are never interpreted by the management coordinator.
class IGatewayPermitJoinAdapter {
public:
    virtual ~IGatewayPermitJoinAdapter() {}
    virtual uint16_t adapterId() const = 0;
    virtual Result openWindow() = 0;
    virtual void closeWindow() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual GatewayPermitJoinAdapterState state() const = 0;
    virtual ErrorCode lastError() const = 0;
};

} // namespace blinker

#endif
