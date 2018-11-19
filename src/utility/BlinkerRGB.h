#ifndef BlinkerRGB_H
#define BlinkerRGB_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerRGB
{
    public :
        BlinkerRGB(const String & _name, callback_with_rgb_arg_t _func = NULL)
            // : rgbName(_name)
        {
            wNum = Blinker.attachWidget(_name, _func);

            // wNum ? (registered = true) : (registered = false);

            // rgbName = (char*)malloc((_name.length()+1)*sizeof(char));
            // strcpy(rgbName, _name.c_str());
        }

        void attach(callback_with_rgb_arg_t _func)
        {
            if (wNum == 0) {
                return;
            }

            Blinker.freshAttachWidget(Blinker.widgetName_rgb(wNum), _func);
        }

        void brightness(uint8_t _bright) { rgbrightness = _bright; }

        void print(uint8_t _r, uint8_t _g, uint8_t _b)
        {
            if (wNum == 0) {
                return;
            }

            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(rgbrightness) + "]";

            Blinker.printArray(Blinker.widgetName_rgb(wNum), rgbData);
        }

        void print(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _bright)
        {
            if (wNum == 0) {
                return;
            }

            String rgbData = "[" + STRING_format(_r) + "," + STRING_format(_g) + "," + \
                            STRING_format(_b) + "," + STRING_format(_bright) + "]";

            Blinker.printArray(Blinker.widgetName_rgb(wNum), rgbData);
        }

    private :
        // String  rgbName;
        // char *  rgbName;
        uint8_t wNum;
        // bool    registered = false;
        uint8_t rgbrightness = 0;
};

#endif
