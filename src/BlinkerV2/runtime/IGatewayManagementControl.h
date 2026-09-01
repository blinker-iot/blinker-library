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
    virtual Result handleCommand(ByteView encoded) = 0;
    virtual void poll() = 0;
    virtual ByteView pendingResult() const = 0;
    virtual void markResultPublished() = 0;
    virtual void reset() = 0;
};

} // namespace blinker

#endif
