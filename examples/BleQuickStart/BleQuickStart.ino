// BLE QuickStart for every supported board in the single Blinker library.
#include <BlinkerBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

bool currentPower = false;
uint32_t pressCount = 0;

void onPower(bool value) {
    currentPower = value;
    power.report(currentPower);
}

void onButton() {
    buttonPresses.report(++pressCount);
}

void setup() {
    power.onWrite(onPower);
    button.onInvoke(onButton);
    if (Blinker.begin(power, buttonPresses, button)) {
        Blinker.report(power, currentPower, buttonPresses, pressCount);
    }
}

void loop() {
    Blinker.run();
}
