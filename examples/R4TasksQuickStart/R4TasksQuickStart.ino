// Fixed four-slot reference. Requires the maintained R4 build profile; see README.
#if !defined(ARDUINO_UNOR4_WIFI)
#error "R4TasksQuickStart requires UNO R4 WiFi"
#endif
#if !defined(BLINKER_ENABLE_TASKS) || BLINKER_ENABLE_TASKS != 1 || \
    !defined(BLINKER_INTERNAL_UNO_R4_TASKS) || BLINKER_INTERNAL_UNO_R4_TASKS != 1
#error "Select the maintained R4 task build profile; defining a Sketch macro alone is not sufficient"
#endif

#include "UserCredentials.h"
#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {
bool begun = false;

void onPower(bool value) { power.report(value); }
void onDuty(uint32_t value) { duty.report(value); }
uint32_t sampleUptime() { return millis() / 1000U; }
}

void setup() {
    power.onWrite(onPower);
    duty.onWrite(onDuty);
    uptimeSeconds.onSample(sampleUptime);

    begun = Blinker.enableTasks(power, duty) &&
        Blinker.begin(R4_DEVICE_KEY, R4_WIFI_SSID, R4_WIFI_PASSWORD,
                      power, duty, uptimeSeconds);
    if (begun) Blinker.report(power, false, duty, 0U);
}

void loop() {
    if (begun) Blinker.run();
}
