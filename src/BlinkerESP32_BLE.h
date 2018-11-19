#ifndef BlinkerESP32_BLE_H
#define BlinkerESP32_BLE_H

#include "BlinkerBLE.h"

class BlinkerESP32_BLE:public BlinkerProtocol<BlinkerBLE>
{
    typedef BlinkerProtocol<BlinkerBLE> Base;

    public:
        BlinkerESP32_BLE(BlinkerBLE & transp)
            : Base(transp)
        {}

        void begin();
};

static BlinkerBLE _blinkerTransportBLE;
BlinkerESP32_BLE Blinker(_blinkerTransportBLE);

#include <BlinkerWidgets.h>

#endif
