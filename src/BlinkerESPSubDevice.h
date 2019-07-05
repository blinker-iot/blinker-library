#ifndef BLINKER_ESP_SUBDEVICE_H
#define BLINKER_ESP_SUBDEVICE_H

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerSubDevice.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPSubDevice : public BlinkerApi
{
    public :
        void begin(const char* _key, const char* _type)
        {
            transport(Transp);
            BApi::begin(_key, _type);
            BLINKER_LOG(BLINKER_F("Blinker SubDevice initialized..."));
        }

    private :
        BlinkerSubDevice Transp;
};

#endif
