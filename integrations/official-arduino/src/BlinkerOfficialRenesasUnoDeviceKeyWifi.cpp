#include "BlinkerOfficialDeviceKeyWifi.h"
#include "BlinkerOfficialDeviceKeyWifiComposition.h"
#include "BlinkerOfficialRenesasUnoWifiNetworkPlatform.h"

namespace blinker {
namespace integration {
namespace official_detail {

namespace {

typedef DeviceKeyWifiComposition<RenesasUnoWifiNetworkPlatform>
    RenesasUnoDeviceKeyWifiComposition;

RenesasUnoDeviceKeyWifiComposition& composition() {
    static RenesasUnoDeviceKeyWifiComposition instance;
    return instance;
}

} // namespace

IProductLifecycle& renesasUnoDeviceKeyWifiLifecycle(
    StringView ssid,
    StringView password,
    StringView deviceKey) {
    RenesasUnoDeviceKeyWifiComposition& selected = composition();
    (void)selected.configure(ssid, password, deviceKey);
    return selected;
}

} // namespace official_detail
} // namespace integration
} // namespace blinker
