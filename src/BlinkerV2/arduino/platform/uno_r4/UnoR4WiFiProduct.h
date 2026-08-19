#ifndef BLINKER_V2_ARDUINO_UNO_R4_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_UNO_R4_WIFI_PRODUCT_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "UnoR4WiFiProduct requires an Arduino Renesas target"
#endif

#include "../../internal/WiFiProduct.h"
#include "UnoR4WiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef DeviceKeyWifiProduct<RenesasUnoWifiNetworkPlatform>
    RenesasUnoDeviceKeyWifiProduct;

inline RenesasUnoDeviceKeyWifiProduct& renesasUnoWifiProduct() {
    static RenesasUnoDeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::RenesasUnoDeviceKeyWifiProduct& selected =
        official_detail::renesasUnoWifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker

#endif
