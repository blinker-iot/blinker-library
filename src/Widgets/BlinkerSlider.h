#ifndef BLINKER_SLIDER_H
#define BLINKER_SLIDER_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerSlider : public BlinkerWidget
{
    public :
        BlinkerSlider(const char* _name, 
            blinker_callback_with_int32_arg_t _func = NULL)
            : BlinkerWidget(_name)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
        }
        BlinkerSlider& attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(name), _func);
            return *this;
        }
        
        // 重写父类的color方法以支持方法链
        BlinkerSlider& color(const String & _clr)
        {
            BlinkerWidget::color(_clr);
            return *this;
        }
          void print(char value)              { print(STRING_format(value)); }
        void print(unsigned char value)     { print(STRING_format(value)); }
        void print(int value)               { print(STRING_format(value)); }
        void print(unsigned int value)      { print(STRING_format(value)); }       
        void print(long value)              { print(STRING_format(value)); }        
        void print(unsigned long value)     { print(STRING_format(value)); }
        void print(double value)            { print(STRING_format(value)); }
        
        void print(const String& n)
        {
            value(n);
            print();
        }
        
        void print() override
        {
            if (wNum == 0) return;

            String sliderData = buildJsonData();
            
            if (sliderData == BLINKER_F("{}")) return;

            clearData();

            Blinker.printArray(const_cast<char*>(name), sliderData);
        }};

#endif
