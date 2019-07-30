#ifndef BlinkerText_H
#define BlinkerText_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerText
{
    public :
        BlinkerText(char _name[])
        {
            textName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(textName, _name);
        }
        
        template <typename T>
        void print(T _text)
        {
            // if (isnan(_text)) return;

            String textData = BLINKER_F("{\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text);
            textData += BLINKER_F("\"");
            if (_fresh >> 0 & 0x01)
            {
                
                textData += BLINKER_F(",\"");
                textData += BLINKER_F(BLINKER_CMD_ICON);
                textData += BLINKER_F("\":\"");
                textData += nicon;
                textData += BLINKER_F("\"");

                free(nicon);
            }

            if (_fresh >> 1 & 0x01)
            {
                textData += BLINKER_F(",\"");
                textData += BLINKER_F(BLINKER_CMD_COLOR);
                textData += BLINKER_F("\":\"");
                textData += ncolor;
                textData += BLINKER_F("\"");

                free(ncolor);
            }

            textData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(textName, textData);
        }

        template <typename T1, typename T2>
        void print(T1 _text1, T2 _text2)
        {
            // if (isnan(_text1) || isnan(_text2)) return;

            String textData = BLINKER_F("{\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text1);
            textData += BLINKER_F("\",\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT1);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text2);
            textData += BLINKER_F("\"");

            if (_fresh >> 0 & 0x01)
            {
                
                textData += BLINKER_F(",\"");
                textData += BLINKER_F(BLINKER_CMD_ICON);
                textData += BLINKER_F("\":\"");
                textData += nicon;
                textData += BLINKER_F("\"");

                free(nicon);
            }

            if (_fresh >> 1 & 0x01)
            {
                textData += BLINKER_F(",\"");
                textData += BLINKER_F(BLINKER_CMD_COLOR);
                textData += BLINKER_F("\":\"");
                textData += ncolor;
                textData += BLINKER_F("\"");

                free(ncolor);
            }

            textData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(textName, textData);
        }

        void icon(const String & _icon)
        {
            if (_fresh >> 0 & 0x01) free(nicon);

            nicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(nicon, _icon.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & _clr)
        {
            if (_fresh >> 1 & 0x01) free(ncolor);

            ncolor = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(ncolor, _clr.c_str());

            _fresh |= 0x01 << 1;
        }
    
    private :
        char * nicon;// = "";
        char * ncolor;// = "";
        char * textName;
        uint8_t _fresh = 0;
};

#endif
