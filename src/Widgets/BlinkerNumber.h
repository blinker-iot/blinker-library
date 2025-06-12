#ifndef BLINKER_NUMBER_H
#define BLINKER_NUMBER_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerNumber : public BlinkerWidget
{
    public :
        BlinkerNumber(const char* _name)
            : BlinkerWidget(_name)
        {}

        // 重写父类方法以支持方法链
        BlinkerNumber& icon(const String & _icon)
        {
            BlinkerWidget::icon(_icon);
            return *this;
        }

        BlinkerNumber& color(const String & _clr)
        {
            BlinkerWidget::color(_clr);
            return *this;
        }

        BlinkerNumber& unit(const String & _unit)
        {
            BlinkerWidget::unit(_unit);
            return *this;
        }

        template <typename T>
        BlinkerNumber& text(T _text)
        {
            BlinkerWidget::text(STRING_format(_text));
            return *this;
        }

        // 重写父类的value方法以返回BlinkerNumber类型
        template<typename T>
        BlinkerNumber& value(T _value)
        {
            BlinkerWidget::value(_value);
            return *this;
        }
        
        // 只保留无参数的print方法
        void print() override
        {
            String numberData = buildJsonData();
            
            if (numberData == BLINKER_F("{}")) return;

            clearData();

            Blinker.printArray(const_cast<char*>(name), numberData);
        }
};

#endif
