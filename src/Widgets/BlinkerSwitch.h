#ifndef BLINKER_SWITCH_H
#define BLINKER_SWITCH_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerSwitch : public BlinkerWidget
{
    public :
        static BlinkerSwitch& getInstance()
        {
            static BlinkerSwitch instance;
            return instance;
        }

        BlinkerSwitch(const BlinkerSwitch&) = delete;
        BlinkerSwitch& operator=(const BlinkerSwitch&) = delete;
        BlinkerSwitch& attach(blinker_callback_with_string_arg_t _func)
        {
            ensureInitialized();
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), _func);
            return *this;
        }

        void print(const String & _state)
        {
            ensureInitialized();
            if (_state.length() == 0 || wNum == 0)
            {
                return;
            }

            Blinker.print(BLINKER_CMD_BUILTIN_SWITCH, _state);
        }

        void print() override
        {
            print(currentState);
        }

        BlinkerSwitch& state(const String & _state)
        {
            currentState = _state;
            return *this;
        }

    private :
        BlinkerSwitch() : BlinkerWidget(BLINKER_CMD_BUILTIN_SWITCH)
        {
        }

        String      currentState = "";
        bool        isInitialized = false;        void ensureInitialized()
        {
            if (!isInitialized)
            {
                // 使用明确的类型转换避免重载歧义
                blinker_callback_with_string_arg_t nullCallback = nullptr;
                wNum = Blinker.attachWidget(const_cast<char*>(BLINKER_CMD_BUILTIN_SWITCH), nullCallback);
                isInitialized = true;
            }
        }
};

#define BLINKER_BUILTIN_SWITCH BlinkerSwitch::getInstance()

#endif
