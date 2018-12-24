#ifndef BLINKER_H
#define BLINKER_H

#if defined(BLINKER_BLE)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.    
    #endif

    #if defined(BLINKER_DUEROS_LIGHT) || defined(BLINKER_DUEROS_OUTLET) || \
        defined(BLINKER_DUEROS_SWITCH)|| defined(BLINKER_DUEROS_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #if defined(ESP32)
    #else
        #include "BlinkerSerialBLE.h"

        BlinkerSerialBLE Blinker;
    #endif

#endif

#include "BlinkerWidgets.h"

#endif
