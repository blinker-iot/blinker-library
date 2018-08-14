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
        }

        void color(const String & _clr) { 
            iconClr = _clr;
        }

        template <typename T>
        void content(T _con) { 
            bcon = STRING_format(_con); 
        }

        template <typename T>
        void text(T _text) { 
            btext = STRING_format(_text); 
        }

        template <typename T1, typename T2>
        void text(T1 _text1, T2 _text2) { 
            btext = STRING_format(_text1); btext1 = STRING_format(_text2); 
        }

        void textColor(const String & _clr) { 
            textClr = _clr; 
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
                buttonData += ",\""BLINKER_CMD_ICON"\":\"" + (bicon) + "\"";
            }

            if (iconClr.length()) {
                buttonData += ",\""BLINKER_CMD_COLOR"\":\"" + (iconClr) + "\"";
            }

            if (bcon.length()) {
                buttonData += ",\""BLINKER_CMD_CONTENT"\":\"" + (bcon) + "\"";
            }

            if (btext.length()) {
                buttonData += ",\""BLINKER_CMD_TEXT"\":\"" + (btext) + "\"";
            }

            if (btext1.length()) {
                buttonData += ",\""BLINKER_CMD_TEXT1"\":\"" + (btext1) + "\"";
            }

            if (textClr.length()) {
                buttonData += ",\""BLINKER_CMD_TEXTCOLOR"\":\"" + (textClr) + "\"";
            }

            buttonData += "}";

#ifdef BLINKER_DEBUG_ALL
            BLINKER_LOG2(BLINKER_F("buttonData: "), buttonData);

            BLINKER_LOG4(BLINKER_F("bicon: "), bicon, " ,", bicon.length());
            BLINKER_LOG4(BLINKER_F("iconClr: "), iconClr, " ,", iconClr.length());
            BLINKER_LOG4(BLINKER_F("bcon: "), bcon, " ,", bcon.length());
            BLINKER_LOG4(BLINKER_F("btext: "), btext, " ,", btext.length());
            BLINKER_LOG4(BLINKER_F("btext1: "), btext1, " ,", btext1.length());
            BLINKER_LOG4(BLINKER_F("textClr: "), textClr, " ,", textClr.length());
#endif

            Blinker.printArray(buttonName, buttonData);

            bicon = "";
            iconClr = "";
            bcon = "";
            btext = "";
            btext1 = "";
            textClr = "";

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
