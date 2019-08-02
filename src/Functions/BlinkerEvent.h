#ifndef BLINKER_EVENT_H
#define BLINKER_EVENT_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BLINKEREVENT
{
    public :
        BLINKEREVENT()
        {}

        void warning(const String & msg)
        {
            Blinker.eventWarn(msg);
        }

        void error(const String & msg)
        {
            Blinker.eventError(msg);
        }

        void message(const String & msg)
        {
            Blinker.eventMsg(msg);
        }

    private :
    
};

#endif
