#ifndef BLINKER_PROVISIONING_CONTROLLERCONTROLENDPOINT_H
#define BLINKER_PROVISIONING_CONTROLLERCONTROLENDPOINT_H

#include "ControllerControlCoordinator.h"
#include "../interface/IControllerControlEndpoint.h"
#include "../interface/IRandom.h"

namespace blinker {

// One active local session may own the control window. The 256-byte contract
// workspace is caller-owned so products can account for or safely reuse it.
class ControllerControlEndpoint final
    : public IControllerControlEndpoint {
public:
    ControllerControlEndpoint(
        ControllerControlCoordinator& coordinator,
        IRandom& random,
        MutableByteSpan workspace);
    ~ControllerControlEndpoint() override;

    Result beginControlWindow(
        const RxContext& rx,
        ByteView& controlNonce) override;
    Result applyControllerMutation(
        const RxContext& rx,
        ByteView encodedGrant,
        ByteView controllerSecret,
        MutableByteSpan output,
        ByteView& receipt) override;
    void controllerSessionClosed(const RxContext& rx) override;

    void reset();

private:
    bool owns(const RxContext& rx) const;

    ControllerControlCoordinator& coordinator_;
    IRandom& random_;
    MutableByteSpan workspace_;
    uint32_t sessionId_;
    uint8_t transportId_;
    bool active_;

    ControllerControlEndpoint(const ControllerControlEndpoint&);
    ControllerControlEndpoint& operator=(
        const ControllerControlEndpoint&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(ControllerControlEndpoint) <= 28U,
    "Controller control endpoint exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
