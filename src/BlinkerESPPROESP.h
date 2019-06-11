#ifndef BLINKER_ESP_PRO_ESP_H
#define BLINKER_ESP_PRO_ESP_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerPROESP.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPPROESP : public BlinkerApi
{
    public : 
        void begin(const char* _key, const char* _type)
        {
            transport(Transp);
            BApi::begin(_key, _type);
            BApi::loadTimer();
            BLINKER_LOG(BLINKER_F("ESP_PRO initialized..."));
        }

    private :
        BlinkerPROESP Transp;
};

#endif

#endif
