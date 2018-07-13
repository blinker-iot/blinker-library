#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

bool freshed = false;

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

    if (Blinker.builtInSwitch()) {
        digitalWrite(LED_BUILTIN, HIGH);

        if (!freshed) {
            BLINKER_LOG1("builtInSwitch on");
            freshed = true;
        }
    }
    else {
        digitalWrite(LED_BUILTIN, LOW);

        if (freshed) {
            BLINKER_LOG1("builtInSwitch off");
            freshed = false;
        }
    }
}