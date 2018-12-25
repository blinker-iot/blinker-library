#ifndef BLINKER_ESP_PRO_H
#define BLINKER_ESP_PRO_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerPRO.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPPRO : public BlinkerApi
{
    public : 
        void begin(const char* _type = BLINKER_AIR_DETECTOR)
        {
            transport(Transp);
            BApi::begin(_type);
            BApi::loadTimer();
            BLINKER_LOG(BLINKER_F("ESP8266_PRO initialized..."));
        }

    private :
        BlinkerPRO Transp;
};

#endif

#endif
