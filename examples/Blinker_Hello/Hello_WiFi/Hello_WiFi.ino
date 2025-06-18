#define BLINKER_WIFI
#define BLINKER_WIDGET

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

BlinkerButton Button1("btn-abc");
BlinkerNumber Number1("num-abc");

int counter = 0;

void button1_callback(const String & state)
{
    BLINKER_LOG("get button state: ", state);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    
    if (digitalRead(LED_BUILTIN)) {
        Button1.icon("far fa-lightbulb")
               .color("#FFAA00")
               .text("LED ON")
               .print();
    }
    else {
        Button1.icon("far fa-lightbulb")
               .color("#000000")
               .text("LED OFF")
               .print();
    }
}

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);
    counter++;
    
    Number1.icon("far fa-swords")
           .color("#0066FF")
           .unit("次")
           .text("计数器")
           .value(counter)
           .print();
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);

    Button1.attach(button1_callback);
}

void loop()
{
    Blinker.run();
}
