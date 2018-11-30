#if defined(ESP8266) || defined(ESP32)

#define BLINKER_PRO

#include "BlinkerESPPRO.h"
#include "Adapters/BlinkerPRO.h"
#include "Blinker/BlinkerDebug.h"

void BlinkerESPPRO::begin(const char* _type)
{
    Base::begin(_type);
    Base::loadTimer();
    BLINKER_LOG(BLINKER_F("ESP8266_PRO initialized..."));
}

#endif
