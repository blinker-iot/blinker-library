#ifndef BLINKER_ESP32_WIFI_ADAPTER_H
#define BLINKER_ESP32_WIFI_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32WifiAdapter requires an ESP32 Arduino target"
#endif

#include <BlinkerV2/interface/IWifiStation.h>

namespace blinker {

class Esp32WifiStation final : public IWifiStation {
public:
    Esp32WifiStation();

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
