#ifndef BLINKER_V2_ARDUINO_ESP32_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_WIFI_PRODUCT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32WiFiProduct requires an ESP32 Arduino target"
#endif

#include "../../internal/WiFiProduct.h"
#include "Esp32WiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>

namespace blinker {
namespace integration {
namespace official_detail {

typedef DeviceKeyWifiProduct<Esp32WifiNetworkPlatform>
    Esp32DeviceKeyWifiProduct;

inline Esp32DeviceKeyWifiProduct& esp32WifiProduct() {
    static Esp32DeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::Esp32DeviceKeyWifiProduct& selected =
        official_detail::esp32WifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker

#endif
