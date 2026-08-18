#ifndef BLINKER_API_WIFI_DEVICE_KEY_PROFILE_H
#define BLINKER_API_WIFI_DEVICE_KEY_PROFILE_H

#include "../core/Span.h"

namespace blinker {

// Borrowed manual/education WiFi bootstrap input. DeviceKey remains the
// long-lived, refreshable device credential; the product composition copies
// only what it needs before begin() returns.
class WifiDeviceKeyProfile {
public:
    WifiDeviceKeyProfile(
        StringView deviceKey,
        StringView ssid,
        StringView password)
        : deviceKey_(deviceKey), ssid_(ssid), password_(password) {}

    StringView deviceKey() const { return deviceKey_; }
    StringView ssid() const { return ssid_; }
    StringView password() const { return password_; }

private:
    StringView deviceKey_;
    StringView ssid_;
    StringView password_;
};

} // namespace blinker

#endif
