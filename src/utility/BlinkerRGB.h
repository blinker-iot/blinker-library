#ifndef BlinkerRGB_H
#define BlinkerRGB_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerRGB
{
    public :
        BlinkerRGB(const String & _name, callback_with_rgb_arg_t _func)
            : rgbName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void brightness(uint8_t _bright) { rgbrightness = _bright; }

        void print(uint8_t _r, uint8_t _g, uint8_t _b) {
            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(rgbrightness) + "]";

            Blinker.printArray(rgbName, rgbData);
        }

        void print(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _bright) {
            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(_bright) + "]";

            Blinker.printArray(rgbName, rgbData);
        }
        // void name(String name) { rgbName = name; }
        // String getName() { return rgbName; }
        // void freshValue(b_rgb_t color,uint8_t value) { rgbValue[color] = value; }
        // uint8_t getValue(b_rgb_t color) { return rgbValue[color]; }
        // bool checkName(String name) { return ((rgbName == name) ? true : false); }
    
    private :
        String  rgbName;
        bool    registered = false;
        uint8_t rgbrightness = 0;
};

#endif
