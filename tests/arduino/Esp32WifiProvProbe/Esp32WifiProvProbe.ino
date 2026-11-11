#define BLINKER_ESP32_PROVISIONING_BLE
#include "../../local/Esp32LocalGateway.h"

#include <BlinkerWiFiBLE.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

uint32_t nextDiagnosticsAt = 0U;

void onPower(bool value) {
    Serial.print("power=");
    Serial.println(value ? "on" : "off");
    power.report(value);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Blinker.debug(Serial, blinker::LogLevel::Debug);
    power.onWrite(onPower);
    Serial.println("ESP32 WiFiProv probe starting");
    Serial.print("begin=");
    Serial.println(Blinker.begin(power));
    Serial.print("lastError=");
    Serial.println(Blinker.lastErrorText());
}

void loop() {
    Blinker.run();
    if (Serial.available()) {
        const char command = static_cast<char>(Serial.read());
        if (command == 'n') {
            Serial.print("resetNetwork=");
            Serial.println(Blinker.resetNetwork());
        } else if (command == 'r') {
            Serial.print("resetAccess=");
            Serial.println(Blinker.resetAccess());
        }
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextDiagnosticsAt) >= 0) {
        Blinker.printDiagnostics(Serial);
        nextDiagnosticsAt = now + 5000U;
    }
}
