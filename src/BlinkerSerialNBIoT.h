#ifndef BlinkerSerialNBIOT_H
#define BlinkerSerialNBIOT_H

#include <Adapters/BlinkerNBIOT.h>

static BlinkerTransportStream _blinkerTransport;
BlinkerNBIOT Blinker(_blinkerTransport);

#include <BlinkerWidgets.h>

#endif