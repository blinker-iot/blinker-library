#include "Blinker/BlinkerDebug.h"

#include <stddef.h>
#ifdef ESP8266
    extern "C" {
    #include <ets_sys.h>
    #include <os_type.h>
    #include <mem.h>
    }
    #include <Esp.h>
#else
    #include <inttypes.h>
#endif

#if defined(ARDUINO)
    #if (defined(__AVR__))
        #include <avr/pgmspace.h>
    #elif defined(ESP8266) || defined(ESP32)
        #include <pgmspace.h>
    #endif
#endif

uint32_t BLINKER_FreeHeap()
{
#if defined(ARDUINO) 
    #if defined(ESP8266) || defined(ESP32)
        return ESP.getFreeHeap();
// #elif defined(ARDUINO) && defined(ESP32)
//     return ESP.getFreeHeap();
    #elif defined(__AVR__)
        extern int __heap_start, *__brkval;
        int v;
        return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
    #else
        return 0;
    #endif
#else
    return 0;
#endif
}

BlinkerDebug BLINKER_DEBUG;

void BlinkerDebug::stream(Stream& s, blinker_debug_level_t level)
{
    isInit = true;
    debugger = &s;
    debug_level = level;
}

void BlinkerDebug::time()
{
    if (debug_level != _debug_none)
    {
        debugger->print(BLINKER_DEBUG_F("["));
        debugger->print(millis());
        debugger->print(BLINKER_DEBUG_F("] "));
    }
    return;
}

// uint32_t BlinkerDebug::BLINKER_FreeHeap()
// {
// #if defined(ARDUINO) 
//     #if defined(ESP8266) || defined(ESP32)
//         return ESP.getFreeHeap();
// // #elif defined(ARDUINO) && defined(ESP32)
// //     return ESP.getFreeHeap();
//     #elif defined(__AVR__)
//         extern int __heap_start, *__brkval;
//         int v;
//         return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
//     #else
//         return 0;
//     #endif
// #else
//     return 0;
// #endif
// }

void BlinkerDebug::freeheap()
{
    if (debug_level != _debug_none)
    {
        // time();
        // debugger->print(BLINKER_DEBUG_F("Freeheap: "));
        debugger->println(BLINKER_FreeHeap());
    }
    return;
}

void BLINKER_LOG_TIME()
{
    if (BLINKER_DEBUG.isDebug())
    {
        BLINKER_DEBUG.print(BLINKER_DEBUG_F("["));
        BLINKER_DEBUG.print(millis());
        BLINKER_DEBUG.print(BLINKER_DEBUG_F("] "));
    }
    return;
}

void BLINKER_LOG_FreeHeap()
{
    if (BLINKER_DEBUG.isDebug())
    {
        BLINKER_LOG_TIME();
        BLINKER_DEBUG.print(BLINKER_DEBUG_F("Freeheap: "));
        BLINKER_DEBUG.freeheap();
    }
    return;
}

void BLINKER_LOG_FreeHeap_ALL()
{
    if (BLINKER_DEBUG.isDebugAll())
    {
        BLINKER_LOG_TIME();
        BLINKER_DEBUG.print(BLINKER_DEBUG_F("Freeheap: "));
        BLINKER_DEBUG.freeheap();
    }
    return;
}

void BLINKER_LOG_T()
{
    if (BLINKER_DEBUG.isDebug()) BLINKER_DEBUG.println();
    return;
}
