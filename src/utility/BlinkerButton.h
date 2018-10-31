#ifndef BlinkerButton_H
#define BlinkerButton_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerButton
{
    public :
        BlinkerButton(const String & _name, callback_with_string_arg_t _func = NULL)
            // : buttonName(_name)
        {
            wNum = Blinker.attachWidget(_name, _func);

            // wNum ? (registered = true) : (registered = false);

            // buttonName = (char*)malloc((_name.length()+1)*sizeof(char));
            // strcpy(buttonName, _name.c_str());
        }

        void attach(callback_with_string_arg_t _func)
        {
            if (!wNum) {
                return;
            }

            Blinker.freshAttachWidget(Blinker.widgetName_str(wNum), _func);
        }

        void icon(const String & _icon) {
            bicon = (char*)malloc((_icon.length()+1)*sizeof(char));
            strcpy(bicon, _icon.c_str());

            _fresh |= 0x01 << 0;
        }

        void color(const String & _clr) {
            iconClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(iconClr, _clr.c_str());

            _fresh |= 0x01 << 1;
        }

        template <typename T>
        void content(T _con) {
            String _bcon = STRING_format(_con);
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());

            _fresh |= 0x01 << 2;
        }

        template <typename T>
        void text(T _text) {
            String _btext = STRING_format(_text);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _fresh |= 0x01 << 3;
        }

        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2) {
            String _btext = STRING_format(_text1);
            btext = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext, _btext.c_str());

            _fresh |= 0x01 << 3;

            _btext = STRING_format(_text2);
            btext1 = (char*)malloc((_btext.length()+1)*sizeof(char));
            strcpy(btext1, _btext.c_str());

            _fresh |= 0x01 << 4;
        }

        void textColor(const String & _clr) {
            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 5;
        }

        void print() { print(""); }

        void print(const String & _state)
        {
            if (!wNum) {
                return;
            }

            String buttonData;

            if (_state.length()) {
                buttonData += BLINKER_F("{\""BLINKER_CMD_SWITCH"\":\"");
                buttonData += (_state);
                buttonData += BLINKER_F("\"");
            }

            // if (bicon && (_fresh >> 0 & 0x01)) {
            if (_fresh >> 0 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\""BLINKER_CMD_ICON"\":\"");
                buttonData += (bicon);
                buttonData += BLINKER_F("\"");
                
                free(bicon);
            }
            
            // if (iconClr && (_fresh >> 1 & 0x01)) {
            if (_fresh >> 1 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += STRING_format(BLINKER_F("{"));
                
                buttonData += BLINKER_F("\""BLINKER_CMD_COLOR"\":\"");
                buttonData += (iconClr);
                buttonData += BLINKER_F("\"");
                
                free(iconClr);
            }
            
            // if (bcon && (_fresh >> 2 & 0x01)) {
            if (_fresh >> 2 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\""BLINKER_CMD_CONTENT"\":\"");
                buttonData += (bcon);
                buttonData += BLINKER_F("\"");
                
                free(bcon);
            }
            
            // if (btext && (_fresh >> 3 & 0x01)) {
            if (_fresh >> 3 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");

                buttonData += BLINKER_F("\""BLINKER_CMD_TEXT"\":\"");
                buttonData += (btext);
                buttonData += BLINKER_F("\"");
                
                free(btext);
            }
            
            // if (btext1 && (_fresh >> 4 & 0x01)) {
            if (_fresh >> 4 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\""BLINKER_CMD_TEXT1"\":\"");
                buttonData += (btext1);
                buttonData += BLINKER_F("\"");
                
                free(btext1);
            }
            
            // if (textClr && (_fresh >> 5 & 0x01)) {
            if (_fresh >> 5 & 0x01) {
                if (buttonData.length()) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\""BLINKER_CMD_TEXTCOLOR"\":\"");
                buttonData += (textClr);
                buttonData += BLINKER_F("\"");
                
                free(textClr);
            }

            buttonData += BLINKER_F("}");

            _fresh = 0;

            Blinker.printArray(Blinker.widgetName_str(wNum), buttonData);
        }

    private :
        // char * buttonName;
        uint8_t wNum;
        
        // bool registered = false;
        
        char * bicon;
        char * iconClr;
        char * bcon;
        char * btext;
        char * btext1;
        char * textClr;
        uint8_t _fresh = 0;
};

#endif
