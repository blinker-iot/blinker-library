#ifndef BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_WIFI_H
#define BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_WIFI_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalWiFi requires a Wio Terminal target"
#endif

#include <BlinkerV2/interface/IWifiStation.h>

namespace blinker {

class WioTerminalWifiStation final : public IWifiStation {
public:
    WioTerminalWifiStation();

    Result start(const WifiNetworkConfig& config) override;
    void poll() override;
    WifiStationState state() const override { return state_; }
    ErrorCode lastError() const override { return lastError_; }
    void stop() override;

private:
    void fail(ErrorCode error);

    WifiStationState state_;
    ErrorCode lastError_;
};

} // namespace blinker

#include "WioTerminalWiFi.ipp"

#endif
