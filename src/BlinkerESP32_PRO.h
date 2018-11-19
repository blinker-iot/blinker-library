#ifndef BlinkerESP32_PRO_H
#define BlinkerESP32_PRO_H

#include "Adapters/BlinkerPRO.h"

class BlinkerESP32_PRO 
    : public BlinkerProtocol<BlinkerPRO>
{
    typedef BlinkerProtocol<BlinkerPRO> Base;

    public : 
        BlinkerESP32_PRO(BlinkerPRO &transp)
            : Base(transp)
        {}

        void begin(const char* _type = BLINKER_AIR_DETECTOR) {
            Base::begin(_type);
            Base::loadTimer();
            BLINKER_LOG("ESP8266_PRO initialized...");
        }
};

static BlinkerPRO  _blinkerTransport;
BlinkerESP32_PRO Blinker(_blinkerTransport);

#include <BlinkerWidgets.h>

#endif