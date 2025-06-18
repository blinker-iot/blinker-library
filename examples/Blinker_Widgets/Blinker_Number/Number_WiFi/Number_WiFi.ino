#define BLINKER_WIFI
#define BLINKER_WIDGET

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define Number_1 "NUMKey"

BlinkerNumber Number1(Number_1);

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    Blinker.vibrate();
    
    uint32_t BlinkerTime = millis();
    
    Blinker.print("millis", BlinkerTime);

    Number1.icon("far fa-clock")
           .color("#00AA00")
           .unit("ms")
           .text("运行时间")
           .value(BlinkerTime)
           .print();

    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);
}

void loop()
{
    Blinker.run();
}
