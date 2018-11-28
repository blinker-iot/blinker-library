#ifndef BLINKER_BRIDGE_H
#define BLINKER_BRIDGE_H

class BlinkerBridge
{
    public :
        BlinkerBridge(char _key[], blinker_callback_with_string_arg_t _func = NULL)
        {
            bNum = Blinker.attachBridge(_key, _func);
        }

        void attach(blinker_callback_with_string_arg_t _func)
        {
            if (wNum == 0) return;

            Blinker.freshAttachBridge(Blinker.bridgeKey(bNum), _func);
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

        // char *_bKey;
        // char *bridgeName;
};

#endif
