#if defined(BLINKER_DEMO_WIFI_BLE)
#include <BlinkerWiFiBLE.h>
#else
#include <Blinker.h>
#endif

#include <WiFi.h>

#if !defined(BLINKER_DEMO_WIFI_BLE)
#include "secrets.h"
#endif

// Controls drive the onboard addressable RGB LED.
BLINKER_PROPERTY(demoPower, bool, blinker::readWrite());
BLINKER_PROPERTY(demoBrightness, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0, 100, 1, "%"));
BLINKER_PROPERTY(demoLabel, blinker::StringView,
    blinker::textProperty(blinker::PropertyMode::ReadWrite, 24));
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_ACTION(pressButton);

// Actual board measurements, not simulated sensor readings.
BLINKER_PROPERTY(wifiRssi, int32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadOnly, -127, 0, 1, "dBm"));
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::readOnly());
BLINKER_PROPERTY(freeHeapBytes, uint32_t, blinker::readOnly());
BLINKER_PROPERTY(totalHeapBytes, uint32_t, blinker::readOnly());

bool currentPower = false;
uint32_t currentBrightness = 50;
uint32_t presses = 0;
uint32_t lastReportMs = 0;
bool started = false;
bool flashing = false;
uint32_t flashStartedMs = 0;

void applyLed() {
    const uint8_t intensity = currentPower ? currentBrightness * 255U / 100U : 0;
    rgbLedWrite(RGB_BUILTIN, 0, 0, intensity);
}

void onPower(bool value) {
    currentPower = value;
    if (!flashing) applyLed();
    demoPower.report(currentPower);
}

void onBrightness(uint32_t value) {
    currentBrightness = value;
    if (!flashing) applyLed();
    demoBrightness.report(currentBrightness);
}

void onLabel(blinker::StringView value) {
    demoLabel.report(value);
}

void onPressButton() {
    flashing = true;
    flashStartedMs = millis();
    rgbLedWrite(RGB_BUILTIN, 64, 64, 64);

    buttonPresses.report(++presses);
}

void reportMeasurements() {
    uptimeSeconds.report(millis() / 1000U);
    freeHeapBytes.report(ESP.getFreeHeap());
    totalHeapBytes.report(ESP.getHeapSize());

    if (WiFi.status() == WL_CONNECTED) {
        wifiRssi.report(WiFi.RSSI());
    }
}

void setup() {
    Serial.begin(115200);
    applyLed();

    demoPower.onWrite(onPower);
    demoBrightness.onWrite(onBrightness);
    demoLabel.onWrite(onLabel);
    pressButton.onInvoke(onPressButton);

    started = Blinker.begin(
#if !defined(BLINKER_DEMO_WIFI_BLE)
        DEMO_DEVICE_KEY, DEMO_WIFI_SSID, DEMO_WIFI_PASSWORD,
#endif
        demoPower, demoBrightness, demoLabel, buttonPresses, pressButton,
        wifiRssi, uptimeSeconds, freeHeapBytes, totalHeapBytes);

    if (!started) {
        Serial.println("Blinker initialization failed; send d for diagnostics.");
        return;
    }

    demoPower.report(currentPower);
    demoBrightness.report(currentBrightness);
    demoLabel.report(blinker::StringView("ESP32-S3 demo"));
    buttonPresses.report(presses);
    reportMeasurements();
}

void loop() {
    Blinker.run();

    const uint32_t now = millis();
    if (flashing && now - flashStartedMs >= 250U) {
        flashing = false;
        applyLed();
    }

    if (started && now - lastReportMs >= 5000U) {
        lastReportMs = now;
        reportMeasurements();
    }

    if (Serial.available() > 0 && Serial.read() == 'd') {
        Blinker.printDiagnostics(Serial);
    }
}
