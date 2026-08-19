// BLE QuickStart for every supported board in the single Blinker library.
#include <BlinkerBLE.h>

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
