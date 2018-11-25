#ifndef BlinkerSerialNBIOT_H
#define BlinkerSerialNBIOT_H

#if defined(BLINKER_NB73)
    #include "Adapters/BlinkerNB73.h"

    static BlinkerTransportStream _blinkerTransport;
    BlinkerNB73 Blinker(_blinkerTransport);
#else
    #error Please set up your NBIoT module type
#endif

#include "BlinkerWidgets.h"

#endif