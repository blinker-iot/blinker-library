#ifndef BLINKER_WIFI_H
#define BLINKER_WIFI_H

#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE)
#error "Blinker product selectors cannot be combined"
#endif

#define BLINKER_PRODUCT_WIFI 1
#include "BlinkerV2/api/BlinkerFacade.h"

namespace blinker { namespace facade_detail {
template <typename First, typename... Rest>
struct EntryProfile<First, Rest...> : WifiEntryProfile {};
} }

#if defined(ARDUINO_ARCH_ESP32)
#include "BlinkerV2/arduino/platform/esp32/Esp32WiFiProduct.h"
#elif defined(ARDUINO_ARCH_RENESAS_UNO) || \
      defined(ARDUINO_ARCH_RENESAS)
#include "BlinkerV2/arduino/platform/uno_r4/UnoR4WiFiProduct.h"
#elif defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)
#include "BlinkerV2/arduino/platform/wio_terminal/WioTerminalWiFiProduct.h"
#endif

#endif
