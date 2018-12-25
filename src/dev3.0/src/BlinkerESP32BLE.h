#ifndef BLINKER_ESP32_BLE_H
#define BLINKER_ESP32_BLE_H

#if defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerBLE.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESP32BLE : public BlinkerApi
{
    public :
        void begin()
        {
            BApi::begin();
            Transp.begin();
            transport(Transp);
            BLINKER_LOG("ESP32_BLE initialized...");
        }

    private :
        BlinkerBLE Transp;
};

#endif

#endif
