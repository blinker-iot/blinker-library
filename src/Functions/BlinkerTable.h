#ifndef BlinkerTable_H
#define BlinkerTable_H

#include "Blinker/BlinkerConfig.h"
#include "Blinker/BlinkerUtility.h"

class BlinkerTable
{
    public :
        BlinkerTable(char _name[], blinker_callback_with_table_arg_t _func = NULL)
        {
            wNum = Blinker.attachWidget(_name, _func);
        }

        void attach(blinker_callback_with_table_arg_t _func)
        {
            if (wNum == 0) return;

            Blinker.freshAttachWidget(Blinker.widgetName_tab(wNum), _func);
        }

        void table_0()
        {
            tabSet = tabSet | (1 << 4);
        }

        void table_1()
        {
            tabSet = tabSet | (1 << 3);
        }

        void table_2()
        {
            tabSet = tabSet | (1 << 2);
        }

        void table_3()
        {
            tabSet = tabSet | (1 << 1);
        }

        void table_4()
        {
            tabSet = tabSet | (1 << 0);
        }

        void print()
        {
            if (wNum == 0) return;

            String tabData;

            tabData += BLINKER_F("{\"");
            tabData += BLINKER_F(BLINKER_CMD_VALUE);
            tabData += BLINKER_F("\":\"");

            BLINKER_LOG_ALL(BLINKER_F("tabSet: "), tabSet);

            if ((tabSet >> 4) & 0x01) { tabData += "1"; }
            else { tabData += "0"; }
            
            if ((tabSet >> 3) & 0x01) { tabData += "1"; }
            else { tabData += "0"; }

            if ((tabSet >> 2) & 0x01) { tabData += "1"; }
            else { tabData += "0"; }

            if ((tabSet >> 1) & 0x01) { tabData += "1"; }
            else { tabData += "0"; }

            if ((tabSet >> 0) & 0x01) { tabData += "1"; }
            else { tabData += "0"; }

            tabData += BLINKER_F("\"}");

            tabSet = 0;

            Blinker.printArray(Blinker.widgetName_tab(wNum), tabData);
        }

    private :
        uint8_t wNum;
        uint8_t tabSet = 0;
};

#endif
