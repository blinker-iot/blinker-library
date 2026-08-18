#ifndef BLINKER_OFFICIAL_RENESAS_UNO_BLE_H
#define BLINKER_OFFICIAL_RENESAS_UNO_BLE_H

#include "BlinkerOfficialBleProduct.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Internal R4b-6C package/resource seams. The public BLE facade remains
// withheld until target hardware gates and the R4b-7 API freeze complete.
IProductLifecycle& renesasUnoPlatformBleLifecycle();
ILocalBleComposition& renesasUnoLocalBleLifecycle();

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
