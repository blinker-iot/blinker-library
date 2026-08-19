#ifndef BLINKER_V2_ARDUINO_UNO_R4_BLE_PRODUCT_H
#define BLINKER_V2_ARDUINO_UNO_R4_BLE_PRODUCT_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "UnoR4BLEProduct requires an Arduino Renesas target"
#endif

#include "../../internal/BleProduct.h"
#include "UnoR4BLEPlatform.h"
#include <BlinkerV2/api/BlinkerFacade.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef PlatformBleOnlyComposition<RenesasUnoPlatformBlePlatform>
    RenesasUnoPlatformBleProduct;

inline RenesasUnoPlatformBleProduct& renesasUnoBleProduct() {
    static RenesasUnoPlatformBleProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(facade_detail::BleProfile) {
    return official_detail::renesasUnoBleProduct();
}

} // namespace integration
} // namespace blinker

#endif
