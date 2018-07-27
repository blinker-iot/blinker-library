#ifndef BlinkerJoystick_H
#define BlinkerJoystick_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerJoystick
{
    public :
        BlinkerJoystick()
            : jName(BLINKER_CMD_JOYSTICK)
        {}
        
        void attach(callback_with_joy_arg_t _func) {
            Blinker.attachJoystick(_func);
        }
    
    private :
        String jName;
};

#endif
