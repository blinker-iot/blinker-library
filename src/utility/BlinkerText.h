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
        
        // template <typename T>
        // void title(T _title) { tTitle = STRING_format(_title); }
        
        template <typename T>
        void print(T _text) {
            String textData = "{\""BLINKER_CMD_TEXT"\":\"" + STRING_format(_text) + "\"}";

            Blinker.printArray(textName, textData);
        }

        template <typename T1, typename T2>
        void print(T1 _text1, T2 _text2) {
            String textData = "{\""BLINKER_CMD_TEXT"\":\"" + STRING_format(_text1) + "\"," + \
                                "\""BLINKER_CMD_TEXT1"\":" + STRING_format(_text2) + "\"}";

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