#ifndef BLINKER_OFFICIAL_DEVICE_KEY_WIFI_H
#define BLINKER_OFFICIAL_DEVICE_KEY_WIFI_H

#include <BlinkerV2/api/WifiDeviceKeyProfile.h>
#include <BlinkerV2/interface/IProductLifecycle.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Provisioning variants remain internal until their public begin() contracts
// and APP/service endpoints are frozen. Manual DeviceKey WiFi binds directly
// to integration::lifecycle(const WifiDeviceKeyProfile&).
IProductLifecycle& esp32ProvisionedDeviceKeyWifiLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    bool forceProvisioning = false);
IProductLifecycle& esp32SoftApProvisionedDeviceKeyWifiBleLifecycle(
    StringView serviceName,
    StringView proofOfPossession,
    StringView serviceKey,
    bool forceProvisioning = false);
} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
