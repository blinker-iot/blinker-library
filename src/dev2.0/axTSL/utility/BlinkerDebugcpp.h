#ifndef BlinkerDebugTest_H
#define BlinkerDebugTest_H

#include "BlinkerTest.h"

// uint8_t _flag;

void testPrint2();
// {
// #if defined(BLINKER_PRINT)
//     Serial.println(".cpp defined");
// #else
//     Serial.println(".cpp not defined");
// #endif    
//     testPrint3();
// }

#endif

// void testPrint1()
// {
// #if defined(BLINKER_PRINT)
//     Serial.println("1.h defined");
// #else
//     Serial.println("1.h not defined");
// #endif    
//     testPrint2();
// }

// void testPrint3()
// {
// #if defined(BLINKER_PRINT)
//     Serial.println("3.h defined");
// #else
//     Serial.println("3.h not defined");
// #endif    
// }






// #include "BlinkerDebug.h"

// uint32_t BLINKER_FreeHeap()
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

// // void BLINKER_LOG_FreeHeap()
// // {
// // #if defined(BLINKER_PRINT)
// //     BLINKER_LOG_TIME();
// //     BLINKER_PRINT.print(BLINKER_DEBUG_F("Freeheap: "));
// //     BLINKER_PRINT.println(BLINKER_FreeHeap());
// // #endif
// //     return;
// // }

// // void BLINKER_LOG_TIME()
// // {
// // #if defined(BLINKER_PRINT)
// //     BLINKER_PRINT.print(BLINKER_DEBUG_F("["));
// //     BLINKER_PRINT.print(millis());
// //     BLINKER_PRINT.print(BLINKER_DEBUG_F("] "));
// // #endif
// //     return;
// // }

// void BLINKER_LOG_T()
// {
// #if defined(BLINKER_PRINT)
//     BLINKER_PRINT.println();
// #endif
//    return;
// }
