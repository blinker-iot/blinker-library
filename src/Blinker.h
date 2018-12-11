#ifndef BLINKER_H
#define BLINKER_H

#if defined(BLINKER_BLE)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    
    #endif

    #if defined(ESP32)
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESP32BLE.h"

        static BlinkerBLE       _blinkerTransport;
        BlinkerESP32BLE         Blinker(_blinkerTransport);
    #else
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerSerialBLE.h"

        static BlinkerSerial    _blinkerTransport;
        BlinkerSerialBLE        Blinker(_blinkerTransport);
    #endif

#elif defined(BLINKER_WIFI)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESPWiFi.h"

        static BlinkerWiFi  _blinkerTransport;
        BlinkerESPWiFi      Blinker(_blinkerTransport);
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

    #if defined(BLINKER_DUEROS_LIGHT)
        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_OUTLET)
        #define BLINKER_DUEROS
    #elif defined(BLINKER_DUEROS_SENSOR)
        #define BLINKER_DUEROS
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESPMQTT.h"

        static BlinkerMQTT  _blinkerTransport;
        BlinkerESPMQTT      Blinker(_blinkerTransport);     
    #else
        #define BLINKER_ESP_AT

        #define BLINKER_MQTT_AT

        #undef BLINKER_MQTT

        #include "BlinkerSerialESPMQTT.h"

        static BlinkerSerialMQTT    _blinkerTransport;
        BlinkerSerialESPMQTT        Blinker(_blinkerTransport);  
    #endif

#elif defined(BLINKER_PRO)

    #if defined(BLINKER_ALIGENIE_LIGHT) || defined(BLINKER_ALIGENIE_OUTLET) || \
        defined(BLINKER_ALIGENIE_SWITCH)|| defined(BLINKER_ALIGENIE_SENSOR)
        #error This code is intended to run on the BLINKER_MQTT mode! Please check your mode setting.
    #endif

    #define BLINKER_ALIGENIE

    #ifndef BLINKER_ESP_SMARTCONFIG
        #ifndef BLINKER_APCONFIG
            #define BLINKER_ESP_SMARTCONFIG
        #endif
    #endif

    #if defined(ESP8266) || defined(ESP32)
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESPPRO.h"

        static BlinkerPRO   _blinkerTransport;
        BlinkerESPPRO       Blinker(_blinkerTransport); 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#elif defined(BLINKER_AT_MQTT)

    #define BLINKER_ESP_AT

    #if defined(ESP8266) || defined(ESP32)
        #include "Blinker/BlinkerDebug.h"
        #include "BlinkerESPMQTTAT.h"

        static BlinkerMQTTAT    _blinkerTransport;
        BlinkerESPMQTTAT        Blinker(_blinkerTransport); 
    #else
        #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting.
    #endif

#endif

#include "BlinkerWidgets.h"

#if defined(BLINKER_MQTT)

    #if defined(ESP8266)
        extern "C" {
        #   include "user_interface.h"
        }

        #define blinker_procTaskPrio        2
        #define blinker_procTaskQueueLen    1
        os_event_t    blinker_procTaskQueue[blinker_procTaskQueueLen];

        // uint32_t oldtime = 0;

        static void  blinker_run(os_event_t *events)
        {
            //user code
            // if (millis() - oldtime > 1000) {
            //     oldtime = millis();
            //     Serial.println("user loop");
            // }
            Blinker.run();
            // run (schedule) this loop task again
            system_os_post(blinker_procTaskPrio, 0, 0 );
        }

        void blinkerTaskInit()
        {
            // ets_task
            system_os_task(blinker_run, blinker_procTaskPrio, blinker_procTaskQueue, blinker_procTaskQueueLen);
            system_os_post(blinker_procTaskPrio, 0, 0 );
        }
    #endif

#endif

#endif
