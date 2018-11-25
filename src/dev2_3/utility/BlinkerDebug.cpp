#include "utility/BlinkerDebug.h"

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

void BLINKER_LOG_TIME()
{
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.print(BLINKER_DEBUG_F("["));
    BLINKER_PRINT.print(millis());
    BLINKER_PRINT.print(BLINKER_DEBUG_F("] "));
#endif
    return;
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

void BLINKER_LOG_FreeHeap_ALL()
{
#if defined(BLINKER_DEBUG_ALL)
    BLINKER_LOG_TIME();
    BLINKER_PRINT.print(BLINKER_DEBUG_F("Freeheap: "));
    BLINKER_PRINT.println(BLINKER_FreeHeap());
#endif
    return;
}

void BLINKER_LOG_T()
{
#if defined(BLINKER_PRINT)
    BLINKER_PRINT.println();
#endif
   return;
}

// /* BLINKER_LOG_T递归模板 */
// template <typename T,typename... Ts>
// void BLINKER_LOG_T(T arg,Ts... args)
// {
// #if defined(BLINKER_PRINT)
//     BLINKER_PRINT.print(arg);
//     BLINKER_LOG_T(args...);
// #endif
//     return;
// }
// /* BLINKER_LOG可变参数模板 */
// template <typename... Ts>
// void BLINKER_LOG(Ts... args)
// {
//     BLINKER_LOG_TIME();
//     BLINKER_LOG_T(args...);
// // #if defined(BLINKER_PRINT)
// //     BLINKER_PRINT.println();
// // #endif
//     return;
// }
// /* BLINKER_ERR_LOG可变参数模板 */
// template <typename... Ts>
// void BLINKER_ERR_LOG(Ts... args)
// {
//     BLINKER_LOG_TIME();
// #if defined(BLINKER_PRINT)
//     BLINKER_PRINT.print(BLINKER_DEBUG_F("ERROR: "));
//     BLINKER_LOG_T(args...);
//     // BLINKER_PRINT.println();
// #endif
//     return;
// }

// template <typename... Ts>
// void BLINKER_LOG_ALL(Ts... args)
// {
// #if defined(BLINKER_DEBUG_ALL)
//     BLINKER_LOG_TIME();
//     BLINKER_LOG_T(args...);
// #endif
//     return;
// }

// template <typename... Ts>
// void BLINKER_ERR_LOG_ALL(Ts... args)
// {
// #if defined(BLINKER_DEBUG_ALL)
//     BLINKER_ERR_LOG(args...);
// #endif
//     return;
// }