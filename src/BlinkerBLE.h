#ifndef BLINKER_BLE_H
#define BLINKER_BLE_H

#if defined(BLINKER_PRODUCT_WIFI) || defined(BLINKER_PRODUCT_WIFI_BLE)
#error "Blinker product selectors cannot be combined"
#endif

#define BLINKER_PRODUCT_BLE 1
#include "BlinkerV2/api/BlinkerFacade.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "BlinkerV2/arduino/platform/esp32/Esp32BLEProduct.h"
#elif defined(ARDUINO_ARCH_RENESAS_UNO) || \
      defined(ARDUINO_ARCH_RENESAS)
#include "BlinkerV2/arduino/platform/uno_r4/UnoR4BLEProduct.h"
#elif defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL)
#include "BlinkerV2/arduino/platform/wio_terminal/WioTerminalBLEProduct.h"
#endif

#endif
