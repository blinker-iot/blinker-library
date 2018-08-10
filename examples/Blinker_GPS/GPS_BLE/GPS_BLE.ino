#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

void setup()
{
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
        Blinker.vibrate();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);

        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    BLINKER_LOG2("GPS LONG.: ", Blinker.gps(LONG));
    BLINKER_LOG2("GPS LAT.: ", Blinker.gps(LAT));

    Blinker.delay(2000);
}