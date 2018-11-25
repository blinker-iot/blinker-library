#include "BlinkerDebug.h"

#include <stddef.h>
#ifdef ESP8266
    extern "C" {
    #include "ets_sys.h"
    #include "os_type.h"
    #include "mem.h"
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

#define BLINKER_DEBUG_F(s)  F(s)

BlinkerDebug Debug;

void BlinkerDebug::stream(Stream& s, blinker_debug_level_t level)
{
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

void BlinkerDebug::freeheap()
{
    if (debug_level != _debug_none)
    {
        time();
        debugger->print(BLINKER_DEBUG_F("Freeheap: "));
        debugger->println(BLINKER_FreeHeap());
    }
    return;
}

void BlinkerDebug::BLINKER_LOG_T()
{
    if (debug_level != _debug_none)
    {
        debugger->println();
    }
    return;
}

/* BLINKER_LOG_T递归模板 */
template <typename T,typename... Ts>
void BlinkerDebug::BLINKER_LOG_T(T arg,Ts... args)
{
    if (debug_level != _debug_none)
    {
        debugger->print(arg);
        BLINKER_LOG_T(args...);
    }
    return;
}
/* BLINKER_LOG可变参数模板 */
template <typename... Ts>
void BlinkerDebug::log(Ts... args)
{
    if (debug_level != _debug_none)
    {
        time();
        BLINKER_LOG_T(args...);
    }
    return;
}
/* BLINKER_ERR_LOG可变参数模板 */
template <typename... Ts>
void BlinkerDebug::error(Ts... args)
{
    if (debug_level != _debug_none)
    {
        time();
        debugger->print(BLINKER_DEBUG_F("ERROR: "));
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BlinkerDebug::logAll(Ts... args)
{
    if (debug_level == _debug_all)
    {
        time();
        BLINKER_LOG_T(args...);
    }
    return;
}

template <typename... Ts>
void BlinkerDebug::errorAll(Ts... args)
{
    if (debug_level == _debug_all)
    {
       error(args...);
    }
    return;
}

uint32_t BlinkerDebug::BLINKER_FreeHeap()
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