#ifndef BLINKER_IMAGE_H
#define BLINKER_IMAGE_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"
#include "BlinkerWidgets.h"

class BlinkerImage : public BlinkerWidget
{
    public :
        BlinkerImage(const char* _name, 
            blinker_callback_with_int32_arg_t _func = NULL)
            : BlinkerWidget(_name), currentImg(0)
        {
            wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
        }

        BlinkerImage& attach(blinker_callback_with_int32_arg_t _func)
        {
            if (wNum == 0) wNum = Blinker.attachWidget(const_cast<char*>(name), _func);
            else Blinker.freshAttachWidget(const_cast<char*>(name), _func);
            return *this;
        }

        void print(uint8_t _num)
        {
            currentImg = _num;
            print();
        }

        void print() override
        {
            String ImageData = BLINKER_F("{\"img\":");
            ImageData += STRING_format(currentImg);
            ImageData += BLINKER_F("}");

            Blinker.printArray(const_cast<char*>(name), ImageData);
        }

    private :
        uint8_t     currentImg;
};

#endif
