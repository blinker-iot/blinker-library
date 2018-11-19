#ifndef BlinkerSwitch_H
#define BlinkerSwitch_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerSwitch
{
    public :
        BlinkerSwitch()
            : sName(BLINKER_CMD_BUILTIN_SWITCH)
        {}
        
        void attach(callback_with_string_arg_t _func) {
            Blinker.attachSwitch(_func);
        }
        void print(const String & _state) {
            Blinker.print(sName, _state);
        }
    
    private :
        String sName;
};

#endif
