#ifndef BLINKER_PRO_H
#define BLINKER_PRO_H

#if defined(ESP8266) || defined(ESP32)

#include "Functions/BlinkerPROESP.h"
#include "Blinker/BlinkerProtocol.h"
#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerPro: public BlinkerProtocol<BlinkerPROESP>
{
    typedef BlinkerProtocol<BlinkerPROESP> Base;

    public :
        BlinkerPro(BlinkerPROESP &transp) : Base(transp) {}

        void begin( const char* _key, 
                    const char* _type)
        {
            Base::begin();
            this->conn.begin(_key, _type);

        #if defined(BLINKER_WIDGET)
            Base::loadTimer();
        #endif
        }
};

#endif

#endif
