// Compile-only, isolated manual WiFi product. Not a hardware credential or an
// advertised production example. Set both gates for ALL compilation units:
// -DBLINKER_INTERNAL_LOCAL_ACCESS_CONTROL=1 -DBLINKER_INTERNAL_LAN_WEBSOCKET=1
// LAN defaults to Plain; add -DBLINKER_INTERNAL_LAN_ENCRYPTED=1 to test Noise.
#if BLINKER_INTERNAL_LAN_WEBSOCKET && !BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#error "LAN requires the documented shared-authority build flag"
#endif
// Omitting both flags builds the identical Sketch as a WiFi-only size/control.
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
void onPower(bool value) { power.report(value); }
void setup() {
    power.onWrite(onPower);
    Blinker.begin("COMPILE_ONLY_DEVICE_KEY", "COMPILE_ONLY_SSID", "COMPILE_ONLY_PASSWORD", power);
}
void loop() { Blinker.run(); }
