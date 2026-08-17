#ifndef BLINKER_INTERFACE_IWIFISTATION_H
#define BLINKER_INTERFACE_IWIFISTATION_H

#include "IWifiCredentialSink.h"

namespace blinker {

enum class WifiStationState : uint8_t {
    Stopped = 0,
    Connecting,
    Connected,
    Failed
};

// Cooperative platform boundary for an infrastructure WiFi station. start()
// may perform the bounded radio/IPC work required to initiate a connection,
// but must not run an association, DHCP or reachability retry loop. poll() may
// perform one bounded platform status transaction. Implementations must expose
// terminal connection failures through state()/lastError(), synchronously
// consume config, and never retain a borrowed config view after start()
// returns.
class IWifiStation {
public:
    virtual ~IWifiStation() {}

    virtual Result start(const WifiNetworkConfig& config) = 0;
    virtual void poll() = 0;
    virtual WifiStationState state() const = 0;
    virtual ErrorCode lastError() const = 0;
    virtual void stop() = 0;
};

} // namespace blinker

#endif
