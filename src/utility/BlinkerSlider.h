#ifndef BlinkerSlider_H
#define BlinkerSlider_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerSlider
{
    public :
        BlinkerSlider(const String & _name, callback_with_int32_arg_t _func = NULL)
            : sliderName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void attach(callback_with_int32_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(sliderName, _func);
        }
        
        void color(const String & _clr) { textClr = _clr; }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
    
    private :
        String sliderName;
        bool registered = false;
        String textClr = "";

        void _print(const String & n) {
            if (!registered) {
                return;
            }

            String sliderData = "{\""BLINKER_CMD_VALUE"\":" + n;

            if (textClr.length()) {
                sliderData += ",\""BLINKER_CMD_COLOR"\":\"" + textClr + "\"";
            }

            sliderData += "}";

            Blinker.printArray(sliderName, sliderData);

            textClr = "";
        }
};

#endif
