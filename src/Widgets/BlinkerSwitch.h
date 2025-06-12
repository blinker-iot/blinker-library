#ifndef BLINKER_SWITCH_H
#define BLINKER_SWITCH_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerSwitch
{
    public :
        BlinkerSwitch(blinker_callback_with_string_arg_t _func = NULL)
        {
            wNum = Blinker.attachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
        }

        BlinkerSwitch& attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
            else Blinker.freshAttachWidget(BLINKER_CMD_BUILTIN_SWITCH, _func);
            return *this;
        }

        void print(const String & _state)
        {
            if (_state.length() == 0 || wNum == 0)
            {
                return;
            }

            Blinker.print(BLINKER_CMD_BUILTIN_SWITCH, _state);
        }

        void print()
        {
            print(currentState);
        }

        BlinkerSwitch& state(const String & _state)
        {
            currentState = _state;
            return *this;
        }

    private :
        uint8_t     wNum = 0;
        String      currentState = "";
};

#endif
