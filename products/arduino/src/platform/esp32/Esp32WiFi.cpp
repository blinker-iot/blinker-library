#include "WiFiProduct.h"
#include "Esp32WiFiPlatform.h"

namespace blinker {
namespace integration {

namespace {

typedef official_detail::DeviceKeyWifiProduct<
    official_detail::Esp32WifiNetworkPlatform>
    Esp32DeviceKeyWifiProduct;

Esp32DeviceKeyWifiProduct& product() {
    static Esp32DeviceKeyWifiProduct instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    Esp32DeviceKeyWifiProduct& selected = product();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker
