#define BLINKER_PRINT	Serial
#define BLINKER_WIFI

#include <Blinker.h>

char ssid[] = "<Your WiFi network SSID or name>";
char pswd[] = "<Your WiFi network WPA password or WEP key>";

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(ssid, pswd);
    Blinker.attachAhrs();
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

    BLINKER_LOG2("AHRS Yaw: ", Blinker.ahrs(Yaw));
    BLINKER_LOG2("AHRS Roll: ", Blinker.ahrs(Roll));
    BLINKER_LOG2("AHRS Pitch: ", Blinker.ahrs(Pitch));

    Blinker.delay(2000);
}