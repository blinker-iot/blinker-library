// WiFi QuickStart for every supported board in the single Blinker library.
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(
        "REPLACE_WITH_DEVICE_KEY",
        "YOUR_WIFI_SSID",
        "YOUR_WIFI_PASSWORD",
        power);
}

void loop() {
    Blinker.run();
}
