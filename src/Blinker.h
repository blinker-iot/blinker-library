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
    #endif

#endif

#endif
