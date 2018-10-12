#ifndef Blinker_H
#define Blinker_H

#if defined(BLINKER_BLE)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) \
    ||  defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    
    #endif

    #if defined(ESP32)
        #include <BlinkerSimpleESP32_BLE.h>
    #else
        #include <BlinkerSimpleSerialBLE.h>
    #endif

#elif defined(BLINKER_WIFI)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) \
    ||  defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #if defined(ESP8266)
        #include <BlinkerSimpleESP8266_WS.h>
    #elif defined(ESP32)
        #include <BlinkerSimpleESP32_WS.h>
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_MQTT)

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

    #if defined(ESP8266)
        #include <BlinkerSimpleESP8266_MQTT.h>
    #elif defined(ESP32)
        #include <BlinkerSimpleESP32_MQTT.h>
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_PRO)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) \
    ||  defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #ifndef BLINKER_ESP_SMARTCONFIG
        #ifndef BLINKER_APCONFIG
            #define BLINKER_ESP_SMARTCONFIG
        #endif
    #endif

    #if defined(ESP8266)
        #include <BlinkerSimpleESP8266_PRO.h>
    #elif defined(ESP32)
        #include <BlinkerSimpleESP32_PRO.h>
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#else
    #error Please setting connect mode ahead! Here provided BLINKER_BLE/BLINKER_WIFI/BLINKER_MQTT mode!
#endif

#endif
