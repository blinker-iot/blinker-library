// Official BLE product example.
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void onPower(bool value) {
    power.report(value);
}

void setup() {
    power.onWrite(onPower);
    Blinker.begin(blinker::BLE, power);
}

void loop() {
    Blinker.run();
}
