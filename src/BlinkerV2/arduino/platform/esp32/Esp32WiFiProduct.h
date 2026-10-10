#ifndef BLINKER_V2_ARDUINO_ESP32_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_WIFI_PRODUCT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32WiFiProduct requires an ESP32 Arduino target"
#endif

#include "../../internal/WiFiProduct.h"
#include "Esp32WiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>
#include <BlinkerV2/api/TaskSupport.h>
#include "../../config/TaskConfig.h"
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#error "Task pilot admits manual WiFi only; combined LAN/control profile is not yet admitted"
#endif
#include "Esp32WiFiTaskExtension.h"
#endif
#if defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET
#include "Esp32WiFiLanExtension.h"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
typedef DeviceKeyWifiProduct<Esp32WifiNetworkPlatform,
    Esp32WifiTaskExtension<DeviceKeyWifiStack<Esp32WifiNetworkPlatform>, BLINKER_TASK_CAPACITY> >
#elif defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET
typedef DeviceKeyWifiProduct<Esp32WifiNetworkPlatform,
    Esp32WifiLanExtension<DeviceKeyWifiStack<Esp32WifiNetworkPlatform> > >
#else
typedef DeviceKeyWifiProduct<Esp32WifiNetworkPlatform>
#endif
    Esp32DeviceKeyWifiProduct;

inline Esp32DeviceKeyWifiProduct& esp32WifiProduct() {
    static Esp32DeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
template <>
struct TaskSupport<WifiDeviceKeyProfile> {
    template <typename... T>
    static Result configure(Property<T>&... properties) {
        return official_detail::esp32WifiProduct().extension().configure(properties...);
    }
};
#endif

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::Esp32DeviceKeyWifiProduct& selected =
        official_detail::esp32WifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected.lifecycle();
}

} // namespace integration
} // namespace blinker

#endif
