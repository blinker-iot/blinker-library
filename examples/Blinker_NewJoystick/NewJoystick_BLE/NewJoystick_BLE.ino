#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

#define JOY_1 "JOYKey"

BlinkerJoystick JOY1(JOY_1);

void joystick1_callback(uint8_t xAxis, uint8_t yAxis)
{
    BLINKER_LOG2("Joystick1 X axis: ", xAxis);
    BLINKER_LOG2("Joystick1 Y axis: ", yAxis);
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin();

    JOY1.attach(joystick1_callback);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);
    }
}
