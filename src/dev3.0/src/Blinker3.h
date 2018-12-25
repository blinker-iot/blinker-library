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
        #include "BlinkerESP32BLE.h"

        BlinkerESP32BLE     Blinker;
    #else
        #include "BlinkerSerialBLE.h"

        BlinkerSerialBLE    Blinker;
    #endif

#elif defined(BLINKER_WIFI) || defined(BLINKER_MQTT)

    #if defined(BLINKER_WIFI)
        #undef BLINKER_WIFI
        #define BLINKER_MQTT
    #endif

    #if defined(BLINKER_ALIGENIE_LIGHT)
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_OUTLET)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SWITCH)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SENSOR)
            #undef BLINKER_ALIGENIE_SENSOR
        #endif

        #define BLINKER_ALIGENIE
    #elif defined(BLINKER_ALIGENIE_SENSOR)
        #if defined(BLINKER_ALIGENIE_LIGHT)
            #undef BLINKER_ALIGENIE_LIGHT
        #endif
        #if defined(BLINKER_ALIGENIE_OUTLET)
            #undef BLINKER_ALIGENIE_OUTLET
        #endif
        #if defined(BLINKER_ALIGENIE_SWITCH)
            #undef BLINKER_ALIGENIE_SWITCH
        #endif
        
        #define BLINKER_ALIGENIE
    #endif

    #if defined(BLINKER_DUEROS_LIGHT)
        #if defined(BLINKER_DUEROS_OUTLET)
            #undef BLINKER_DUEROS_OUTLET
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_SENSOR)
            #undef BLINKER_DUEROS_SENSOR
        #endif

        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_OUTLET)
        #if defined(BLINKER_DUEROS_LIGHT)
            #undef BLINKER_DUEROS_LIGHT
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_SENSOR)
            #undef BLINKER_DUEROS_SENSOR
        #endif

        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_SENSOR)
        #if defined(BLINKER_DUEROS_LIGHT)
            #undef BLINKER_DUEROS_LIGHT
        #endif
        #if defined(BLINKER_DUEROS_SWITCH)
            #undef BLINKER_DUEROS_SWITCH
        #endif
        #if defined(BLINKER_DUEROS_OUTLET)
            #undef BLINKER_DUEROS_OUTLET
        #endif

        #define BLINKER_DUEROS
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPMQTT.h"

        BlinkerESPMQTT      Blinker;     
    #else
        #define BLINKER_ESP_AT

        #define BLINKER_MQTT_AT

        #undef BLINKER_MQTT
        
        #include "BlinkerSerialESPMQTT.h"

        BlinkerSerialESPMQTT      Blinker;  
    #endif

#elif defined(BLINKER_PRO)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif    

    #if defined(BLINKER_DUEROS_LIGHT) || defined(BLINKER_DUEROS_OUTLET) || \
        defined(BLINKER_DUEROS_SWITCH)|| defined(BLINKER_DUEROS_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #define BLINKER_ALIGENIE

    #define BLINKER_DUEROS

    #ifndef BLINKER_ESP_SMARTCONFIG
        #ifndef BLINKER_APCONFIG
            #define BLINKER_ESP_SMARTCONFIG
        #endif
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPPRO.h"
        
        BlinkerESPPRO       Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_AT_MQTT)

    #define BLINKER_ESP_AT

    #if defined(ESP8266) || defined(ESP32)
        #include "BlinkerESPMQTTAT.h"

        BlinkerESPMQTTAT    Blinker; 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#endif

#include "BlinkerWidgets.h"

#endif
