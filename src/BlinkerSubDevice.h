#ifndef BLINKER_SUBDEVICE_H
#define BLINKER_SUBDEVICE_H

#include "Adapters/BlinkerSubStream.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerSubDevice : public BlinkerApi
{
    public :
        void begin()
        {
            transport(Transp);
            BApi::begin();
            BLINKER_LOG(BLINKER_F("Blinker SubDevice initialized..."));
        }

    private :
        BlinkerSubStream Transp;
};

#endif
