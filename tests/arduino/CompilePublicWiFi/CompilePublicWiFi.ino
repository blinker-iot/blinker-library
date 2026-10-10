#include <BlinkerWiFi.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());

void setup() {
    Serial.begin(115200);
    Blinker.debug(Serial, blinker::LogLevel::Debug);
    Blinker.enableTime();
    Blinker.begin(
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8",
        "example-network",
        "example-password",
        power);
    Blinker.printDiagnostics(Serial);
}

void loop() {
    Blinker.run();
    uint64_t utcSeconds = 0U;
    (void)Blinker.time(utcSeconds);
}
