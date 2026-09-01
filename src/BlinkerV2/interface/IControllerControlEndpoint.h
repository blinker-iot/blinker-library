#ifndef BLINKER_INTERFACE_ICONTROLLERCONTROLENDPOINT_H
#define BLINKER_INTERFACE_ICONTROLLERCONTROLENDPOINT_H

#include "../core/Result.h"
#include "../core/Span.h"
#include "../transport/IFrameTransport.h"

namespace blinker {

// Runtime-facing seam for authenticated direct-access management. Runtime
// owns frame validation and authorization; implementations apply controller
// grants and, when supported, strict-CAS PresenceKey mutations.
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
    virtual bool supportsPresenceKeyControl() const { return false; }
    virtual Result applyPresenceKeyMutation(
        const RxContext& rx,
        ByteView encodedMutation,
        MutableByteSpan output,
        ByteView& receipt) {
        (void)rx;
        (void)encodedMutation;
        (void)output;
        receipt = ByteView();
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    virtual void controllerSessionClosed(const RxContext& rx) = 0;
};

} // namespace blinker

#endif
