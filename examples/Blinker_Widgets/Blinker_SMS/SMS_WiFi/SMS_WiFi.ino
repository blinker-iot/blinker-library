#define BLINKER_WIFI
#define BLINKER_WIDGET

#include <Blinker.h>

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);

    uint32_t BlinkerTime = millis();

    Blinker.vibrate();        
    Blinker.print("millis", BlinkerTime);

    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Blinker.sms("Hello blinker!");

    Blinker.delay(60000);
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