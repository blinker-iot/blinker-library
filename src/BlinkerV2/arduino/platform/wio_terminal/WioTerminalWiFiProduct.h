#ifndef BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PRODUCT_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalWiFiProduct requires a Wio Terminal target"
#endif

#include "../../internal/WiFiProduct.h"
#include "WioTerminalWiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef DeviceKeyWifiProduct<WioTerminalWifiNetworkPlatform>
    WioTerminalDeviceKeyWifiProduct;

inline WioTerminalDeviceKeyWifiProduct& wioTerminalWifiProduct() {
    static WioTerminalDeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::WioTerminalDeviceKeyWifiProduct& selected =
        official_detail::wioTerminalWifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker

#endif
