#ifndef BLINKER_ESP_HTTP_H
#define BLINKER_ESP_HTTP_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerHTTP.h"
#include "Blinker/BlinkerApi.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif

#if defined(ESP8266)
    #include <ESP8266mDNS.h>
    #include <ESP8266WiFi.h>
    // #include <ESP8266WebServer.h>
#elif defined(ESP32)
    #include <ESPmDNS.h>
    #include <WiFi.h>
    // #include <WebServer.h>
#endif

typedef BlinkerApi BApi;

class BlinkerESPHTTP : public BlinkerApi
{
    public : 
    #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_APCONFIG)
        void begin(const char* _auth)
        {
            BApi::begin();
            Transp.begin(_auth);

            transport(Transp);

            #if defined(BLINKER_ESP_SMARTCONFIG)
                Transp.smartconfigBegin();
            #elif defined(BLINKER_APCONFIG)
                Transp.apconfigBegin();
            #endif
        }
    #else
        void begin( const char* _auth, 
                    const char* _ssid, 
                    const char* _pswd )
        {
            BApi::begin();
            Transp.begin(_auth);

            transport(Transp);

            Transp.commonBegin(_ssid, _pswd);
        }
    #endif

    private :
        BlinkerHTTP Transp;
};

#endif

#endif
