#ifndef BLINKER_RGB_H
#define BLINKER_RGB_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerRGB : public BlinkerWidget
{
    public :
        BlinkerRGB(const char* _name, 
            blinker_callback_with_rgb_arg_t _func = NULL)
            : BlinkerWidget(_name)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
        }
        BlinkerRGB& attach(blinker_callback_with_rgb_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(name), _func);
            return *this;
        }

        BlinkerRGB& brightness(uint8_t _bright) 
        { 
            rgbrightness = _bright; 
            return *this;
        }

        BlinkerRGB& red(uint8_t _r)
        {
            rgbRed = _r;
            return *this;
        }

        BlinkerRGB& green(uint8_t _g)
        {
            rgbGreen = _g;
            return *this;
        }

        BlinkerRGB& blue(uint8_t _b)
        {
            rgbBlue = _b;
            return *this;
        }

        BlinkerRGB& color(uint8_t _r, uint8_t _g, uint8_t _b)
        {
            rgbRed = _r;
            rgbGreen = _g;
            rgbBlue = _b;
            return *this;
        }        void print() override
        {
            print(rgbRed, rgbGreen, rgbBlue);
        }

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

            Blinker.printArray(const_cast<char*>(name), rgbData);
        }

        void print(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _bright)
        {
            if (wNum == 0) return;

            String rgbData = BLINKER_F("[");
            rgbData += STRING_format(_r);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_g);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_b);
            rgbData += BLINKER_F(",");
            rgbData += STRING_format(_bright);
            rgbData += BLINKER_F("]");

            Blinker.printArray(const_cast<char*>(name), rgbData);
        }

    private :
        uint8_t     rgbrightness = 0;
        uint8_t     rgbRed = 0;
        uint8_t     rgbGreen = 0;
        uint8_t     rgbBlue = 0;
};

#endif
