#ifndef BLINKER_ESP_WIFI_H
#define BLINKER_ESP_WIFI_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerWiFi.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerESPWiFi: public BlinkerProtocol<BlinkerWiFi>
{
    typedef BlinkerProtocol<BlinkerWiFi> Base;

    public : 
        BlinkerESPWiFi(BlinkerWiFi &transp)
            : Base(transp)
        {}

        void begin()
        {
            #if defined(BLINKER_ESP_SMARTCONFIG)
                smartconfigBegin();
            #elif defined(BLINKER_APCONFIG)
                apconfigBegin();
            #endif
        }

        void begin( const char* _ssid, 
                    const char* _pswd )
        {
            commonBegin(_ssid, _pswd);
        }

    private :
        void commonBegin(const char* _ssid, 
                        const char* _pswd);
        void smartconfigBegin();
        void apconfigBegin();

        bool autoInit();
        void smartconfig();
        void softAPinit();
        // void serverClient();
        bool parseUrl(String data);
        
        void connectWiFi(String _ssid, String _pswd);
        void connectWiFi(const char* _ssid, const char* _pswd);
        void mDNSInit();
};

#endif

#endif
