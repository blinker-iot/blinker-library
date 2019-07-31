#ifndef BLINKER_JOYSTICK_H
#define BLINKER_JOYSTICK_H

#if defined(BLINKER_BLE)
    #include "../Blinker/BlinkerConfig.h"
    #include "../Blinker/BlinkerUtility.h"

    class BlinkerJoystick
    {
        public :
            BlinkerJoystick(char _name[], blinker_callback_with_joy_arg_t _func = NULL)
                // : jName(_name)
            {
                wNum = Blinker.attachWidget(_name, _func);
            }
            
            void attach(blinker_callback_with_joy_arg_t _func)
            {
                if (wNum == 0) {
                    return;
                }

                Blinker.freshAttachWidget(Blinker.widgetName_joy(wNum), _func);
            }
        
        private :
            uint8_t wNum;
    };
#endif

#endif
