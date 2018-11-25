#ifndef BlinkerESPAT_MQTT_H
#define BlinkerESPAT_MQTT_H

#include "Adapters/BlinkerMQTT_AT.h"

static BlinkerTransportStream _blinkerTransport;
BlinkerMQTT_AT Blinker(_blinkerTransport);

#endif