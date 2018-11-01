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
            wNum = Blinker.attachWidget(_name, _func);

            // wNum ? (registered = true) : (registered = false);

            // jName = (char*)malloc((_name.length()+1)*sizeof(char));
            // strcpy(jName, _name.c_str());
        }
        
        void attach(callback_with_joy_arg_t _func)
        {
            if (wNum == 0) {
                return;
            }

            Blinker.freshAttachWidget(Blinker.widgetName_joy(wNum), _func);
        }
    
    private :
        // String jName;
        // char * jName;
        uint8_t wNum;
        // bool registered = false;
};

#endif
