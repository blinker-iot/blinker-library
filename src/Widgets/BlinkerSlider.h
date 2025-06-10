#ifndef BLINKER_SLIDER_H
#define BLINKER_SLIDER_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BlinkerSlider
{
    public :
        BlinkerSlider(Functions& nFunc, const char* _name, 
            blinker_callback_with_int32_arg_t _func = NULL)
            :   func(nFunc),
                name(_name)
        {
            wNum = func.attachWidget(name, _func);
        }

        BlinkerSlider& attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) wNum = func.attachWidget(name, _func);
            else func.freshAttachWidget(name, _func);
            return *this;
        }
        
        BlinkerSlider& color(const String & _clr)
        {
            if (_fresh >> 0 & 0x01) free(textClr);

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 0;
            return *this;
        }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
        void print()                        { _print(""); }

    private :
        Functions&  func;
        const char* name;
        uint8_t     wNum;
        char *      textClr;
        uint8_t     _fresh = 0;

        void _print(const String & n)
        {
            if ((_fresh == 0 && n.length() == 0) || \
                wNum == 0)
            {
                return;
            }

            String sliderData;

            if (n.length())
            {
                sliderData += BLINKER_F("{\"");
                sliderData += BLINKER_F(BLINKER_CMD_VALUE);
                sliderData += BLINKER_F("\":");
                sliderData += n;
            }

            if (_fresh >> 0 & 0x01)
            {
                if (sliderData.length()) sliderData += BLINKER_F(",");
                else sliderData += BLINKER_F("{");

                sliderData += BLINKER_F("\"");
                sliderData += BLINKER_F(BLINKER_CMD_COLOR);
                sliderData += BLINKER_F("\":\"");
                sliderData += (textClr);
                sliderData += BLINKER_F("\"");

                free(textClr);
            }

            sliderData += BLINKER_F("}");

            _fresh = 0;

            func.printArray(name, sliderData);
        }
};

#endif
