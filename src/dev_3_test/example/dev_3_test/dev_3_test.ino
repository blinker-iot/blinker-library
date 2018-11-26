#define BLINKER_ESP_SMARTCONFIG

#include "Blinker4.h"

#define BLINKER_PRINT Serial

char auth[] = "bc5a991c7ec4";

// char* test;//[100] = "12345678";

// void charTest(char * _data)
// {
//     uint8_t num = strlen(_data);
//     for(uint8_t c_num = num; c_num > 0; c_num--)
//     {
//         _data[c_num+7] = _data[c_num-1];
//     }

//     BLINKER_LOG("char _data: ", _data, " , num: ", num);

//     for(uint8_t c_num = 0; c_num < 8; c_num++)
//     {
//         _data[c_num] = '1';
//     }

//     BLINKER_LOG("char _data: ", _data, " , num: ", num);
// }

void setup()
{
    // test = (char*)malloc(100*sizeof(char));
    // String data = "12345678";
    

    Serial.begin(115200);

    #if defined(BLINKER_PRINT)
    BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif
    BLINKER_DEBUG.debugAll();
    BLINKER_DEBUG.freeheap();

    Blinker.begin(auth);

    // strcpy(test, data.c_str());
    // charTest(test);
    // BLINKER_LOG("char test: ", test);

    // Blinker.hello();

    // BLINKER_LOG("hello", F("world"), 123, 456.78, -1);
}

void loop()
{
    Blinker.run();

    if (Blinker.available())
    {
        BLINKER_LOG_FreeHeap();
        BLINKER_LOG("Blinker.readString(): ", Blinker.readString());
        BLINKER_LOG_FreeHeap();
        Blinker.print("state", "online");
        Blinker.checkState(false);
        BLINKER_LOG_FreeHeap();
    }
}
