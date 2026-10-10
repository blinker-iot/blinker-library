#include <BlinkerV2Advanced.h>

#include <string.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <Esp32WiFi.h>
typedef blinker::Esp32WifiStation ProbeWifiStation;
#elif defined(ARDUINO_ARCH_RENESAS)
#include <UnoR4WiFi.h>
typedef blinker::RenesasUnoWifiStation ProbeWifiStation;
#else
#error "CompileWifiStationAdapter supports ESP32 and Renesas targets"
#endif

ProbeWifiStation station;

void setup() {
    static const char ssid[] = "compile-probe";
    static const char password[] = "compile-password";
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(ssid), strlen(ssid));
    config.authentication = blinker::WifiAuthentication::Wpa2Personal;
    config.credential = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(password), strlen(password));
    (void)station.start(config);
    station.poll();
    station.stop();
}

void loop() {}
