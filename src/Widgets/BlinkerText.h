#ifndef BLINKER_TEXT_H
#define BLINKER_TEXT_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerText : public BlinkerWidget
{
    public :
        BlinkerText(const char* _name)
            : BlinkerWidget(_name)
        {}
          template <typename T>
        void print(T _text)
        {
            text(STRING_format(_text));
            print();
        }

        template <typename T1, typename T2>
        void print(T1 _text1, T2 _text2)
        {
            text(STRING_format(_text1), STRING_format(_text2));
            print();
        }

        void print() override
        {
            String textData = buildJsonData();
            
            if (textData == BLINKER_F("{}")) return;

            clearData();

            Blinker.printArray(const_cast<char*>(name), textData);
        }

        // 重写父类方法以支持方法链
        BlinkerText& icon(const String & _icon)
        {
            BlinkerWidget::icon(_icon);
            return *this;
        }

        BlinkerText& color(const String & _clr)
        {
            BlinkerWidget::color(_clr);
            return *this;
        }
};

#endif
