#ifndef BLINKER_BLE_H
#define BLINKER_BLE_H

#include "Functions/BlinkerBLEESP.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerBLE: public BlinkerProtocol<BlinkerBLEESP>
{
    typedef BlinkerProtocol<BlinkerBLEESP> Base;

    public :
        BlinkerBLE(BlinkerBLEESP &transp) : Base(transp) {}

        void begin()
        {
            Base::begin();
            this->conn.begin();
        }

    private :

};

#endif
