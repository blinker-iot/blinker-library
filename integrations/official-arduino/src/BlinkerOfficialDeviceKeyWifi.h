#ifndef BLINKER_OFFICIAL_DEVICE_KEY_WIFI_H
#define BLINKER_OFFICIAL_DEVICE_KEY_WIFI_H

#include <BlinkerV2/core/Span.h>
#include <BlinkerV2/interface/IProductLifecycle.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Internal compile/resource seams. These are not release API.
IProductLifecycle& esp32DeviceKeyWifiLifecycle(
    StringView ssid,
    StringView password,
    StringView deviceKey);
IProductLifecycle& esp32ProvisionedDeviceKeyWifiLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    bool forceProvisioning = false);
IProductLifecycle& esp32SoftApProvisionedDeviceKeyWifiBleLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    StringView serviceKey,
    bool forceProvisioning = false);
IProductLifecycle& renesasUnoDeviceKeyWifiLifecycle(
    StringView ssid,
    StringView password,
    StringView deviceKey);

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
