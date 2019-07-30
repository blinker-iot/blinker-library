#ifndef BlinkerRGB_H
#define BlinkerRGB_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerRGB
{
    public :
        BlinkerRGB(char _name[], blinker_callback_with_rgb_arg_t _func = NULL)
            // : rgbName(_name)
        {
            wNum = Blinker.attachWidget(_name, _func);
        }

        void attach(blinker_callback_with_rgb_arg_t _func)
        {
            if (wNum == 0) return;

            Blinker.freshAttachWidget(Blinker.widgetName_rgb(wNum), _func);
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

            Blinker.printArray(Blinker.widgetName_rgb(wNum), rgbData);
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

            Blinker.printArray(Blinker.widgetName_rgb(wNum), rgbData);
        }

    private :
        uint8_t wNum;
        uint8_t rgbrightness = 0;
};

#endif
