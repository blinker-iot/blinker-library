#ifndef BlinkerSlider_H
#define BlinkerSlider_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerSlider
{
    public :
        BlinkerSlider(const String & _name, callback_with_int32_arg_t _func)
            : sliderName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void color(const String & _clr) { textClr = _clr; }
        void print(int n) {
            if (!registered) {
                return;
            }

            String sliderData = "{\"val\":\"" + STRING_format(n) + "\"";

            if (textClr.length()) {
                sliderData += ",\"col\":\"" + textClr + "\"";
            }

            sliderData += "}";

            Blinker.printArray(sliderName, sliderData);
        }
    
    private :
        String sliderName;
        bool registered = false;
        String textClr = "";
};

#endif
