// BLE QuickStart for every supported board in the single Blinker library.
#include <BlinkerBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));
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

uint32_t sampleUptimeSeconds() {
    // Sample only while an authorized realtime lease is due.
    return millis() / 1000U;
}

void setup() {
    power.onWrite(onPower);
    button.onInvoke(onButton);
    uptimeSeconds.onSample(sampleUptimeSeconds);
    if (Blinker.begin(power, buttonPresses, button, uptimeSeconds)) {
        Blinker.report(power, currentPower, buttonPresses, pressCount);
    }
}

void loop() {
    Blinker.run();
}
