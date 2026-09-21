#ifndef BLINKER_RUNTIME_IGATEWAYMANAGEMENTCONTROL_H
#define BLINKER_RUNTIME_IGATEWAYMANAGEMENTCONTROL_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

// Physical policy only; the coordinator remains the authority for whether an
// incoming command continues its existing bounded transaction.
class IGatewayManagementAdmission {
public:
    virtual ~IGatewayManagementAdmission() {}
    virtual bool allowNewManagementWork() = 0;
};

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
    virtual bool continuesCommand(ByteView) const { return false; }
    // Optional southbound ControllerControl response. Coordinators that do
    // not use the child management side channel deliberately ignore it.
    virtual void handleControllerControlResponse(ByteView) {}
    virtual void poll() = 0;
    // Outstanding work/lease, independently of whether a reply is queued.
    // A published Ready/ACK does not release ownership to another family.
    virtual bool busy() const = 0;
    // Native child-session ownership is NOT pendingResult()/busy(). A Prepare
    // result may be published while its exact nonce session is still needed.
    // Permit-join reserves the same radio through its separate adapter;
    // Opening/Closing (including after reset) still exclude another owner.
    virtual bool ownsChildSession() const { return false; }
    virtual ByteView pendingResult() const = 0;
    virtual void markResultPublished() = 0;
    virtual void reset() = 0;
};

} // namespace blinker

#endif
