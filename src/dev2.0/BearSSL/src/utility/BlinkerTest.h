#ifndef BlinkerTest_H
#define BlinkerTest_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#if defined(BLINKER_PRINT)
    extern uint8_t _flag = 1;
#endif

// void testPrint2();
// extern void testPrint1();
// extern void testPrint3();

// void testPrint1()
// {
// #if defined(BLINKER_PRINT)
//     Serial.println("1.h defined");
// #else
//     Serial.println("1.h not defined");
// #endif    
//     // testPrint2();
// }

// void testPrint3()
// {
// #if defined(BLINKER_PRINT)
//     Serial.println("3.h defined");
// #else
//     Serial.println("3.h not defined");
// #endif    
// }

#endif