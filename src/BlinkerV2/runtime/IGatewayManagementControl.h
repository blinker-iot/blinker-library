#ifndef BLINKER_RUNTIME_IGATEWAYMANAGEMENTCONTROL_H
#define BLINKER_RUNTIME_IGATEWAYMANAGEMENTCONTROL_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

// One bounded handler behind the authenticated Edge Hub management topic.
// Proof, revoke and future rotate coordinators remain independent of MQTT.
class IGatewayManagementControl {
public:
    virtual ~IGatewayManagementControl() {}
    static void controllerControlResponseThunk(
        void* context,
        ByteView encoded) {
        IGatewayManagementControl* control =
            static_cast<IGatewayManagementControl*>(context);
        if (control != nullptr) {
            control->handleControllerControlResponse(encoded);
        }
    }
    virtual Result handleCommand(ByteView encoded) = 0;
    // Optional southbound ControllerControl response. Coordinators that do
    // not use the child management side channel deliberately ignore it.
    virtual void handleControllerControlResponse(ByteView) {}
    virtual void poll() = 0;
    virtual ByteView pendingResult() const = 0;
    virtual void markResultPublished() = 0;
    virtual void reset() = 0;
};

} // namespace blinker

#endif
