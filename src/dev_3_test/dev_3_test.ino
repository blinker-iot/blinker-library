#include "Blinker4.h"

#define BLINKER_PRINT Serial

void setup()
{
    Serial.begin(115200);

    #if defined(BLINKER_PRINT)
    BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif
    BLINKER_DEBUG.freeheap();

    Blinker.begin();

    BLINKER_LOG("hello", F("world"), 123, 456.78, -1);
}

void loop()
{
    BLINKER_LOG("hello", F("world"), 123, 456.78, -1);

    delay(5000);
}
