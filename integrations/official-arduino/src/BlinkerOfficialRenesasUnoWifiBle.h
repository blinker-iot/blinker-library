#ifndef BLINKER_OFFICIAL_RENESAS_UNO_WIFI_BLE_H
#define BLINKER_OFFICIAL_RENESAS_UNO_WIFI_BLE_H

#include <BlinkerV2/interface/IProductLifecycle.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Internal R4b-5C gate. The public WiFiBLE facade remains withheld until
// hardware pairing, encryption-loss and radio coexistence tests pass.
IProductLifecycle& renesasUnoWifiBleLifecycle();

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
