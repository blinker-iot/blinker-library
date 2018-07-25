#ifndef BlinkerButton_H
#define BlinkerButton_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerButton
{
    public :
        BlinkerButton(const String & _name, callback_with_string_arg_t _func)
            : buttonName(_name)
        {
            registered = Blinker.attachWidget(_name, _func);
        }
        
        void icon(const String & _icon) { bicon = _icon; }
        void iconColor(const String & _clr) { iconClr = _clr; }
        template <typename T>
        void content(T _con) { bcon = STRING_format(_con); }
        template <typename T>
        void text(T _text) { btext = STRING_format(_text); }
        void textColor(const String & _clr) { textClr = _clr; }
        void print(const String & _state) {
            if (!registered) {
                return;
            }

            // String buttonData = "{\"" + _state + "\",\"" + bicon + \
            //                     "\",\"" + iconClr + "\",\"" + btext + \
            //                     "\",\"" + textClr + "\"}";

            String buttonData = "{\"swi\":\"" + _state + "\"";

            if (bicon.length()) {
                buttonData += ",\"ico\":\"" + bicon + "\"";
            }
            if (iconClr.length()) {
                buttonData += ",\"col\":\"" + iconClr + "\"";
            }
            if (bcon.length()) {
                buttonData += ",\"con\":\"" + bcon + "\"";
            }
            if (btext.length()) {
                buttonData += ",\"tex\":\"" + btext + "\"";
            }
            if (textClr.length()) {
                buttonData += ",\"tco\":\"" + textClr + "\"";
            }

            buttonData += "}";

            Blinker.printArray(buttonName, buttonData);
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
        String textClr = "";
};

#endif
