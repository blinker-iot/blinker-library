#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

void switch_callback(const String & state)
{
    BLINKER_LOG2("get switch state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);
        BUILTIN_SWITCH.print("on");
    }
    else {
        digitalWrite(LED_BUILTIN, LOW);
        BUILTIN_SWITCH.print("off");
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin();
    
    BUILTIN_SWITCH.attach(switch_callback);
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
