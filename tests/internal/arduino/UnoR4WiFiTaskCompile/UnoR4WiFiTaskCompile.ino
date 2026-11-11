// Compile-only four-slot composition gate. Explicit policy values are supplied
// build-wide by the test driver, not admitted production defaults. Never upload
// this credential-free fixture or treat successful linking as storage admission.
#define BLINKER_ENABLE_TASKS 1
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));

void onPower(bool value) { power.report(value); }
void onDuty(uint32_t value) { duty.report(value); }

void setup() {
    power.onWrite(onPower);
    duty.onWrite(onDuty);
    Blinker.enableTasks(power, duty);
    Blinker.begin("compile-only-key", "compile-only-ssid", "compile-only-password", power, duty);
}
void loop() { Blinker.run(); }
