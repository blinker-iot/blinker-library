#ifndef BLINKER_V2_ARDUINO_UNO_R4_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_UNO_R4_WIFI_PRODUCT_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "UnoR4WiFiProduct requires an Arduino Renesas target"
#endif

#include "../../internal/WiFiProduct.h"
#include "UnoR4WiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>
#include <BlinkerV2/api/TaskSupport.h>
#include "../../config/TaskConfig.h"
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if !defined(BLINKER_INTERNAL_UNO_R4_TASKS) || !BLINKER_INTERNAL_UNO_R4_TASKS
#error "UNO R4 tasks require internal storage and signing-profile admission"
#endif
#if defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#error "Task pilot admits manual WiFi only; combined LAN/control profile is not yet admitted"
#endif
#include "UnoR4WiFiTaskExtension.h"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
typedef DeviceKeyWifiProduct<RenesasUnoWifiNetworkPlatform,
    RenesasUnoWifiTaskExtension<DeviceKeyWifiStack<RenesasUnoWifiNetworkPlatform>, BLINKER_TASK_CAPACITY>>
#else
typedef DeviceKeyWifiProduct<RenesasUnoWifiNetworkPlatform>
#endif
    RenesasUnoDeviceKeyWifiProduct;

inline RenesasUnoDeviceKeyWifiProduct& renesasUnoWifiProduct() {
    static RenesasUnoDeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
template <>
struct TaskSupport<WifiDeviceKeyProfile> {
    template <typename... T>
    static Result configure(Property<T>&... properties) {
        return official_detail::renesasUnoWifiProduct().extension().configure(properties...);
    }
};
#endif

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::RenesasUnoDeviceKeyWifiProduct& selected =
        official_detail::renesasUnoWifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected.lifecycle();
}

} // namespace integration
} // namespace blinker

#endif
