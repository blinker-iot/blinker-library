#ifndef BLINKER_ESP_MQTT_H
#define BLINKER_ESP_MQTT_H

#include "BlinkerDebug.h"

class BlinkerESPMQTT
{
    public :
        BlinkerESPMQTT() {}

        void begin()
        {
            BLINKER_LOG("BlinkerESPMQTT begin");
        }
};

BlinkerESPMQTT Blinker;

#endif
