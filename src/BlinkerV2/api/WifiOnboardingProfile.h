#ifndef BLINKER_API_WIFIONBOARDINGPROFILE_H
#define BLINKER_API_WIFIONBOARDINGPROFILE_H

#include "../core/Span.h"

namespace blinker {

// Borrowed, headless WiFi bootstrap input. The official composition validates
// and durably persists the network during begin(). The EnrollmentKey
// remains an opaque one-time server capability and is never a device ID.
class WifiOnboardingProfile {
public:
    StringView ssid() const { return ssid_; }
    StringView password() const { return password_; }
    StringView enrollmentKey() const { return enrollmentKey_; }
    bool openNetwork() const { return openNetwork_; }

private:
    WifiOnboardingProfile(
        StringView ssid,
        StringView password,
        StringView enrollmentKey,
        bool openNetwork)
        : ssid_(ssid),
          password_(password),
          enrollmentKey_(enrollmentKey),
          openNetwork_(openNetwork) {}

    StringView ssid_;
    StringView password_;
    StringView enrollmentKey_;
    bool openNetwork_;

    friend struct WiFiTag;
};

} // namespace blinker

#endif
