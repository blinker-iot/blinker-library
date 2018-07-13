#define BLINKER_PRINT Serial
#define BLINKER_WIFI

#include <Blinker.h>

char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define BUTTON_1 "ButtonKey"

bool freshed = false;

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(ssid, pswd);
    Blinker.wInit(BUTTON_1, W_BUTTON);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        uint32_t BlinkerTime = millis();

        Blinker.beginFormat();
        Blinker.vibrate();        
        Blinker.print("millis", BlinkerTime);
        Blinker.endFormat();
    }

    if (Blinker.builtInSwitch()) {
        if (!freshed) {
            BLINKER_LOG1("builtInSwitch on");
            freshed = true;
        }
    }
    else {
        if (freshed) {
            BLINKER_LOG1("builtInSwitch off");
            freshed = false;
        }
    }

    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}