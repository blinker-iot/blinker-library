#ifndef Blinker_H
#define Blinker_H

#if defined(BLINKER_BLE)

    #if defined(ESP32)
        #include <BlinkerSimpleESP32_BLE.h>
    #else
        #include <BlinkerSimpleSerialBLE.h>
    #endif

#elif defined(BLINKER_WIFI)

    #if defined(ESP8266)
        #include <BlinkerSimpleESP8266_WS.h>
    #elif defined(ESP32)
        #include <BlinkerSimpleESP32_WS.h>
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_MQTT)

    #if defined(ESP8266)
        #include <BlinkerSimpleESP8266_MQTT.h>
    #elif defined(ESP32)
        #include <BlinkerSimpleESP32_MQTT.h>
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#endif

#endif
