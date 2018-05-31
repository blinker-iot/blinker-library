#ifndef BlinkerSimpleSerialBLE_H
#define BlinkerSimpleSerialBLE_H

#include <Adapters/BlinkerSerial.h>

static BlinkerTransportStream _blinkerTransport;
BlinkerSerial Blinker(_blinkerTransport);

#endif