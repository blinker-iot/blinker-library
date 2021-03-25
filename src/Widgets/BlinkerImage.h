#ifndef BLINKER_IMAGE_H
#define BLINKER_IMAGE_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

template <class Functions>
class BlinkerImage
{
    public :
        BlinkerImage(Functions& nFunc, const char* _name, 
            blinker_callback_with_int32_arg_t _func = NULL)
            :   func(nFunc),
                name(_name)
        {
            wNum = func.attachWidget(name, _func);
        }

        void attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) wNum = func.attachWidget(name, _func);
            else func.freshAttachWidget(name, _func);
        }

        void print(uint8_t _num)
        {
            String ImageData = BLINKER_F("{\"img\":");
            ImageData += STRING_format(_num);
            ImageData += BLINKER_F("}");

            // Blinker.printArray(numName, ImageData);
            func.printArray(name, ImageData);
        }

    private :
        Functions&  func;
        const char* name;
        uint8_t     wNum;
};

#endif
