// ESP32 WiFi+BLE QuickStart. The App installs WiFi and device access data.
// SoftAP WiFiProv is the portable default. On an ESP32 core built with IDF
// NimBLE, define BLINKER_ESP32_PROVISIONING_BLE before this include to opt in
// to BLE WiFiProv without changing begin().
// #define BLINKER_ESP32_PROVISIONING_BLE
#include <BlinkerWiFiBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(power);
}

void loop() {
    Blinker.run();
}
