#ifndef BLINKER_OFFICIAL_ESP32_BLE_H
#define BLINKER_OFFICIAL_ESP32_BLE_H

#include "BlinkerOfficialBleProduct.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Internal R4b-6C package/resource seams. R4b-7 may bind validated products
// to the public BLE facade; ordinary sketches must not use these accessors.
IProductLifecycle& esp32PlatformBleLifecycle();
ILocalBleComposition& esp32LocalBleLifecycle();

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
