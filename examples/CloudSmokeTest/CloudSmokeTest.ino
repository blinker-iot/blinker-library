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
    power.onWrite(onPower);
    uptimeSeconds.onSample(sampleUptimeSeconds);

    if (Blinker.enableTime() && Blinker.begin(
            BLINKER_DEVICE_KEY,
            BLINKER_WIFI_SSID,
            BLINKER_WIFI_PASSWORD,
            power,
            uptimeSeconds)) {
        // Initialize recoverable state once. Realtime data comes from onSample().
        power.report(currentPower);
    }
}

void loop() {
    Blinker.run();

    // Send 'd' once from Serial Monitor for diagnostics and business UTC.
    // Print is synchronous: keep the monitor reading while requesting output.
    // No periodic dump/debug output can stall an unattended device's run().
    if (Serial.available() > 0 && Serial.read() == 'd') {
        Blinker.printDiagnostics(Serial);
        uint64_t utcSeconds = 0U;
        Serial.print("Business UTC: ");
        if (Blinker.time(utcSeconds)) {
            // The admitted 2000..2100 UTC range fits an Arduino unsigned long.
            Serial.println(static_cast<unsigned long>(utcSeconds));
        } else {
            Serial.println("unsynchronized");
        }
    }
}
