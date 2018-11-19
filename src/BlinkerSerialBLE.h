#ifndef BlinkerSerialBLE_H
#define BlinkerSerialBLE_H

#include <Adapters/BlinkerSerial.h>

static BlinkerTransportStream _blinkerTransport;
BlinkerSerial Blinker(_blinkerTransport);

#include <BlinkerWidgets.h>

#endif