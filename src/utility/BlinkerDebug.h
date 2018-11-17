#ifndef BlinkerDebug_H
#define BlinkerDebug_H

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
    #if ARDUINO >= 100
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #if (defined(__AVR__))
        #include <avr/pgmspace.h>

        #define BLINKER_DEBUG_F(s)  F(s)
    #elif defined(ESP8266) || defined(ESP32)
        #include <pgmspace.h>

        #define BLINKER_DEBUG_F(s)  s
    #endif

    #define BLINKER_F(s)        F(s)
    #define BLINKER_PSTR(s)     PSTR(s)
#else
    #define BLINKER_F(s)        s
    #define BLINKER_PSTR(s)     s
#endif

extern uint32_t BLINKER_FreeHeap();
// #if defined(ARDUINO) && defined(ESP8266)
//     static uint32_t BLINKER_FreeHeap()
//     {
//         return ESP.getFreeHeap();
//     }
// #elif defined(ARDUINO) && defined(ESP32)
//     static uint32_t BLINKER_FreeHeap()
//     {
//         return ESP.getFreeHeap();
//     }
// #elif defined(ARDUINO) && defined(__AVR__)
//     static uint32_t BLINKER_FreeHeap()
//     {
//         extern int __heap_start, *__brkval;
//         int v;
//         return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
//     }
// #else
//     static uint32_t BLINKER_FreeHeap()
//     {
//         return 0;
//     }
// #endif

#ifndef BLINKER_PRINT
#undef BLINKER_DEBUG
#undef BLINKER_DEBUG_ALL
#endif

extern void BLINKER_LOG_TIME();
extern void BLINKER_LOG_FreeHeap();
extern void BLINKER_LOG_T(); /* BLINKER_LOG_T模板递归结束 */
/* BLINKER_LOG_T递归模板 */
template <typename T,typename... Ts>
void BLINKER_LOG_T(T arg,Ts... args)
{
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.print(arg);
    BLINKER_LOG_T(args...);
#endif
    return;
}
/* BLINKER_LOG可变参数模板 */
template <typename... Ts>
void BLINKER_LOG(Ts... args)
{
    BLINKER_LOG_TIME();
    BLINKER_LOG_T(args...);
    return;
}
/* BLINKER_ERR_LOG可变参数模板 */
template <typename... Ts>
void BLINKER_ERR_LOG(Ts... args)
{
    BLINKER_LOG_TIME();
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: "));
#endif
    BLINKER_LOG_T(args...);
    return;
}
/* 待废止内容 */
#if defined(BLINKER_PRINT)
    // #define BLINKER_LOG_FreeHeap()                  { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("Freeheap: ")); BLINKER_PRINT.println(BLINKER_FreeHeap());}
    #define BLINKER_LOG1(p1)                        { BLINKER_LOG_TIME(); BLINKER_PRINT.println(p1); }
    #define BLINKER_LOG2(p1, p2)                    { BLINKER_LOG_TIME(); BLINKER_PRINT.print(p1); BLINKER_PRINT.println(p2); }
    #define BLINKER_LOG3(p1, p2, p3)                { BLINKER_LOG_TIME(); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.println(p3); }
    #define BLINKER_LOG4(p1, p2, p3, p4)            { BLINKER_LOG_TIME(); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.println(p4); }
    #define BLINKER_LOG5(p1, p2, p3, p4, p5)        { BLINKER_LOG_TIME(); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.print(p4); BLINKER_PRINT.println(p5); }
    #define BLINKER_LOG6(p1, p2, p3, p4, p5, p6)    { BLINKER_LOG_TIME(); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.print(p4); BLINKER_PRINT.print(p5); BLINKER_PRINT.println(p6); }
    #define BLINKER_ERR_LOG1(p1)                    { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.println(p1); }
    #define BLINKER_ERR_LOG2(p1, p2)                { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.print(p1); BLINKER_PRINT.println(p2); }
    #define BLINKER_ERR_LOG3(p1, p2, p3)            { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.println(p3); }
    #define BLINKER_ERR_LOG4(p1, p2, p3, p4)        { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.println(p4); }
    #define BLINKER_ERR_LOG5(p1, p2, p3, p4, p5)    { BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.print(p4); BLINKER_PRINT.println(p5); }
    #define BLINKER_ERR_LOG6(p1, p2, p3, p4, p5, p6){ BLINKER_LOG_TIME(); BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: ")); BLINKER_PRINT.print(p1); BLINKER_PRINT.print(p2); BLINKER_PRINT.print(p3); BLINKER_PRINT.print(p4); BLINKER_PRINT.print(p5); BLINKER_PRINT.println(p6); }

    // static void BLINKER_LOG_TIME() {
    //     BLINKER_PRINT.print(BLINKER_DEBUG_F("["));
    //     BLINKER_PRINT.print(millis());
    //     BLINKER_PRINT.print(BLINKER_DEBUG_F("] "));
    // }
#else
    // #define BLINKER_LOG_FreeHeap()
    #define BLINKER_LOG1(p1)
    #define BLINKER_LOG2(p1, p2)
    #define BLINKER_LOG3(p1, p2, p3)
    #define BLINKER_LOG4(p1, p2, p3, p4)
    #define BLINKER_LOG5(p1, p2, p3, p4, p5)
    #define BLINKER_LOG6(p1, p2, p3, p4, p5, p6)
    #define BLINKER_ERR_LOG1(p1)
    #define BLINKER_ERR_LOG2(p1, p2)
    #define BLINKER_ERR_LOG3(p1, p2, p3)
    #define BLINKER_ERR_LOG4(p1, p2, p3, p4)
    #define BLINKER_ERR_LOG5(p1, p2, p3, p4, p5)
    #define BLINKER_ERR_LOG6(p1, p2, p3, p4, p5, p6)
#endif

#endif
