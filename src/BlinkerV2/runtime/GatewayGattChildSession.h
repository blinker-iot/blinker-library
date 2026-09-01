#ifndef BLINKER_RUNTIME_GATEWAYGATTCHILDSESSION_H
#define BLINKER_RUNTIME_GATEWAYGATTCHILDSESSION_H

#include "GatewayChildSession.h"
#include "GatewayPresenceMatcher.h"
#include "../transport/GattDirectChildLink.h"

namespace blinker {

// GATT is one composition of the bearer-neutral native secure session. A
// future ESP-NOW or serial-NCP implementation supplies another Link/Selector
// pair without changing Method 2, DirectSecure or the cloud lifecycle.
typedef BasicGatewayChildSession<
    GattDirectChildLink,
    GatewayPresenceMatcher> GatewayGattChildSession;

} // namespace blinker

#endif
