#define BLINKER_PRINT	Serial
#define BLINKER_WIFI
#define BLINKER_ESP_SMARTCONFIG

#include <Blinker.h>

void setup()
{
    Serial.begin(115200);

    Blinker.begin();
}

void loop()
{
    Blinker.run();
}