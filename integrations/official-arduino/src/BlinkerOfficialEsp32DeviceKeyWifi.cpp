#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialDeviceKeyWifiComposition.h"
#include "BlinkerOfficialEsp32WifiNetworkPlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

typedef DeviceKeyWifiComposition<Esp32WifiNetworkPlatform>
    Esp32DeviceKeyWifiComposition;

Esp32DeviceKeyWifiComposition& composition() {
    static Esp32DeviceKeyWifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& esp32DeviceKeyWifiLifecycle(
    StringView ssid,
    StringView password,
    StringView deviceKey) {
    Esp32DeviceKeyWifiComposition& selected = composition();
    (void)selected.configure(ssid, password, deviceKey);
    return selected;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
