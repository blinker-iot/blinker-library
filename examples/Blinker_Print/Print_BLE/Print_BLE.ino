#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

#define SLIDER_1 "SliderKey"
#define TOGGLE_1 "ToggleKey"
#define TEXT_1   "millis"

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Blinker.begin();
    Blinker.wInit(SLIDER_1, W_SLIDER);
    Blinker.wInit(TOGGLE_1, W_TOGGLE);
}

uint8_t s_value = 0;
bool    on_off = false;

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print(TEXT_1, BlinkerTime);
    }

    Blinker.print(SLIDER_1, s_value);
    Blinker.print(TOGGLE_1, on_off?"on":"off");
    Blinker.print(TEXT_1, millis());

    digitalWrite(LED_BUILTIN, on_off);

    s_value++;
    on_off = !on_off;

    Blinker.delay(1000);
}