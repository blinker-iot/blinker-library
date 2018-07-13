#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin();
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        uint32_t BlinkerTime = millis();

        Blinker.beginFormat();
        Blinker.vibrate();        
        Blinker.print("millis", BlinkerTime);
        Blinker.endFormat();
    }

    if (Blinker.switchAvailable()) {
        if (Blinker.switchGet()) {
            digitalWrite(LED_BUILTIN, HIGH);

            BLINKER_LOG1("builtInSwitch on");

            Blinker.switchUpdate();
        }
        else {
            digitalWrite(LED_BUILTIN, LOW);

            BLINKER_LOG1("builtInSwitch off");

            Blinker.switchUpdate();
        }
    }
}