#ifndef BLINKER_SUBDEVICE_H
#define BLINKER_SUBDEVICE_H

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerSubStream.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerSubDevice : public BlinkerApi
{
    public :
        void begin(const char* _auth)
        {
            BApi::begin();
            Transp.begin(_auth);
            transport(Transp);
            BLINKER_LOG(BLINKER_F("Blinker SubDevice initialized..."));
        }

    private :
        BlinkerSubStream Transp;
};

#endif
