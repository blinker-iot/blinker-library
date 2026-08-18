// Official cloud interoperability smoke test.
#include <Blinker.h>

// Minimal APP/service/broker smoke test. It uses only the public Arduino API
// while exercising DeviceKey session creation and BBP/2 state traffic.
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
            blinker::WiFi,
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
