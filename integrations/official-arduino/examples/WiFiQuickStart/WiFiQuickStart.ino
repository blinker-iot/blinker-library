#include <Blinker.h>

// For devices whose WiFi, ownership and cloud credential were installed
// before this Sketch starts. DIY/education devices normally use the sibling
// WiFiOnboardingQuickStart example instead.

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(blinker::WiFi, power);
}

void loop() {
    Blinker.run();
}
