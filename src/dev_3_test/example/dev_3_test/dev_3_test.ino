// #define BLINKER_ESP_SMARTCONFIG
// #define BLINKER_ALIGENIE_OUTLET
#define BLINKER_MQTT
#define BLINKER_ALIGENIE_OUTLET

#include "Blinker4.h"

#define BLINKER_PRINT Serial

char auth[] = "bc5a991c7ec4";
char ssid[] = "mostfun";
char pswd[] = "18038083873";
bool oState = false;

void aligeniePowerState(const String & state)
{
    BLINKER_LOG("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerAliGenie.powerState("on");
        BlinkerAliGenie.print();

        oState = true;
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerAliGenie.powerState("off");
        BlinkerAliGenie.print();

        oState = false;
    }
}

void aligenieQuery(int32_t queryCode)
{
    BLINKER_LOG("AliGenie Query codes: ", queryCode);

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_ALL_NUMBER :
            BLINKER_LOG("AliGenie Query All");
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.print();
            break;
        case BLINKER_CMD_QUERY_POWERSTATE_NUMBER :
            BLINKER_LOG("AliGenie Query Power State");
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.print();
            break;
        default :
            BlinkerAliGenie.powerState(oState ? "on" : "off");
            BlinkerAliGenie.print();
            break;
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

    // strcpy(test, data.c_str());
    // charTest(test);
    // BLINKER_LOG("char test: ", test);

    // Blinker.hello();

    // BLINKER_LOG("hello", F("world"), 123, 456.78, -1);

    BLINKER_LOG_FreeHeap();

    BlinkerAliGenie.attachPowerState(aligeniePowerState);
    BlinkerAliGenie.attachQuery(aligenieQuery);
}

void loop()
{
    Blinker.run();

    if (Blinker.available())
    {
        BLINKER_LOG_FreeHeap();
        BLINKER_LOG("Blinker.readString(): ", Blinker.readString());

        BLINKER_LOG("Blinker.wday(): ", Blinker.wday());

        // BLINKER_LOG("Blinker.aqi(): ", Blinker.aqi());
        BLINKER_LOG_FreeHeap();
        // Blinker.print("state", "online");
        // Blinker.checkState(false);
        // BLINKER_LOG_FreeHeap();
    }
}
