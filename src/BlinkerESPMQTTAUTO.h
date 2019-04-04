#ifndef BLINKER_ESP_MQTT_AUTO_H
#define BLINKER_ESP_MQTT_AUTO_H

#if defined(ESP8266) || defined(ESP32)

#ifndef BLINKER_ARDUINOJSON
    #define BLINKER_ARDUINOJSON
#endif

#include "Adapters/BlinkerMQTTAUTO.h"
#include "Blinker/BlinkerApi.h"

typedef BlinkerApi BApi;

class BlinkerESPMQTTAUTO : public BlinkerApi
{
    public : 
        void begin(const char* _key, const char* _type)
        {
            transport(Transp);
            BApi::begin(_key, _type);
            BApi::loadTimer();
            BLINKER_LOG(BLINKER_F("ESP8266_MQTT_AUTO initialized..."));
        }

    private :
        BlinkerMQTTAUTO Transp;
};

#endif

#endif
