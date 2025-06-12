#ifndef BLINKER_SWITCH_H
#define BLINKER_SWITCH_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerSwitch : public BlinkerWidget
{
    public :
        BlinkerSwitch(blinker_callback_with_string_arg_t _func = NULL)
            : BlinkerWidget(BLINKER_CMD_BUILTIN_SWITCH)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), _func);
        }        BlinkerSwitch& attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), _func);
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

        void print() override
        {
            print(currentState);
        }

        BlinkerSwitch& state(const String & _state)
        {
            currentState = _state;
            return *this;
        }

    private :
        String      currentState = "";
};

#endif
