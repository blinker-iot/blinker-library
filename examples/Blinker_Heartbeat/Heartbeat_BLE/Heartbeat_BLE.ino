#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

bool switch_state = false;

void switch_callback(const String & state)
{
    BLINKER_LOG2("get switch state: ", state);

    if (state == BLINKER_CMD_ON) {
        switch_state = true;
        digitalWrite(LED_BUILTIN, HIGH);

        BUILTIN_SWITCH.print("on");
    }
    else if (state == BLINKER_CMD_OFF) {
        switch_state = false;
        digitalWrite(LED_BUILTIN, LOW);

        BUILTIN_SWITCH.print("off");
    }
}

void heartbeat()
{
    if (switch_state) BUILTIN_SWITCH.print("on");
    else BUILTIN_SWITCH.print("off");
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin();
    
    Blinker.attachHeartbeat(heartbeat);
    
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
