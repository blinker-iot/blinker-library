#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialDeviceKeyWifiProduct.h"
#include "BlinkerOfficialEsp32WifiNetworkPlatform.h"

namespace blinker {
namespace integration {

namespace {

typedef official_detail::DeviceKeyWifiProduct<
    official_detail::Esp32WifiNetworkPlatform>
    Esp32DeviceKeyWifiComposition;

Esp32DeviceKeyWifiComposition& composition() {
    static Esp32DeviceKeyWifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    Esp32DeviceKeyWifiComposition& selected = composition();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker
