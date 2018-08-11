#ifndef BlinkerSimplerESP32_PRO_H
#define BlinkerSimplerESP32_PRO_H

#include "Adapters/BlinkerPRO.h"

class BlinkerSimpleESP32_PRO 
    : public BlinkerProtocol<BlinkerPRO>
{
    typedef BlinkerProtocol<BlinkerPRO> Base;

    public : 
        BlinkerSimpleESP32_PRO(BlinkerPRO &transp)
            : Base(transp)
        {}

        void begin(const char* _type = BLINKER_AIR_DETECTOR) {
            Base::begin(_type);
            Base::loadTimer();
            BLINKER_LOG1("ESP8266_PRO initialized...");
        }
};

static BlinkerPRO  _blinkerTransport;
BlinkerSimpleESP32_PRO Blinker(_blinkerTransport);

#include <BlinkerWidgets.h>

#endif