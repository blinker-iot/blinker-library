#ifndef BLINKER_RUNTIME_IPRECLOUDDIRECTENDPOINT_H
#define BLINKER_RUNTIME_IPRECLOUDDIRECTENDPOINT_H

#include "../transport/IFrameTransport.h"

namespace blinker {

class BleFrameTransport;
class IAuthorizationProvider;

class IPreCloudDirectEndpoint {
public:
    virtual ~IPreCloudDirectEndpoint() {}

    // Synchronous, borrowed frame/session. Only dedicated install/handoff or
    // installed-Key network kinds reach it; each owner rejects other purposes.
    // Result is local, never a generic wire ACK.
    virtual Result receive(ByteView encodedFrame, const RxContext& rx,
        uint16_t negotiatedMaximumFrame) = 0;
    // Explicit owner cancellation/retirement. A supervisor must take any
    // admitted handoff value first. Ordinary BLE peer closure does not call it.
    virtual void retire() = 0;
};

// The explicit owner keeps this immutable binding and its services alive.
// The normal Runtime never constructs, advertises, or uses one.
struct PreCloudDirectBinding {
    BleFrameTransport& transport;
    IAuthorizationProvider& authorization;
    IPreCloudDirectEndpoint& endpoint;
};

} // namespace blinker
#endif
