#ifndef BLINKER_WIDGETS_H
#define BLINKER_WIDGETS_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerWidget
{
    public:
        BlinkerWidget(const char* _name) 
            : name(_name), wNum(0), _fresh(0)
        {
            nicon = nullptr;
            ncolor = nullptr;
            ntext1 = nullptr;
            ntext2 = nullptr;
            nstate = nullptr;
            nvalue = nullptr;
            nunit = nullptr;
        }

        virtual ~BlinkerWidget()
        {
            if (nicon) free(nicon);
            if (ncolor) free(ncolor);
            if (ntext1) free(ntext1);
            if (ntext2) free(ntext2);
            if (nstate) free(nstate);
            if (nvalue) free(nvalue);
            if (nunit) free(nunit);
        }

        // 设置图标
        BlinkerWidget& icon(const String& _icon)
        {
            if (_fresh >> 0 & 0x01) free(nicon);

            nicon = (char*)malloc((_icon.length() + 1) * sizeof(char));
            strcpy(nicon, _icon.c_str());

            _fresh |= 0x01 << 0;
            return *this;
        }

        // 设置颜色
        BlinkerWidget& color(const String& _color)
        {
            if (_fresh >> 1 & 0x01) free(ncolor);

            ncolor = (char*)malloc((_color.length() + 1) * sizeof(char));
            strcpy(ncolor, _color.c_str());

            _fresh |= 0x01 << 1;
            return *this;
        }

        // 设置文本（单行）
        BlinkerWidget& text(const String& _text)
        {
            if (_fresh >> 2 & 0x01) free(ntext1);

            ntext1 = (char*)malloc((_text.length() + 1) * sizeof(char));
            strcpy(ntext1, _text.c_str());

            _fresh |= 0x01 << 2;
            return *this;
        }

        // 设置文本（双行）
        BlinkerWidget& text(const String& _text1, const String& _text2)
        {
            if (_fresh >> 2 & 0x01) free(ntext1);
            if (_fresh >> 3 & 0x01) free(ntext2);

            ntext1 = (char*)malloc((_text1.length() + 1) * sizeof(char));
            strcpy(ntext1, _text1.c_str());

            ntext2 = (char*)malloc((_text2.length() + 1) * sizeof(char));
            strcpy(ntext2, _text2.c_str());

            _fresh |= 0x01 << 2;
            _fresh |= 0x01 << 3;
            return *this;
        }

        // 设置状态
        BlinkerWidget& state(const String& _state)
        {
            if (_fresh >> 4 & 0x01) free(nstate);

            nstate = (char*)malloc((_state.length() + 1) * sizeof(char));
            strcpy(nstate, _state.c_str());

            _fresh |= 0x01 << 4;
            return *this;
        }

        // 设置数值
        template<typename T>
        BlinkerWidget& value(T _value)
        {
            String valueStr = STRING_format(_value);
            
            if (_fresh >> 5 & 0x01) free(nvalue);

            nvalue = (char*)malloc((valueStr.length() + 1) * sizeof(char));
            strcpy(nvalue, valueStr.c_str());

            _fresh |= 0x01 << 5;
            return *this;
        }

        // 设置单位
        BlinkerWidget& unit(const String& _unit)
        {
            if (_fresh >> 6 & 0x01) free(nunit);

            nunit = (char*)malloc((_unit.length() + 1) * sizeof(char));
            strcpy(nunit, _unit.c_str());

            _fresh |= 0x01 << 6;
            return *this;
        }

        // 获取组件名称
        const char* getName() const
        {
            return name;
        }

        // 获取组件编号
        uint8_t getNum() const
        {
            return wNum;
        }

        // 纯虚函数，子类必须实现
        virtual void print() = 0;

    protected:
        // 构建通用JSON数据
        String buildJsonData()
        {
            String jsonData = BLINKER_F("{");
            bool hasContent = false;

            if (_fresh >> 0 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_ICON);
                jsonData += BLINKER_F("\":\"");
                jsonData += nicon;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            if (_fresh >> 1 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_COLOR);
                jsonData += BLINKER_F("\":\"");
                jsonData += ncolor;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            if (_fresh >> 2 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_TEXT);
                jsonData += BLINKER_F("\":\"");
                jsonData += ntext1;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            if (_fresh >> 3 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_TEXT1);
                jsonData += BLINKER_F("\":\"");
                jsonData += ntext2;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            if (_fresh >> 4 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_STATE);
                jsonData += BLINKER_F("\":\"");
                jsonData += nstate;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            if (_fresh >> 5 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_VALUE);
                jsonData += BLINKER_F("\":");
                jsonData += nvalue;
                hasContent = true;
            }

            if (_fresh >> 6 & 0x01) {
                if (hasContent) jsonData += BLINKER_F(",");
                jsonData += BLINKER_F("\"");
                jsonData += BLINKER_F(BLINKER_CMD_UNIT);
                jsonData += BLINKER_F("\":\"");
                jsonData += nunit;
                jsonData += BLINKER_F("\"");
                hasContent = true;
            }

            jsonData += BLINKER_F("}");
            return jsonData;
        }

        // 清理所有数据并重置标志
        void clearData()
        {
            if (_fresh >> 0 & 0x01) { free(nicon); nicon = nullptr; }
            if (_fresh >> 1 & 0x01) { free(ncolor); ncolor = nullptr; }
            if (_fresh >> 2 & 0x01) { free(ntext1); ntext1 = nullptr; }
            if (_fresh >> 3 & 0x01) { free(ntext2); ntext2 = nullptr; }
            if (_fresh >> 4 & 0x01) { free(nstate); nstate = nullptr; }
            if (_fresh >> 5 & 0x01) { free(nvalue); nvalue = nullptr; }
            if (_fresh >> 6 & 0x01) { free(nunit); nunit = nullptr; }

            _fresh = 0;
        }

        // 检查是否有数据需要发送
        bool hasData() const
        {
            return _fresh != 0;
        }

    protected:
        const char* name;       // 组件名称
        uint8_t     wNum;       // 组件编号
        uint8_t     _fresh;     // 数据更新标志

        char*       nicon;      // 图标
        char*       ncolor;     // 颜色
        char*       ntext1;     // 文本1
        char*       ntext2;     // 文本2
        char*       nstate;     // 状态
        char*       nvalue;     // 数值
        char*       nunit;      // 单位
};

#endif