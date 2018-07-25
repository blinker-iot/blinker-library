#ifndef BlinkerText_H
#define BlinkerText_H

#include <Blinker/BlinkerConfig.h>
#include <utility/BlinkerUtility.h>

class BlinkerText
{
    public :
        BlinkerText(const String & _name)
            : textName(_name)
        {}
        
        template <typename T>
        void title(T _title) { tTitle = STRING_format(_title); }
        template <typename T>
        void print(T _content) {
            String textData = "[\"" + tTitle + "\",\"" + STRING_format(_content) + "\"]";

            Blinker.printArray(textName, textData);
        }
        // bool checkName(String name) { return ((buttonName == name) ? true : false); }
    
    private :
        String textName;
        // callback_with_string_arg_t _bfunc = NULL;
        // bool registered = false;
        String tTitle = "";
        // String bicon = "";
        // String iconClr = "";
        // String btext = "";
        // String textClr = "";
};

#endif