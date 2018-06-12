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

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);
    }

    BLINKER_LOG2("Joystick X axis: ", Blinker.joystick(J_Xaxis));
    BLINKER_LOG2("Joystick Y axis: ", Blinker.joystick(J_Yaxis));

    Blinker.delay(2000);
}