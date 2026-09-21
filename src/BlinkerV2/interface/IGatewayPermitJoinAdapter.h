#ifndef BLINKER_INTERFACE_IGATEWAYPERMITJOINADAPTER_H
#define BLINKER_INTERFACE_IGATEWAYPERMITJOINADAPTER_H

#include "../core/Result.h"

namespace blinker {

enum class GatewayPermitJoinAdapterState : uint8_t {
    Stopped = 0U,
    Ready,
    Fault,
    Opening,
    Closing
};

// Adapter-scoped window control only. Enrollment bytes remain private to the
// concrete adapter and are never interpreted by the management coordinator.
class IGatewayPermitJoinAdapter {
public:
    virtual ~IGatewayPermitJoinAdapter() {}
    virtual uint16_t adapterId() const = 0;
    // WouldBlock retains Opening; poll completes acquisition. closeWindow
    // cancels Opening or revokes Ready immediately, but Closing keeps ownership
    // until native release. Neither completion requires another wire command.
    virtual Result openWindow() = 0;
    virtual void closeWindow() = 0;
    virtual void poll(uint32_t budgetMicros) = 0;
    virtual GatewayPermitJoinAdapterState state() const = 0;
    virtual ErrorCode lastError() const = 0;
};

} // namespace blinker

#endif
