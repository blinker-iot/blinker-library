// Finite internal Cloud Task candidate. Private credentials and the matching
// four-slot TLS/P-256 release profile are supplied only in an ignored build.
// No Initialize, Start, store reset or periodic diagnostic operation at boot.
#include "CloudCredentials.h"
#define BLINKER_ENABLE_TASKS 1
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool begun = false;
bool currentPower = false;
uint32_t powerWrites = 0U;

void onPower(bool value) {
    currentPower = value;
    ++powerWrites;
    power.report(value);
}
uint32_t sampleReading() {
    return 42U; // Preserve the existing cloud device's scalar Manifest.
}
void observations() {
    blinker::WallClockSnapshot clock;
    const bool timeValid = Blinker.timeStatus(clock);
    Serial.printf("{\"fixture\":\"esp32-cloud-task-d4\",\"begun\":%u,"
                  "\"capacity\":%u,\"power\":%u,\"powerWrites\":%lu,"
                  "\"timeValid\":%u,\"timeState\":%u,\"millis\":%lu}\n",
                  begun, static_cast<unsigned>(BLINKER_TASK_CAPACITY), currentPower,
                  static_cast<unsigned long>(powerWrites), timeValid,
                  static_cast<unsigned>(clock.state), static_cast<unsigned long>(millis()));
}
}

void setup() {
    Serial.begin(115200);
    power.onWrite(onPower);
    uptimeSeconds.onSample(sampleReading);
    begun = Blinker.enableTasks(power) &&
        Blinker.begin(BLINKER_DEVICE_KEY, BLINKER_WIFI_SSID,
                      BLINKER_WIFI_PASSWORD, power, uptimeSeconds);
    if (begun) power.report(currentPower);
    observations();
}

void loop() {
    if (begun) Blinker.run();
    if (Serial.available() <= 0) return;
    const int command = Serial.read(); // Bounded, read-only serial observations.
    if (command == 'j') observations();
    else if (command == 'd') Blinker.printDiagnostics(Serial);
}
