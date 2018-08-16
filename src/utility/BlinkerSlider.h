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
            registered = Blinker.attachWidget(_name, _func);

            sliderName = (char*)malloc((_name.length()+1)*sizeof(char));
            strcpy(sliderName, _name.c_str());

            textClr = (char*)malloc(1*sizeof(char));
            textClr[0] = '\0';
        }
        
        void attach(callback_with_int32_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(sliderName, _func);
        }
        
        void color(const String & _clr) { 
            // textClr = _clr; 
            // if (strlen(textClr)) free(textClr);
            
            // textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            textClr = (char*)realloc(textClr, (_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());
        }
        
        void print(char value)              { _print(STRING_format(value)); }
        void print(unsigned char value)     { _print(STRING_format(value)); }
        void print(int value)               { _print(STRING_format(value)); }
        void print(unsigned int value)      { _print(STRING_format(value)); }       
        void print(long value)              { _print(STRING_format(value)); }        
        void print(unsigned long value)     { _print(STRING_format(value)); }
        void print(double value)            { _print(STRING_format(value)); }
    
    private :
        // String sliderName;
        char * sliderName;
        bool registered = false;
        // String textClr = "";
        char * textClr;// = "";

        void _print(const String & n) {
            if (!registered) {
                return;
            }

            String sliderData;
            sliderData += BLINKER_F("{\""BLINKER_CMD_VALUE"\":");
            sliderData += n;

            // if (textClr.length()) {
            if (strlen(textClr)) {
                sliderData += BLINKER_F(",\""BLINKER_CMD_COLOR"\":\"");
                sliderData += (textClr);
                sliderData += BLINKER_F("\"");
                // free(textClr);

                textClr = (char*)realloc(textClr, 1*sizeof(char));
                textClr[0] = '\0';
            }

            sliderData += BLINKER_F("}");

            Blinker.printArray(sliderName, sliderData);

            // textClr = "";
        }
};

#endif
