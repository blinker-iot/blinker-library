#define BLINKER_PRINT	Serial
#define BLINKER_MQTT

#include <Blinker.h>

char auth[] = "<Your MQTT Secret Key>";
char ssid[] = "<Your WiFi network SSID or name>";
char pswd[] = "<Your WiFi network WPA password or WEP key>";

void setup() {
    Serial.begin(115200);

    Blinker.begin(auth, ssid, pswd);
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
}