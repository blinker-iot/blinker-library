#ifndef BLINKER_V2_ARDUINO_WIO_TERMINAL_BLE_PRODUCT_H
#define BLINKER_V2_ARDUINO_WIO_TERMINAL_BLE_PRODUCT_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalBLEProduct requires a Wio Terminal target"
#endif

#include "../../internal/BleProduct.h"
#include "WioTerminalBLEPlatform.h"
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef PlatformBleOnlyComposition<WioTerminalPlatformBlePlatform>
    WioTerminalPlatformBleProduct;

inline WioTerminalPlatformBleProduct& wioTerminalBleProduct() {
    static WioTerminalPlatformBleProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(facade_detail::BleProfile) {
    return official_detail::wioTerminalBleProduct();
}

} // namespace integration
} // namespace blinker

#endif
