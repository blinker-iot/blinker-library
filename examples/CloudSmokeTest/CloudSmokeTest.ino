// Cloud interoperability smoke test for the single Blinker library.
#include <Blinker.h>

#ifndef BLINKER_DEVICE_KEY
#define BLINKER_DEVICE_KEY "REPLACE_WITH_DEVICE_KEY"
#endif

#ifndef BLINKER_WIFI_SSID
#define BLINKER_WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef BLINKER_WIFI_PASSWORD
#define BLINKER_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// Real Gateway/Broker smoke test using only the public Arduino API. It covers
// DeviceKey login, HELLO/Manifest/State, server COMMAND, device Report and
// lease-controlled Telemetry. Realtime sampling starts only while requested.
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));

namespace {

bool currentPower = false;
uint32_t nextDiagnosticsAt = 10000U;

void onPower(bool requested) {
    currentPower = requested;
    power.report(currentPower);
}

uint32_t sampleUptimeSeconds() {
    // Called by Blinker.run() only while an authorized realtime lease is due.
    return millis() / 1000U;
}

} // namespace

void setup() {
    Serial.begin(115200);
    Blinker.debug(Serial);
    power.onWrite(onPower);
    uptimeSeconds.onSample(sampleUptimeSeconds);

    if (!Blinker.begin(
            BLINKER_DEVICE_KEY,
            BLINKER_WIFI_SSID,
            BLINKER_WIFI_PASSWORD,
            power,
            uptimeSeconds)) {
        Serial.print("Blinker start failed: ");
        Serial.println(Blinker.lastErrorText());
    } else {
        // Initialize recoverable state once. Realtime data comes from onSample().
        power.report(currentPower);
    }
}

void loop() {
    Blinker.run();

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextDiagnosticsAt) >= 0) {
        Blinker.printDiagnostics(Serial);
        nextDiagnosticsAt = now + 10000U;
    }
}
