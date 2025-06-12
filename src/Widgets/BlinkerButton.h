#ifndef BLINKER_BUTTON_H
#define BLINKER_BUTTON_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerButton
{
    public :
        BlinkerButton(const char* _name, 
            blinker_callback_with_string_arg_t _func = NULL)
            : name(_name)
        {
            wNum = Blinker.attachWidget(name, _func);
        }

        BlinkerButton& attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(name, _func);
            else Blinker.freshAttachWidget(name, _func);
            return *this;
        }

        BlinkerButton& icon(const String & _icon)
        {
            if (_fresh >> 0 & 0x01) free(bicon);

            bicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(bicon, _icon.c_str());

            _fresh |= 0x01 << 0;
            return *this;
        }

        BlinkerButton& color(const String & _clr)
        {
            if (_fresh >> 1 & 0x01) free(iconClr);

            iconClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(iconClr, _clr.c_str());

            _fresh |= 0x01 << 1;
            return *this;
        }

        template <typename T>
        BlinkerButton& content(T _con)
        {
            if (_fresh >> 2 & 0x01) free(bcon);

            String _bcon = STRING_format(_con);
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());

            _fresh |= 0x01 << 2;
            return *this;
        }

        template <typename T>
        BlinkerButton& text(T _text)
        {
            if (_fresh >> 3 & 0x01) free(btext);

            String _btext = STRING_format(_text);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _fresh |= 0x01 << 3;
            return *this;
        }

        template <typename T1, typename T2>
        BlinkerButton& text(T1 _text1, T2 _text2)
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
            return *this;
        }

        BlinkerButton& textColor(const String & _clr)
        {
            if (_fresh >> 5 & 0x01) free(textClr);

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 5;
            return *this;
        }

        BlinkerButton& state(const String & _state)
        {
            currentState = _state;
            return *this;
        }

        void print() { print(""); }

        void print(const String & _state)
        {
            String stateToUse = _state.length() ? _state : currentState;
            
            if ((_fresh == 0 && stateToUse.length() == 0) || wNum == 0)
            {
                return;
            }

            String buttonData;

            if (stateToUse.length())
            {
                buttonData += BLINKER_F("{\"");
                buttonData += BLINKER_F(BLINKER_CMD_SWITCH);
                buttonData += BLINKER_F("\":\"");
                buttonData += (stateToUse);
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
            currentState = "";

            Blinker.printArray(name, buttonData);
        }

    private :
        const char* name;
        uint8_t     wNum = 0;
        char *      bicon = nullptr;
        char *      iconClr = nullptr;
        char *      bcon = nullptr;
        char *      btext = nullptr;
        char *      btext1 = nullptr;
        char *      textClr = nullptr;
        uint8_t     _fresh = 0;
        String      currentState = "";
};
#endif
