#ifndef BLINKER_AIR202_LP_H
#define BLINKER_AIR202_LP_H

#if ARDUINO >= 100
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerDebug.h"
#include "Blinker/BlinkerStream.h"
#include "Blinker/BlinkerUtility.h"
#include "Functions/BlinkerAIR202.h"
#include "Functions/BlinkerHTTPAIR202.h"

#include "modules/ArduinoJson/ArduinoJson.h"

class BlinkerAIR202LP : public BlinkerStream
{
    public :
        BlinkerAIR202LP()
            : stream(NULL), isConnect(false)
        {}

        void 

    private :

    protected :
        Stream*     stream;
        char*       msgBuf;
        bool        isConnect;
        bool        isHWS = false;
        char*       imei;
        bool        isAvail = false;
};

#endif
