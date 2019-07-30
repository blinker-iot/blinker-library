#ifndef BlinkerButton_H
#define BlinkerButton_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerButton
{
    public :
        BlinkerButton(char _name[], blinker_callback_with_string_arg_t _func = NULL)
        {
            wNum = Blinker.attachWidget(_name, _func);
        }

        void attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) return;

            Blinker.freshAttachWidget(Blinker.widgetName_str(wNum), _func);
        }

        void icon(const String & _icon)
        {
            if (_fresh >> 0 & 0x01) free(bicon);

            bicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(bicon, _icon.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & _clr)
        {
            if (_fresh >> 1 & 0x01) free(iconClr);

            iconClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(iconClr, _clr.c_str());

            _fresh |= 0x01 << 1;
        }

        template <typename T>
        void content(T _con)
        {
            if (_fresh >> 2 & 0x01) free(bcon);

            String _bcon = STRING_format(_con);
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());

            _fresh |= 0x01 << 2;
        }

        template <typename T>
        void text(T _text)
        {
            if (_fresh >> 3 & 0x01) free(btext);

            String _btext = STRING_format(_text);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _fresh |= 0x01 << 3;
        }

        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2)
        {
            if (_fresh >> 3 & 0x01) free(btext);

            String _btext = STRING_format(_text1);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _fresh |= 0x01 << 3;

            if (_fresh >> 4 & 0x01) free(btext1);

            _btext = STRING_format(_text2);
            btext1 = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext1, _btext.c_str());

            _fresh |= 0x01 << 4;
        }

        void textColor(const String & _clr)
        {
            if (_fresh >> 5 & 0x01) free(textClr);

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 5;
        }

        void print() { print(""); }

        void print(const String & _state)
        {
            if ((_fresh == 0 && _state.length() == 0) || \
                wNum == 0)
            {
                return;
            }

            String buttonData;

            if (_state.length())
            {
                buttonData += BLINKER_F("{\"");
                buttonData += BLINKER_F(BLINKER_CMD_SWITCH);
                buttonData += BLINKER_F("\":\"");
                buttonData += (_state);
                buttonData += BLINKER_F("\"");
            }
            
            if (_fresh >> 0 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_ICON);
                buttonData += BLINKER_F("\":\"");
                buttonData += (bicon);
                buttonData += BLINKER_F("\"");
                
                free(bicon);
            }
            
            if (_fresh >> 1 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += STRING_format(BLINKER_F("{"));
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_COLOR);
                buttonData += BLINKER_F("\":\"");
                buttonData += (iconClr);
                buttonData += BLINKER_F("\"");
                
                free(iconClr);
            }
            
            if (_fresh >> 2 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_CONTENT);
                buttonData += BLINKER_F("\":\"");
                buttonData += (bcon);
                buttonData += BLINKER_F("\"");
                
                free(bcon);
            }
            
            if (_fresh >> 3 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");

                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_TEXT);
                buttonData += BLINKER_F("\":\"");
                buttonData += (btext);
                buttonData += BLINKER_F("\"");
                
                free(btext);
            }
            
            if (_fresh >> 4 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_TEXT1);
                buttonData += BLINKER_F("\":\"");
                buttonData += (btext1);
                buttonData += BLINKER_F("\"");
                
                free(btext1);
            }
            
            if (_fresh >> 5 & 0x01)
            {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_TEXTCOLOR);
                buttonData += BLINKER_F("\":\"");
                buttonData += (textClr);
                buttonData += BLINKER_F("\"");
                
                free(textClr);
            }

            buttonData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(Blinker.widgetName_str(wNum), buttonData);
        }

    private :
        uint8_t wNum;
        
        char * bicon;
        char * iconClr;
        char * bcon;
        char * btext;
        char * btext1;
        char * textClr;
        uint8_t _fresh = 0;
};

#endif
