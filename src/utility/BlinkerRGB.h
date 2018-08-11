#ifndef BlinkerRGB_H
#define BlinkerRGB_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerRGB
{
    public :
        BlinkerRGB(const String & _name, callback_with_rgb_arg_t _func = NULL)
            : rgbName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void attach(callback_with_rgb_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(rgbName, _func);
        }
        
        void brightness(uint8_t _bright) { rgbrightness = _bright; }

        void print(uint8_t _r, uint8_t _g, uint8_t _b)
        {
            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(rgbrightness) + "]";

            Blinker.printArray(rgbName, rgbData);
        }

        void print(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _bright)
        {
            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(_bright) + "]";

            Blinker.printArray(rgbName, rgbData);
        }
    
    private :
        String  rgbName;
        bool    registered = false;
        uint8_t rgbrightness = 0;
};

#endif
