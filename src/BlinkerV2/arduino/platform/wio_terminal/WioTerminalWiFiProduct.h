#ifndef BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PRODUCT_H
#define BLINKER_V2_ARDUINO_WIO_TERMINAL_WIFI_PRODUCT_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalWiFiProduct requires a Wio Terminal target"
#endif

#include "../../internal/WiFiProduct.h"
#include "WioTerminalWiFiPlatform.h"
#include <BlinkerV2/api/WifiDeviceKeyProfile.h>
#include <BlinkerV2/api/TaskSupport.h>
#include "../../config/TaskConfig.h"
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
#if !defined(BLINKER_INTERNAL_WIO_TASKS) || BLINKER_INTERNAL_WIO_TASKS != 1
#error "Wio tasks require internal storage and signing-profile admission"
#endif
#if !defined(BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED) || BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1
#error "Wio tasks require an explicitly reserved task QSPI region"
#endif
#if BLINKER_TASK_CAPACITY != 4
#error "Wio task candidate admits four slots only"
#endif
#if defined(BLINKER_PRODUCT_BLE) || defined(BLINKER_PRODUCT_WIFI_BLE) || \
    (defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \
    (defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET)
#error "Wio task candidate admits manual WiFi only"
#endif
#if BLINKER_OFFICIAL_CLOUD_SESSION_TLS || BLINKER_OFFICIAL_MQTT_TLS
#error "Wio task candidate admits the existing plain WiFi profile only"
#endif
#include "WioTerminalWiFiTaskExtension.h"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
static_assert(BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL &&
    BLINKER_DEVICE_FRAME_SIZE == 256,
    "Wio task candidate requires the reviewed Small256 profile");
static_assert(kDeviceKeyMqttPacketBufferSize == 361U &&
    MQTT_MAX_PACKET_SIZE == kDeviceKeyMqttPacketBufferSize,
    "Wio task candidate requires consistent initial MQTT packet size");
typedef DeviceKeyWifiProduct<WioTerminalWifiNetworkPlatform,
    WioTerminalWifiTaskExtension<DeviceKeyWifiStack<WioTerminalWifiNetworkPlatform>, BLINKER_TASK_CAPACITY>>
#else
typedef DeviceKeyWifiProduct<WioTerminalWifiNetworkPlatform>
#endif
    WioTerminalDeviceKeyWifiProduct;

inline WioTerminalDeviceKeyWifiProduct& wioTerminalWifiProduct() {
    static WioTerminalDeviceKeyWifiProduct instance;
    return instance;
}

} // namespace official_detail

#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
template <>
struct TaskSupport<WifiDeviceKeyProfile> {
    template <typename... T>
    static Result configure(Property<T>&... properties) {
        return official_detail::wioTerminalWifiProduct().extension().configure(properties...);
    }
};
#endif

inline IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    official_detail::WioTerminalDeviceKeyWifiProduct& selected =
        official_detail::wioTerminalWifiProduct();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected.lifecycle();
}

} // namespace integration
} // namespace blinker

#endif
