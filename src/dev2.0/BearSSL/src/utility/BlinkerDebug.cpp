#include "BlinkerDebugcpp.h"

void testPrint2()
{
#if defined(BLINKER_PRINT)
    Serial.println(".cpp defined");
#else
    Serial.println(".cpp not defined");
#endif    
    // testPrint3();
}