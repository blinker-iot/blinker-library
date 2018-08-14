#ifndef BlinkerButton_H
#define BlinkerButton_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerButton
{
    public :
        BlinkerButton(const String & _name, callback_with_string_arg_t _func = NULL)
            : buttonName(_name)
        {
            // free(bicon);
            // free(iconClr);
            // free(bcon);
            // free(btext);
            // free(btext1);
            // free(textClr);
            
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void attach(callback_with_string_arg_t _func)
        {
            if (!registered) {
                return;
            }

            Blinker.freshAttachWidget(buttonName, _func);
        }

        void icon(const String & _icon) { 
            bicon = _icon; 
            // bicon = (char*)malloc(_icon.length()*sizeof(char));
            // strcpy(bicon, _icon.c_str());
        }
        void color(const String & _clr) { 
            iconClr = _clr; 
            // iconClr = (char*)malloc(_clr.length()*sizeof(char));
            // strcpy(iconClr, _clr.c_str());
        }
        template <typename T>
        void content(T _con) { 
            bcon = STRING_format(_con); 
            // String _bcon = STRING_format(_con); 
            // bcon = (char*)malloc(_bcon.length()*sizeof(char));
            // strcpy(bcon, _bcon.c_str());
        }
        template <typename T>
        void text(T _text) { 
            btext = STRING_format(_text); 
            // String _btext = STRING_format(_text); 
            // btext = (char*)malloc(_btext.length()*sizeof(char));
            // strcpy(btext, _btext.c_str());
        }
        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2) { 
            btext = STRING_format(_text1); btext1 = STRING_format(_text2); 
            // String _btext = STRING_format(_text1); 
            // btext = (char*)malloc(_btext.length()*sizeof(char));
            // strcpy(btext, _btext.c_str());
            // _btext = STRING_format(_text2); 
            // btext1 = (char*)malloc(_btext.length()*sizeof(char));
            // strcpy(btext1, _btext.c_str());
        }
        void textColor(const String & _clr) { 
            textClr = _clr; 
            // String _textClr = STRING_format(_clr); 
            // textClr = (char*)malloc(_textClr.length()*sizeof(char));
            // strcpy(textClr, _textClr.c_str());
        }

        void print() { print(""); }

        void print(const String & _state)
        {
            if (!registered) {
                return;
            }

            // String buttonData = "{\"" + _state + "\",\"" + bicon + \
            //                     "\",\"" + iconClr + "\",\"" + btext + \
            //                     "\",\"" + textClr + "\"}";

            String buttonData = "{\""BLINKER_CMD_SWITCH"\":\"" + _state + "\"";

            if (bicon.length()) {
            // if (strlen(bicon)) {
                buttonData += ",\""BLINKER_CMD_ICON"\":\"" + STRING_format(bicon) + "\"";
            }
            if (iconClr.length()) {
            // if (strlen(iconClr)) {
                buttonData += ",\""BLINKER_CMD_COLOR"\":\"" + STRING_format(iconClr) + "\"";
            }
            if (bcon.length()) {
            // if (strlen(bcon)) {
                buttonData += ",\""BLINKER_CMD_CONTENT"\":\"" + STRING_format(bcon) + "\"";
            }
            if (btext.length()) {
            // if (strlen(btext)) {
                buttonData += ",\""BLINKER_CMD_TEXT"\":\"" + STRING_format(btext) + "\"";
            }
            if (btext1.length()) {
            // if (strlen(btext1)) {
                buttonData += ",\""BLINKER_CMD_TEXT1"\":\"" + STRING_format(btext1) + "\"";
            }
            if (textClr.length()) {
            // if (strlen(textClr)) {
                buttonData += ",\""BLINKER_CMD_TEXTCOLOR"\":\"" + STRING_format(textClr) + "\"";
            }

            buttonData += "}";

            Blinker.printArray(buttonName, buttonData);

            // // bicon = "";
            // // iconClr = "";
            // // bcon = "";
            // // btext = "";
            // // btext1 = "";
            // // textClr = "";

            // free(bicon);
            // free(iconClr);
            // free(bcon);
            // free(btext);
            // free(btext1);
            // free(textClr);
        }
        // bool checkName(String name) { return ((buttonName == name) ? true : false); }
    
    private :
        String buttonName;
        // callback_with_string_arg_t _bfunc = NULL;
        bool registered = false;
        String bicon = "";
        String iconClr = "";
        String bcon = "";
        String btext = "";
        String btext1 = "";
        String textClr = "";
        // char * bicon = "";
        // char * iconClr = "";
        // char * bcon = "";
        // char * btext = "";
        // char * btext1 = "";
        // char * textClr = "";
};

#endif
