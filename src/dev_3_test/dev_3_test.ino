#include "BlinkerDebug.h"

#define BLINKER_PRINT Serial

void setup()
{
    Serial.begin(115200);
    
    #if defined(BLINKER_PRINT)
    Debug.stream(BLINKER_PRINT);
    #endif

    Debug.freeheap();
}

void loop()
{}
