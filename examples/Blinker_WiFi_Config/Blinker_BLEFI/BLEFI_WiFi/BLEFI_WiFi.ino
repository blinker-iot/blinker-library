/*
 * ESP32 bleFi provisioning example.
 */

#define BLINKER_WIFI
#define BLINKER_BLEFI

#include <Blinker.h>
#include "OneButton.h"

#define BLINKER_BUTTON_PIN 4

OneButton button(BLINKER_BUTTON_PIN, true);

char auth[] = "Your Device Secret Key";

void deviceReset()
{
    Blinker.reset();
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    Blinker.print("millis", millis());
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth);
    Blinker.attachData(dataRead);
    button.attachLongPressStop(deviceReset);
}

void loop()
{
    Blinker.run();
    button.tick();
}
