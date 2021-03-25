#ifndef BLINKER_RGB_H
#define BLINKER_RGB_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BlinkerRGB
{
    public :
        BlinkerRGB(Functions& nFunc, const char* _name, 
            blinker_callback_with_rgb_arg_t _func = NULL)
            :   func(nFunc),
                name(_name)
        {
            wNum = func.attachWidget(name, _func);
        }

        void attach(blinker_callback_with_rgb_arg_t _func)
        {
            if (wNum == 0) wNum = func.attachWidget(name, _func);
            else func.freshAttachWidget(name, _func);
        }

        void brightness(uint8_t _bright) { rgbrightness = _bright; }

        void print(uint8_t _r, uint8_t _g, uint8_t _b)
        {
            if (wNum == 0) return;

            String rgbData = BLINKER_F("[");
            rgbData += STRING_format(_r);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_g);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_b);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(rgbrightness);
            rgbData += BLINKER_F("]");

            func.printArray(name, rgbData);
        }

        void print(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _bright)
        {
            if (wNum == 0) {
                return;
            }

            String rgbData = BLINKER_F("[");
            rgbData += STRING_format(_r);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_g);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_b);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_bright);
            rgbData += BLINKER_F("]");

            func.printArray(name, rgbData);
        }

    private :
        Functions&  func;
        const char* name;
        uint8_t     wNum;
        uint8_t     rgbrightness = 0;
};

#endif
