#ifndef BLINKER_JOYSTICK_H
#define BLINKER_JOYSTICK_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerJoystick
{
    public :
        BlinkerJoystick(const char* _name, 
            blinker_callback_with_joy_arg_t _func = NULL)
            : name(_name)
        {
            wNum = Blinker.attachWidget(name, _func);
        }

        BlinkerJoystick& attach(blinker_callback_with_joy_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(name, _func);
            else Blinker.freshAttachWidget(name, _func);
            return *this;
        }

    private :
        const char* name;
        uint8_t     wNum = 0;
};

#endif
