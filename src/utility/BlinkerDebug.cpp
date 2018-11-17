#include "BlinkerDebug.h"

uint32_t BLINKER_FreeHeap()
{
#if defined(ARDUINO) && defined(ESP8266)
    return ESP.getFreeHeap();
#elif defined(ARDUINO) && defined(ESP32)
    return ESP.getFreeHeap();
#elif defined(ARDUINO) && defined(__AVR__)
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
#else
    return 0;
#endif
}

void BLINKER_LOG_FreeHeap()
{
#if defined(BLINKER_PRINT)
    BLINKER_LOG_TIME();
    BLINKER_PRINT.print(BLINKER_DEBUG_F("Freeheap: "));
    BLINKER_PRINT.println(BLINKER_FreeHeap());
#endif
    return;
}

void BLINKER_LOG_TIME()
{
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.print(BLINKER_DEBUG_F("["));
    BLINKER_PRINT.print(millis());
    BLINKER_PRINT.print(BLINKER_DEBUG_F("] "));
#endif
    return;
}

void BLINKER_LOG_T()
{
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.print('\n');
#endif
   return;
}
