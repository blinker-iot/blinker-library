#ifndef BLINKER_V2_ARDUINO_PORTS_UNO_R4_WIFI_H
#define BLINKER_V2_ARDUINO_PORTS_UNO_R4_WIFI_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && !defined(ARDUINO_ARCH_RENESAS)
#error "UnoR4WiFi requires a Renesas Arduino target"
#endif

#include <BlinkerV2/interface/IWifiStation.h>

namespace blinker {

class RenesasUnoWifiStation final : public IWifiStation {
public:
    RenesasUnoWifiStation();

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

#include "UnoR4WiFi.ipp"

#endif
