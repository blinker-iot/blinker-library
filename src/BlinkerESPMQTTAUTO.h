#ifndef BLINKER_ESP_MQTT_AUTO_H
#define BLINKER_ESP_MQTT_AUTO_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTTAUTO.h"
#include "Blinker/BlinkerApi.h"
#include "modules/ArduinoJson/ArduinoJson.h"

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    #include <WebServer.h>
#endif

typedef BlinkerApi BApi;

class BlinkerESPMQTTAUTO : public BlinkerApi
{
    public : 
    // #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_APCONFIG)
        void begin(const char* _auth, const char* _type)
        {
            #if defined(BLINKER_ALIGENIE_LIGHT)
                String _aliType = BLINKER_F("&aliType=light");
            #elif defined(BLINKER_ALIGENIE_OUTLET)
                String _aliType = BLINKER_F("&aliType=outlet");
            #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
                String _aliType = BLINKER_F("&aliType=multi_outlet");
            #elif defined(BLINKER_ALIGENIE_SENSOR)
                String _aliType = BLINKER_F("&aliType=sensor");
            #else
                String _aliType = BLINKER_F("");
            #endif

            #if defined(BLINKER_DUEROS_LIGHT)
                String _duerType = BLINKER_F("&duerType=LIGHT");
            #elif defined(BLINKER_DUEROS_OUTLET)
                String _duerType = BLINKER_F("&duerType=SOCKET");
            #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
                String _duerType = BLINKER_F("&duerType=MULTI_SOCKET");
            #elif defined(BLINKER_DUEROS_SENSOR)
                String _duerType = BLINKER_F("&duerType=AIR_MONITOR");
            #else
                String _duerType = BLINKER_F("");
            #endif

            BApi::begin();

            Transp.aliType(_aliType);
            Transp.duerType(_duerType);
            Transp.begin(_auth, _type);

            transport(Transp);

            #if defined(BLINKER_ESP_SMARTCONFIG)
                Transp.smartconfigBegin();
            #elif defined(BLINKER_APCONFIG)
                Transp.apconfigBegin();
            #endif

            BApi::loadTimer();
        }
    // #else
    //     void begin( const char* _auth, 
    //                 const char* _ssid, 
    //                 const char* _pswd )
    //     {
    //         #if defined(BLINKER_ALIGENIE_LIGHT)
    //             String _aliType = BLINKER_F("&aliType=light");
    //         #elif defined(BLINKER_ALIGENIE_OUTLET)
    //             String _aliType = BLINKER_F("&aliType=outlet");
    //         #elif defined(BLINKER_ALIGENIE_MULTI_OUTLET)
    //             String _aliType = BLINKER_F("&aliType=multi_outlet");
    //         #elif defined(BLINKER_ALIGENIE_SENSOR)
    //             String _aliType = BLINKER_F("&aliType=sensor");
    //         #else
    //             String _aliType = BLINKER_F("");
    //         #endif

    //         #if defined(BLINKER_DUEROS_LIGHT)
    //             String _duerType = BLINKER_F("&duerType=LIGHT");
    //         #elif defined(BLINKER_DUEROS_OUTLET)
    //             String _duerType = BLINKER_F("&duerType=SOCKET");
    //         #elif defined(BLINKER_DUEROS_MULTI_OUTLET)
    //             String _duerType = BLINKER_F("&duerType=MULTI_SOCKET");
    //         #elif defined(BLINKER_DUEROS_SENSOR)
    //             String _duerType = BLINKER_F("&duerType=AIR_MONITOR");
    //         #else
    //             String _duerType = BLINKER_F("");
    //         #endif

    //         BApi::begin();

    //         Transp.aliType(_aliType);
    //         Transp.duerType(_duerType);
    //         Transp.begin(_auth);

    //         transport(Transp);

    //         Transp.commonBegin(_ssid, _pswd);
    //         BApi::loadTimer();
    //     }
    // #endif

    private :
        BlinkerMQTTAUTO Transp;
};

#endif

#endif
