#ifndef BlinkerTab_H
#define BlinkerTab_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerTab
{
    public :
        BlinkerTab(char _name[], blinker_callback_with_table_arg_t _func = NULL,
                    blinker_callback_t _func2 = NULL)
        {
            wNum = Blinker.attachWidget(_name, _func, _func2);
            tabSet = 0;
        }

        void attach(blinker_callback_with_table_arg_t _func, blinker_callback_t _func2)
        {
            if (wNum == 0) return;

            Blinker.freshAttachWidget(Blinker.widgetName_tab(wNum), _func, _func2);
        }

        void tab(uint8_t num)
        {
            switch (num)
            {
                case 0:
                    tabSet |= 1 << 4;
                    break;
                case 1:
                    tabSet |= 1 << 3;
                    break;
                case 2:
                    tabSet |= 1 << 2;
                    break;
                case 3:
                    tabSet |= 1 << 1;
                    break;
                case 4:
                    tabSet |= 1 << 0;
                    break;
                default:
                    break;
            }
        }

        void tab_0() { tabSet |= 1 << 4; }
        void tab_1() { tabSet |= 1 << 3; }
        void tab_2() { tabSet |= 1 << 2; }
        void tab_3() { tabSet |= 1 << 1; }
        void tab_4() { tabSet |= 1 << 0; }

        void print()
        {
            if (wNum == 0) return;

            String tabData = "";

            tabData += BLINKER_F("{\"");
            tabData += BLINKER_F(BLINKER_CMD_VALUE);
            tabData += BLINKER_F("\":\"");
            // tabData += (_state);
            if (tabSet & 1 << 4) tabData += BLINKER_F("1");
            else tabData += BLINKER_F("0");
            
            if (tabSet & 1 << 3) tabData += BLINKER_F("1");
            else tabData += BLINKER_F("0");
            
            if (tabSet & 1 << 2) tabData += BLINKER_F("1");
            else tabData += BLINKER_F("0");
            
            if (tabSet & 1 << 1) tabData += BLINKER_F("1");
            else tabData += BLINKER_F("0");

            if (tabSet & 1 << 0) tabData += BLINKER_F("1");
            else tabData += BLINKER_F("0");
            
            tabData += BLINKER_F("\"}");

            tabSet = 0;

            Blinker.printArray(Blinker.widgetName_tab(wNum), tabData);
        }

    private :
        uint8_t wNum;
        uint8_t tabSet;
};

#endif