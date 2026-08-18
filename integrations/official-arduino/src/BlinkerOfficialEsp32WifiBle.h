#ifndef BLINKER_OFFICIAL_ESP32_WIFI_BLE_H
#define BLINKER_OFFICIAL_ESP32_WIFI_BLE_H

#include <BlinkerV2/interface/IProductLifecycle.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Internal R4b-5B gate. R4b-7 will bind the validated product to the public
// WiFiBLE tag; ordinary sketches must not depend on this temporary seam.
IProductLifecycle& esp32WifiBleLifecycle();

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
