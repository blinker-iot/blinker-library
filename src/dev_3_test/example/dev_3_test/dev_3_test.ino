// #define BLINKER_ESP_SMARTCONFIG
// #define BLINKER_ALIGENIE_OUTLET
#include "Blinker4.h"

#define BLINKER_PRINT Serial

char auth[] = "bc5a991c7ec4";
char ssid[] = "mostfun";
char pswd[] = "18038083873";

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

#define BUTTON_1 "ButtonKey"

BlinkerButton Button1(BUTTON_1);

void button1_callback(const String & state)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG("get button state: ", state);

    if (state == BLINKER_CMD_BUTTON_TAP) {
        BLINKER_LOG("Button tap!");

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        Button1.print();
    }
    else if (state == BLINKER_CMD_BUTTON_PRESSED) {
        BLINKER_LOG("Button pressed!");

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        Button1.print();
    }
    else if (state == BLINKER_CMD_BUTTON_RELEASED) {
        BLINKER_LOG("Button released!");

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        // Button1.text("Your button name", "describe");
        Button1.print();
    }
    else if (state == BLINKER_CMD_ON) {
        BLINKER_LOG("Toggle on!");

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        // Button1.text("Your button name", "describe");
        Button1.print("on");
    }
    else if (state == BLINKER_CMD_OFF) {
        BLINKER_LOG("Toggle off!");

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        // Button1.text("Your button name", "describe");
        Button1.print("off");
    }
    else {
        BLINKER_LOG("Get user setting: ", state);

        Button1.icon("icon_1");
        Button1.color("#FFFFFF");
        Button1.text("Your button name or describe");
        // Button1.text("Your button name", "describe");
        Button1.print();
    }
}

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

    Blinker.begin(auth, ssid, pswd);

    Button1.attach(button1_callback);

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

        BLINKER_LOG("Blinker.wday(): ", Blinker.wday());
        BLINKER_LOG_FreeHeap();
        // Blinker.print("state", "online");
        // Blinker.checkState(false);
        // BLINKER_LOG_FreeHeap();
    }
}
