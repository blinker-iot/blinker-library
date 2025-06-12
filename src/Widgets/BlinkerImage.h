#ifndef BLINKER_IMAGE_H
#define BLINKER_IMAGE_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerImage
{
    public :
        BlinkerImage(const char* _name, 
            blinker_callback_with_int32_arg_t _func = NULL)
            : name(_name)
        {
            wNum = Blinker.attachWidget(name, _func);
        }

        BlinkerImage& attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(name, _func);
            else Blinker.freshAttachWidget(name, _func);
            return *this;
        }

        void print(uint8_t _num)
        {
            String ImageData = BLINKER_F("{\"img\":");
            ImageData += STRING_format(_num);
            ImageData += BLINKER_F("}");

            Blinker.printArray(name, ImageData);
        }

    private :
        const char* name;
        uint8_t     wNum = 0;
};

#endif
