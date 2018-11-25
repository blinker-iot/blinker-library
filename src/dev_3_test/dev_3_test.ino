#include "Blinker4.h"

#define BLINKER_PRINT Serial

void setup()
{
    Serial.begin(115200);

    #if defined(BLINKER_PRINT)
    Debug.stream(BLINKER_PRINT);
    #endif
    Debug.freeheap();

    Blinker.begin();

    BLINKER_LOG("hello", F("world"), 123, 456.78, -1);
}

void loop()
{
    BLINKER_LOG("hello", F("world"), 123, 456.78, -1);

    delay(5000);
}
