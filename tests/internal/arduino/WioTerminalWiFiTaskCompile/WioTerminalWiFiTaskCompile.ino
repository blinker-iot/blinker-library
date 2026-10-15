// Compile-only fixture. Never flash this placeholder configuration to a device.
// Candidate admission/policy is supplied library-wide by the finite build gate.
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool currentPower = false;
uint32_t currentDuty = 0U;
void onPower(bool value) { currentPower = value; power.report(value); }
void onDuty(uint32_t value) { currentDuty = value; duty.report(value); }
uint32_t sampleUptime() { return millis() / 1000U; }

// Keep the existing public composition in the link, but never invoke it.
// setup/loop do not open flash, connect, Initialize, Start or execute tasks.
void compileEntry() {
    power.onWrite(onPower);
    duty.onWrite(onDuty);
    uptimeSeconds.onSample(sampleUptime);
    bool ready = Blinker.enableTime();
#if defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS
    ready = ready && Blinker.enableTasks(power, duty);
#endif
    ready = ready && Blinker.begin(
        "public-compile-placeholder-key",
        "public-compile-placeholder-ssid",
        "public-compile-placeholder-password",
        power, duty, uptimeSeconds);
    if (ready) {
        Blinker.report(power, currentPower, duty, currentDuty);
        Blinker.run();
    }
}
void (*volatile retainedCompileEntry)() = &compileEntry;
}

void setup() { (void)retainedCompileEntry; }
void loop() {}
