#ifndef Blinker_H
#define Blinker_H

#if defined(BLINKER_MQTT)
    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPMQTT.h"
    #endif
#endif

#endif
