#ifndef BlinkerText_H
#define BlinkerText_H

#include "Blinker/BlinkerConfig.h"
#include "utility/BlinkerUtility.h"

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
            String textData = BLINKER_F("{\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text);
            textData += BLINKER_F("\"}");

            Blinker.printArray(textName, textData);
        }

        template <typename T1, typename T2>
        void print(T1 _text1, T2 _text2)
        {
            String textData = BLINKER_F("{\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text1);
            textData += BLINKER_F("\",\"");
            textData += BLINKER_F(BLINKER_CMD_TEXT1);
            textData += BLINKER_F("\":\"");
            textData += STRING_format(_text2);
            textData += BLINKER_F("\"}");

            Blinker.printArray(textName, textData);
        }
    
    private :
        char * textName;
};

#endif