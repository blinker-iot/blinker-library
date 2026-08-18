#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialDeviceKeyWifiProduct.h"
#include "BlinkerOfficialRenesasUnoWifiNetworkPlatform.h"

namespace blinker {
namespace integration {

namespace {

typedef official_detail::DeviceKeyWifiProduct<
    official_detail::RenesasUnoWifiNetworkPlatform>
    RenesasUnoDeviceKeyWifiComposition;

RenesasUnoDeviceKeyWifiComposition& composition() {
    static RenesasUnoDeviceKeyWifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    RenesasUnoDeviceKeyWifiComposition& selected = composition();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker
