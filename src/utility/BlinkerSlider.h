#ifndef BlinkerSlider_H
#define BlinkerSlider_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerSlider
{
    public :
        BlinkerSlider(const String & _name, callback_with_int32_arg_t _func = NULL)
            // : sliderName(_name)
        {
            wNum = Blinker.attachWidget(_name, _func);

            // wNum ? (registered = true) : (registered = false);

            // sliderName = (char*)malloc((_name.length()+1)*sizeof(char));
            // strcpy(sliderName, _name.c_str());
        }
        
        void attach(callback_with_int32_arg_t _func)
        {
            if (wNum == 0) {
                return;
            }

            Blinker.freshAttachWidget(Blinker.widgetName_int(wNum), _func);
        }
        
        void color(const String & _clr) {
            if (_fresh >> 0 & 0x01) {
                free(textClr);
            }

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 0;
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
        // String sliderName;
        // char * sliderName;
        uint8_t wNum;
        // bool registered = false;
        // String textClr = "";
        char * textClr;// = "";
        uint8_t _fresh = 0;

        void _print(const String & n) {
            if (wNum == 0 || (_fresh == 0 && n.length() == 0)) {
                return;
            }

            String sliderData;

            if (n.length()) {
                sliderData += BLINKER_F("{\""BLINKER_CMD_VALUE"\":");
                sliderData += n;
            }

            // if (textClr.length()) {
            // if (textClr && (_fresh >> 0 & 0x01)) {
            if (_fresh >> 0 & 0x01) {
                if (sliderData.length()) sliderData += BLINKER_F(",");
                else sliderData += BLINKER_F("{");

                sliderData += BLINKER_F("\""BLINKER_CMD_COLOR"\":\"");
                sliderData += (textClr);
                sliderData += BLINKER_F("\"");

                free(textClr);
            }

            sliderData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(Blinker.widgetName_int(wNum), sliderData);

            // textClr = "";
        }
};

#endif
