#ifndef BLINKER_V2_ARDUINO_ESP32_WIFI_PROV_OPTIONS_H
#define BLINKER_V2_ARDUINO_ESP32_WIFI_PROV_OPTIONS_H

// Optional firmware-provided Security1 PoP. Define before the product include
// using a string literal or static-lifetime text; never derive it from a
// DeviceKey, SSID or WiFi password. Ordinary products keep their no-PoP default.
#ifndef BLINKER_V2_WIFI_PROV_POP
#define BLINKER_V2_WIFI_PROV_POP nullptr
#endif

namespace blinker {
namespace official {

inline constexpr const char* wifiProvProofOfPossession() {
    // Preserve an explicitly empty value: the adapter must reject it rather
    // than silently falling back to the ordinary no-PoP profile.
    return BLINKER_V2_WIFI_PROV_POP;
}

} // namespace official
} // namespace blinker

#endif
