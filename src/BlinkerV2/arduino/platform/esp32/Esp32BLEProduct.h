#ifndef BLINKER_V2_ARDUINO_ESP32_BLE_PRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_BLE_PRODUCT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32BLEProduct requires an ESP32 Arduino target"
#endif

#include "../../internal/BleProduct.h"
#include "Esp32BLEPlatform.h"
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef PlatformBleOnlyComposition<Esp32PlatformBlePlatform>
    Esp32PlatformBleProduct;

inline Esp32PlatformBleProduct& esp32BleProduct() {
    static Esp32PlatformBleProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(facade_detail::BleProfile) {
    return official_detail::esp32BleProduct();
}

} // namespace integration
} // namespace blinker

#endif
