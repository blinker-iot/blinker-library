// Cloud interoperability smoke test for the single Blinker library.
#include <BlinkerWiFi.h>

// Real Gateway/Broker smoke test using only the public Arduino API. It covers
// DeviceKey login, HELLO/Manifest/State, server COMMAND and device Report.
// The zero-dependency wire probe does not run this full state machine.
BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::readOnly());

namespace {

bool currentPower = false;
uint32_t nextReportAt = 0U;

void onPower(bool requested) {
    currentPower = requested;
    power.report(currentPower);
}

} // namespace

void setup() {
    Serial.begin(115200);
    power.onWrite(onPower);

    if (!Blinker.begin(
            "REPLACE_WITH_DEVICE_KEY",
            "YOUR_WIFI_SSID",
            "YOUR_WIFI_PASSWORD",
            power,
            uptimeSeconds)) {
        Serial.print("Blinker start failed: ");
        Serial.println(Blinker.lastErrorText());
    }
}

void loop() {
    Blinker.run();

    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextReportAt) >= 0) {
        Blinker.report(
            power,
            currentPower,
            uptimeSeconds,
            now / 1000U);
        nextReportAt = now + 5000U;
    }
}
