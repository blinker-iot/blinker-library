#ifndef BlinkerImage_H
#define BlinkerImage_H

#include "../Blinker/BlinkerConfig.h"
#include "../Blinker/BlinkerUtility.h"

class BlinkerImage
{
    public :
        BlinkerImage(char _name[])
        {
            numName = (char*)malloc((strlen(_name)+1)*sizeof(char));
            strcpy(numName, _name);
        }

        uint8_t getNum() { return num; }

        void print(uint8_t _num)
        {
            num = _num;

            String ImageData = BLINKER_F("{\"img\":");
            ImageData += STRING_format(num);
            ImageData += BLINKER_F("}");

            Blinker.printArray(numName, ImageData);
        }

    private :
        char * numName;
        uint8_t num = 0;
};

#endif
