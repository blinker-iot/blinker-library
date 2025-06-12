#ifndef BLINKER_JOYSTICK_H
#define BLINKER_JOYSTICK_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerJoystick : public BlinkerWidget
{
    public :
        BlinkerJoystick(const char* _name, 
            blinker_callback_with_joy_arg_t _func = NULL)
            : BlinkerWidget(_name)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
        }

        BlinkerJoystick& attach(blinker_callback_with_joy_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(name), _func);
            return *this;
        }

        void print() override
        {
            // Joystick通常不需要主动print，只接收数据
        }
};

#endif
