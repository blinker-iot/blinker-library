#ifndef BLINKER_H
#define BLINKER_H

#if defined(BLINKER_MQTT)

    #if defined(BLINKER_ALIGENIE_LIGHT)
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef(BLINKER_ALIGENIE_OUTLET)
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef(BLINKER_ALIGENIE_SWITCH)
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef(BLINKER_ALIGENIE_SENSOR)
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef(BLINKER_ALIGENIE_LIGHT)
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef(BLINKER_ALIGENIE_SWITCH)
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef(BLINKER_ALIGENIE_SENSOR)
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SWITCH)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef(BLINKER_ALIGENIE_LIGHT)
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef(BLINKER_ALIGENIE_OUTLET)
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef(BLINKER_ALIGENIE_SENSOR)
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef(BLINKER_ALIGENIE_LIGHT)
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef(BLINKER_ALIGENIE_OUTLET)
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef(BLINKER_ALIGENIE_SWITCH)
        #endif
        
        #define BLINKER_ALIGENIE
    #endif

    #if defined(ESP8266) || defined(ESP32)

        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESPMQTT.h"

        static BlinkerMQTT  _blinkerTransport;
        BlinkerESPMQTT      Blinker(_blinkerTransport);
        
    #endif

#endif

#include "BlinkerWidgets.h"

#endif
