#ifndef BLINKER_ESP_GATEWAY_H
#define BLINKER_ESP_GATEWAY_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerGateway.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPGateway : public BlinkerApi
{
    public : 
        void begin(const char* _key, const char* _type)
        {
            transport(Transp);
            BApi::begin(_key, _type);
            BApi::loadTimer();
            BLINKER_LOG(BLINKER_F("ESP_Gateway initialized..."));
        }

    private :
        BlinkerGateway Transp;
};

#endif

#endif
