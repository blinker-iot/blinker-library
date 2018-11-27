#ifndef BLINKER_H
#define BLINKER_H

#include "Blinker/BlinkerDebug.h"
#include "BlinkerESPMQTT.h"

static BlinkerMQTT  _blinkerTransport;
BlinkerESPMQTT      Blinker(_blinkerTransport);

#include "BlinkerWidgets.h"

#endif
