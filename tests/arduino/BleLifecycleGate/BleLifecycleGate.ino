// Hardware-only lifecycle gate. Never resets access automatically at boot.
// Use only a dedicated test board; 'r' irreversibly clears its access root.
#include <BlinkerBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(button);

uint32_t pressCount = 0;

void onPower(bool value) {
    power.report(value);
}

void onButton() {
    buttonPresses.report(++pressCount);
}

void setup() {
    Serial.begin(115200);
    // USB CDC writes can block when no monitor drains the port. Keep BLE
    // timing independent of diagnostic output; inspect only on command.
    power.onWrite(onPower);
    button.onInvoke(onButton);
    if (Blinker.begin(power, buttonPresses, button)) {
        Blinker.report(power, false, buttonPresses, pressCount);
        Serial.println("BLE_LIFECYCLE_READY");
    } else Serial.println("BLE_LIFECYCLE_BEGIN_FAILED");
}

void loop() {
    Blinker.run();
    if (!Serial.available()) return;
    const int command = Serial.read();
    if (command == 'r') {
        Serial.println(Blinker.resetAccess() ? "ACCESS_RESET_OK" : "ACCESS_RESET_FAILED");
    } else if (command == 's') {
        Serial.print("LOCAL_READY=");
        Serial.println(Blinker.status().localReady ? 1 : 0);
        Serial.print("BUTTON_COUNT=");
        Serial.println(pressCount);
    }
}
