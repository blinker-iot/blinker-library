#ifndef BLINKER_BUTTON_H
#define BLINKER_BUTTON_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerButton : public BlinkerWidget
{
    public :
        BlinkerButton(const char* _name, 
            blinker_callback_with_string_arg_t _func = NULL)
            : BlinkerWidget(_name)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            bcon = nullptr;
            textClr = nullptr;
        }

        ~BlinkerButton()
        {
            if (bcon) free(bcon);
            if (textClr) free(textClr);
        }

        BlinkerButton& attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(name), _func);
            return *this;
        }

        // 重写父类的icon方法以返回BlinkerButton类型
        BlinkerButton& icon(const String& _icon)
        {
            BlinkerWidget::icon(_icon);
            return *this;
        }

        // 重写父类的color方法以返回BlinkerButton类型
        BlinkerButton& color(const String& _color)
        {
            BlinkerWidget::color(_color);
            return *this;
        }

        // 重写父类的text方法以返回BlinkerButton类型
        template <typename T>
        BlinkerButton& text(T _text)
        {
            BlinkerWidget::text(STRING_format(_text));
            return *this;
        }

        template <typename T1, typename T2>
        BlinkerButton& text(T1 _text1, T2 _text2)
        {
            BlinkerWidget::text(STRING_format(_text1), STRING_format(_text2));
            return *this;
        }

        // 重写父类的state方法以返回BlinkerButton类型
        BlinkerButton& state(const String& _state)
        {
            BlinkerWidget::state(_state);
            return *this;
        }

        // Button特有的content方法
        template <typename T>
        BlinkerButton& content(T _con)
        {
            if (_fresh >> 7 & 0x01) free(bcon);

            String _bcon = STRING_format(_con);
            bcon = (char*)malloc((_bcon.length()+1)*sizeof(char));
            strcpy(bcon, _bcon.c_str());

            _fresh |= 0x01 << 7;
            return *this;
        }

        // Button特有的textColor方法
        BlinkerButton& textColor(const String& _clr)
        {
            if (_fresh >> 8 & 0x01) free(textClr);

            textClr = (char*)malloc((_clr.length()+1)*sizeof(char));
            strcpy(textClr, _clr.c_str());

            _fresh |= 0x01 << 8;
            return *this;
        }

        // 实现父类的纯虚函数
        void print() override
        {
            if (_fresh == 0 || wNum == 0)
            {
                return;
            }

            String buttonData;
            bool hasContent = false;

            // 添加状态信息
            if (nstate)
            {
                buttonData += BLINKER_F("{\"");
                buttonData += BLINKER_F(BLINKER_CMD_SWITCH);
                buttonData += BLINKER_F("\":\"");
                buttonData += nstate;
                buttonData += BLINKER_F("\"");
                hasContent = true;
            }
            
            // 添加父类的通用属性
            if (_fresh & 0x3F) // 检查前6位（父类属性）
            {
                String parentData = buildJsonData();
                if (parentData.length() > 2) // 不是空的"{}"
                {
                    // 移除父类JSON的大括号
                    parentData = parentData.substring(1, parentData.length() - 1);
                    
                    if (hasContent) buttonData += BLINKER_F(",");
                    else buttonData += BLINKER_F("{");
                    
                    buttonData += parentData;
                    hasContent = true;
                }
            }
            
            // 添加Button特有的content属性
            if (_fresh >> 7 & 0x01)
            {
                if (hasContent) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_CONTENT);
                buttonData += BLINKER_F("\":\"");
                buttonData += bcon;
                buttonData += BLINKER_F("\"");
                
                free(bcon);
                bcon = nullptr;
                hasContent = true;
            }
            
            // 添加Button特有的textColor属性
            if (_fresh >> 8 & 0x01)
            {
                if (hasContent) buttonData += BLINKER_F(",");
                else buttonData += BLINKER_F("{");
                
                buttonData += BLINKER_F("\"");
                buttonData += BLINKER_F(BLINKER_CMD_TEXTCOLOR);
                buttonData += BLINKER_F("\":\"");
                buttonData += textClr;
                buttonData += BLINKER_F("\"");
                
                free(textClr);
                textClr = nullptr;
                hasContent = true;
            }

            if (hasContent) buttonData += BLINKER_F("}");

            // 清理所有数据
            clearData();
            _fresh = 0;

            if (hasContent) Blinker.printArray(const_cast<char*>(name), buttonData);
        }

    private :
        char*       bcon = nullptr;     // 按钮内容
        char*       textClr = nullptr;  // 文本颜色
};

#endif
