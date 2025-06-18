#define BLINKER_WIFI
#define BLINKER_WIDGET

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

void switch_callback(const String & state)
{
    Serial.print("Switch state: ");
    Serial.println(state);
    
    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, HIGH);
        BLINKER_BUILTIN_SWITCH.state("on").print();
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, LOW);
        BLINKER_BUILTIN_SWITCH.state("off").print();
    }
}

void dataRead(const String & data)
{
    Serial.print("Blinker readString: ");
    Serial.println(data);
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);
    BLINKER_BUILTIN_SWITCH.attach(switch_callback);

    Serial.println("BlinkerSwitch Singleton Example Started");
    Serial.println("Use Blinker app to control the built-in switch");
}

void loop()
{
    Blinker.run();
}
