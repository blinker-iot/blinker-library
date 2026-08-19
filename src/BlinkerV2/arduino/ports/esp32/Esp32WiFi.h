#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_WIFI_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_WIFI_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32WiFi requires an ESP32 Arduino target"
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

#include "Esp32WiFi.ipp"

#endif
