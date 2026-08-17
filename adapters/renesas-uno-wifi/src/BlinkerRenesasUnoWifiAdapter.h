#ifndef BLINKER_RENESAS_UNO_WIFI_ADAPTER_H
#define BLINKER_RENESAS_UNO_WIFI_ADAPTER_H

#if !defined(ARDUINO_ARCH_RENESAS)
#error "BlinkerRenesasUnoWifiAdapter requires a Renesas Arduino target"
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

#endif
