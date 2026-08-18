#include "WiFiProduct.h"
#include "UnoR4WiFiPlatform.h"

namespace blinker {
namespace integration {

namespace {

typedef official_detail::DeviceKeyWifiProduct<
    official_detail::RenesasUnoWifiNetworkPlatform>
    RenesasUnoDeviceKeyWifiProduct;

RenesasUnoDeviceKeyWifiProduct& product() {
    static RenesasUnoDeviceKeyWifiProduct instance;
    return instance;
}

} // namespace

IProductLifecycle& lifecycle(const WifiDeviceKeyProfile& profile) {
    RenesasUnoDeviceKeyWifiProduct& selected = product();
    (void)selected.configure(
        profile.ssid(),
        profile.password(),
        profile.deviceKey());
    return selected;
}

} // namespace integration
} // namespace blinker
