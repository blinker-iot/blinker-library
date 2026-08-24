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
#include "BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h"

#endif
