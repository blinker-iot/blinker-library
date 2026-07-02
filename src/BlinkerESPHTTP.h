#ifndef BLINKER_ESP_HTTP_H
#define BLINKER_ESP_HTTP_H

#include "Blinker/BlinkerWiFiPlatform.h"

#if defined(BLINKER_NATIVE_WIFI)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#if defined(BLINKER_BLEFI)
    #error BLEFI provisioning currently supports BLINKER_WIFI/BLINKER_MQTT on ESP32. Use Blinker.begin(auth, ssid, pswd) with BLINKER_HTTP.
#endif

#include "Adapters/BlinkerHTTP.h"
#include "Blinker/BlinkerApi.h"
#ifndef ARDUINOJSON_VERSION_MAJOR
#include "modules/ArduinoJson/ArduinoJson.h"
#endif

#if !defined(ESP32) && (defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_ESP_SMARTCONFIG_V2))
    #error SMARTCONFIG currently require ESP32. Use Blinker.begin(auth, ssid, pswd) on Arduino UNO R4 WiFi and Raspberry Pi Pico W.
#endif

typedef BlinkerApi BApi;

class BlinkerESPHTTP : public BlinkerApi
{
    public : 
    #if defined(BLINKER_ESP_SMARTCONFIG) || defined(BLINKER_ESP_SMARTCONFIG_V2)
        void begin(const char* _auth)
        {
            BApi::begin();
            Transp.begin(_auth);

            transport(Transp);

            Transp.smartconfigBegin();
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
