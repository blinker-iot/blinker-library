#ifndef BlinkerImage_H
#define BlinkerImage_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerImage
{
    public :
        BlinkerImage(char _name[], blinker_callback_with_int32_arg_t _func = NULL)
        {
            wNum = Blinker.attachWidget(_name, _func);
            // numName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            // strcpy(numName, _name);
        }
        
        void attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) return;

            Blinker.freshAttachWidget(Blinker.widgetName_int(wNum), _func);
        }

        uint8_t getNum() { return num; }

        void print(uint8_t _num)
        {
            num = _num;

            String ImageData = BLINKER_F("{\"img\":");
            ImageData += STRING_format(num);
            ImageData += BLINKER_F("}");

            // Blinker.printArray(numName, ImageData);
            Blinker.printArray(Blinker.widgetName_int(wNum), ImageData);
        }

    private :
        uint8_t wNum;
        uint8_t num = 0;
};

#endif
