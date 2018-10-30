#ifndef BlinkerSimplerESP8266_PRO_H
#define BlinkerSimplerESP8266_PRO_H

#include "Adapters/BlinkerPRO.h"

class BlinkerSimpleESP8266_PRO 
    : public BlinkerProtocol<BlinkerPRO>
{
    typedef BlinkerProtocol<BlinkerPRO> Base;

    public : 
        BlinkerSimpleESP8266_PRO(BlinkerPRO &transp)
            : Base(transp)
        {}

        void begin(const char* _type = BLINKER_AIR_DETECTOR) {
            Base::begin(_type);
            Base::loadTimer();
            BLINKER_LOG1(BLINKER_F("ESP8266_PRO initialized..."));
        }
};

static BlinkerPRO  _blinkerTransport;
BlinkerSimpleESP8266_PRO Blinker(_blinkerTransport);

#include <BlinkerWidgets.h>

#endif