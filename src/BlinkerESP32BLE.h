#ifndef BLINKER_ESP32_BLE_H
#define BLINKER_ESP32_BLE_H

#if defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerBLE.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerESP32BLE
    : public BlinkerProtocol<BlinkerBLE>
{
    typedef BlinkerProtocol<BlinkerBLE> Base;

    public:
        BlinkerESP32BLE(BlinkerBLE & transp)
            : Base(transp)
        {}

        void begin()
        {
            Base::begin();
            this->conn.begin();
            BLINKER_LOG("ESP32_BLE initialized...");
        }
};

#endif

#endif
