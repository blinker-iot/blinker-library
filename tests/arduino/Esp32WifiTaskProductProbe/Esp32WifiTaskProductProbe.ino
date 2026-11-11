// COMPILE ONLY: real public WiFi graph, no network/Flash I/O while unarmed.
// Maintainer measurement policy, NOT production endurance admission.
#ifndef BLINKER_TASK_CAPACITY
#define BLINKER_TASK_CAPACITY 0
#endif
#if BLINKER_TASK_CAPACITY
#define BLINKER_ENABLE_TASKS 1
#endif
#define BLINKER_TASK_WINDOW_MS 60000U
#define BLINKER_TASK_WRITES 2U
#define BLINKER_TASK_MIN_START_MS 60000U
#define BLINKER_TASK_MAX_COMMIT_MS 100U
// The policy above retains code for measurement, NOT production endurance admission.
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
volatile bool blinkerTaskProductProbeArmed = false;
volatile bool blinkerTaskProductProbeStarted = false;
static void onPower(bool value) { power.report(value); }

void setup() {
    if (!blinkerTaskProductProbeArmed) return;
    power.onWrite(onPower);
#if BLINKER_TASK_CAPACITY
    if (!Blinker.enableTasks(power)) return;
#else
    Blinker.enableTime();
#endif
    // Public vector, deliberately unusable SSID. Never a real device credential.
    blinkerTaskProductProbeStarted = Blinker.begin(
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8", "compile-only.invalid", "", power);
}
void loop() {
    if (blinkerTaskProductProbeArmed && blinkerTaskProductProbeStarted) Blinker.run();
}
