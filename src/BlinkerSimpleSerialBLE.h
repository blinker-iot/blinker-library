#ifndef BlinkerSimpleSerialBLE_H
#define BlinkerSimpleSerialBLE_H

#include <Adapters/BlinkerSerial.h>

static BlinkerTransportStream _blinkerTransport;
BlinkerSerail Blinker(_blinkerTransport);

#endif