#ifndef BlinkerJoystick_H
#define BlinkerJoystick_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerJoystick
{
    public :
        BlinkerJoystick(const String & _name, callback_with_joy_arg_t _func = NULL)
            // : jName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);

            jName = (char*)malloc((_name.length()+1)*sizeof(char));
            strcpy(jName, _name.c_str());
        }
        
        void attach(callback_with_joy_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(jName, _func);
        }
    
    private :
        // String jName;
        char * jName;
        bool registered = false;
};

#endif
