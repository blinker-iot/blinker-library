#ifndef BLINKER_ESP_MQTT_AT_H
#define BLINKER_ESP_MQTT_AT_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTTAT.h"
#include "Blinker/BlinkerProtocol.h"

class BlinkerESPMQTTAT : public BlinkerProtocol<BlinkerMQTTAT>
{
    typedef BlinkerProtocol<BlinkerMQTTAT> Base;

    public :
        BlinkerESPMQTTAT(BlinkerMQTTAT& transp)
            : Base(transp)
        {}
        
        void begin();
};

#endif

#endif
