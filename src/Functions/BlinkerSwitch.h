#ifndef BlinkerSwitch_H
#define BlinkerSwitch_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerSwitch
{
    public :
        BlinkerSwitch()
            // : sName(BLINKER_CMD_BUILTIN_SWITCH)
        {}
        
        void attach(blinker_callback_with_string_arg_t _func)
        {
            Blinker.attachSwitch(_func);
        }
        void print(const String & _state)
        {
            Blinker.print(BLINKER_CMD_BUILTIN_SWITCH, _state);
        }
    
    private :
        // String sName;
};

#endif
