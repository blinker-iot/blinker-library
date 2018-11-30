#ifndef BLINKER_ESP_PRO_H
#define BLINKER_ESP_PRO_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerPRO.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerESPPRO : public BlinkerProtocol<BlinkerPRO>
{
    typedef BlinkerProtocol<BlinkerPRO> Base;

    public : 
        BlinkerESPPRO(BlinkerPRO &transp)
            : Base(transp)
        {}

        void begin(const char* _type = BLINKER_AIR_DETECTOR)
        {
            Base::begin(_type);
            Base::loadTimer();
            BLINKER_LOG(BLINKER_F("ESP8266_PRO initialized..."));
        }
};

#endif

#endif
