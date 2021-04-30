#ifndef BLINKER_SWITCH_H
#define BLINKER_SWITCH_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BlinkerSwitch
{
    public :
        BlinkerSwitch(Functions& nFunc, 
            blinker_callback_with_string_arg_t _func = NULL)
            :   func(nFunc)
        {
            wNum = func.attachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
        }

        void attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) wNum = func.attachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
            else func.freshAttachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
        }

        void print(const String & _state)
        {
            if (_state.length() == 0 || \
                wNum == 0)
            {
                return;
            }

            func.print(BLINKER_CMD_BUILTIN_SWITCH, _state);
        }

    private :
        Functions&  func;
        uint8_t     wNum;
};

#endif
