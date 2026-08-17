#ifndef BLINKER_INTERFACE_IWIFICREDENTIALSINK_H
#define BLINKER_INTERFACE_IWIFICREDENTIALSINK_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class WifiAuthentication : uint8_t {
    Open = 0,
    Wpa2Personal = 1,
    Wpa3Personal = 2,
    Wpa2Wpa3Personal = 3
};

struct WifiNetworkConfig {
    ByteView ssid;
    WifiAuthentication authentication;
    ByteView credential;
    bool hidden;

    WifiNetworkConfig()
        : authentication(WifiAuthentication::Open), hidden(false) {}
};

// Platform-owned atomic boundary for WiFi credentials. commit() must consume
// the borrowed views synchronously, return success only after the new profile
// is durably recoverable, and leave the previous profile usable on failure.
// Core never retains SSID or credential bytes after this call.
class IWifiCredentialSink {
public:
    virtual ~IWifiCredentialSink() {}
    virtual Result commit(const WifiNetworkConfig& config) = 0;
};

} // namespace blinker

#endif
