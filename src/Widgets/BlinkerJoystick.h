#ifndef BLINKER_JOYSTICK_H
#define BLINKER_JOYSTICK_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BlinkerJoystick
{
    public :
        BlinkerJoystick(Functions& nFunc, const char* _name, 
            blinker_callback_with_joy_arg_t _func = NULL)
            :   func(nFunc),
                name(_name)
            {
                wNum = func.attachWidget(name, _func);
            }

        void attach(blinker_callback_with_joy_arg_t _func)
        {
            if (wNum == 0) wNum = func.attachWidget(name, _func);
            else func.freshAttachWidget(name, _func);
        }

    private :
        Functions&  func;
        const char* name;
        uint8_t     wNum;
};

#endif
