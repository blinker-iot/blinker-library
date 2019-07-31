#ifndef BLINKER_BRIDGE_H
#define BLINKER_BRIDGE_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerBridge
{
    public :
        BlinkerBridge(char _key[], blinker_callback_with_string_arg_t _func = NULL)
        {
            bNum = Blinker.attachBridge(_key, _func);
        }

        void attach(blinker_callback_with_string_arg_t _func)
        {
            if (bNum == 0) return;

            Blinker.freshAttachBridge(Blinker.bridgeKey(bNum), _func);
        }

        template <typename T1>
        void print(T1 n1)
        {
            BLINKER_LOG_ALL(BLINKER_F("Bridge to: "), bNum, BLINKER_F(", data: "), n1);
            if (strcmp(Blinker.bridgeName(bNum), BLINKER_CMD_FALSE) != 0)
            {
                Blinker.bridgePrint(Blinker.bridgeName(bNum), STRING_format(n1));
            }
        }

        template <typename T1, typename T2>
        void print(T1 n1, T2 n2)
        {
            if (strcmp(Blinker.bridgeName(bNum), BLINKER_CMD_FALSE) != 0)
            {
                String msg = BLINKER_F("{\"");
                msg += STRING_format(n1);
                msg += BLINKER_F("\":\"");
                msg += STRING_format(n2);
                msg += BLINKER_F("\"}");

                Blinker.bridgePrint(Blinker.bridgeName(bNum), msg);
            }
        }

        // void name(char name[])
        // {
        //     _bName = (char*)malloc((strlen(name)+1)*sizeof(char));
        //     strcpy(_bName, name);
        // }
        // char * getName() { return _bName; }
        // void freshBridge(const String & name)
        // {
        //     bridgeName = (char*)malloc((name.length()+1)*sizeof(char));
        //     strcpy(bridgeName, name.c_str());
        // }
        // char * getBridge() { return bridgeName; }
        // bool checkName(char name[]) { return strcmp(_bName, name) == 0; }

    private :
        uint8_t bNum;
};

#endif
