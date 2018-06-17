#define BLINKER_PRINT Serial
#define BLINKER_WIFI

#include <Blinker.h>

char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define BUTTON_1 "ButtonKey"

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(ssid, pswd);
    Blinker.setTimezone(8.0);
    Blinker.wInit(BUTTON_1, W_BUTTON);
}

void loop()
{
    Blinker.run();

    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);
    }
    
    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        BLINKER_LOG2("Now second: ", Blinker.second());
        BLINKER_LOG2("Now minute: ", Blinker.minute());
        BLINKER_LOG2("Now hour: ", Blinker.hour());
        BLINKER_LOG2("Now wday: ", Blinker.wday());
        BLINKER_LOG2("Now month: ", Blinker.month());
        BLINKER_LOG2("Now mday: ", Blinker.mday());
        BLINKER_LOG2("Now year: ", Blinker.year());
        BLINKER_LOG2("Now yday: ", Blinker.yday());
        BLINKER_LOG2("Now ntp time: ", Blinker.time());
    } // simple tap
}
