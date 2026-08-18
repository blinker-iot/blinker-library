#ifndef BLINKER_INTERFACE_ICONTROLLERCONTROLENDPOINT_H
#define BLINKER_INTERFACE_ICONTROLLERCONTROLENDPOINT_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../transport/IFrameTransport.h"

namespace blinker {

// Runtime-facing seam for the BBP/2 Controller control family. Runtime owns
// frame validation and authorization; the endpoint owns nonce/session state
// and applies the already-defined durable controller mutation contract.
class IControllerControlEndpoint {
public:
    virtual ~IControllerControlEndpoint() {}

    virtual Result beginControlWindow(
        const RxContext& rx,
        ByteView& controlNonce) = 0;
    virtual Result applyControllerMutation(
        const RxContext& rx,
        ByteView encodedGrant,
        ByteView controllerSecret,
        MutableByteSpan output,
        ByteView& receipt) = 0;
    virtual void controllerSessionClosed(const RxContext& rx) = 0;
};

} // namespace blinker

#endif
