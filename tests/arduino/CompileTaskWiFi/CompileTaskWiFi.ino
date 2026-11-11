// Compile against a maintainer-admitted candidate; source defaults are closed.
// No user-side capacity/policy, internal composition, or workspace is needed.
#define BLINKER_ENABLE_TASKS 1
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
void onPower(bool value) { power.report(value); }

void setup() {
    power.onWrite(onPower);
    if (!Blinker.enableTasks(power)) return;
    Blinker.begin("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8",
                  "compile-only.invalid", "", power);
}
void loop() { Blinker.run(); }
