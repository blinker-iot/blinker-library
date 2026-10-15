#ifndef BLINKER_WIFI_BLE_H
#define BLINKER_WIFI_BLE_H

#if defined(BLINKER_PRODUCT_WIFI) || defined(BLINKER_PRODUCT_BLE)
#error "Blinker product selectors cannot be combined"
#endif

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerWiFiBLE.h currently supports ESP32 only"
#endif

#define BLINKER_PRODUCT_WIFI_BLE 1
#include "BlinkerV2/api/BlinkerFacade.h"

namespace blinker { namespace facade_detail {
template <typename First, typename... Rest>
struct EntryProfile<First, Rest...> : LocalEntryProfile<WifiBleProfile> {};
} }

#include "BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h"

#endif
