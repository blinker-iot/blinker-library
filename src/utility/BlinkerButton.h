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
        
        void name(const String & name) { buttonName = name; }
        String getName() { return buttonName; }
        // void setFunc(callback_with_string_arg_t newFunc) { _bfunc = newFunc; }
        // callback_with_string_arg_t getFunc() { return _bfunc; }
        void icon(const String & _icon) { bicon = _icon; }
        void iconColor(const String & _clr) { iconClr = _clr; }
        // String getIcon() { return _icon; }
        void text(const String & _text) { btext = _text; }
        void textColor(const String & _clr) { textClr = _clr; }
        // String getText() { return _text; }
        bool print(const String & _state) {
            if (!registered) {
                return false;
            }

            String buttonData = "[\"" + _state + "\",\"" + bicon + \
                                "\",\"" + iconClr + "\",\"" + btext + \
                                "\",\"" + textClr + "\"]";

            Blinker.printArray(buttonName, buttonData);
        }
        // bool checkName(String name) { return ((buttonName == name) ? true : false); }
    
    private :
        String buttonName;
        // callback_with_string_arg_t _bfunc = NULL;
        bool registered = false;
        String bicon = "";
        String iconClr = "";
        String btext = "";
        String textClr = "";
};

#endif
