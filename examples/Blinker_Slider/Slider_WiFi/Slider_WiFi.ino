#define BLINKER_PRINT Serial
#define BLINKER_WIFI

#include <Blinker.h>

char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

#define Slider_1 "SliderKey"

BlinkerSlider Slider1(Slider_1);

void slider1_callback(int32_t value)
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG2("get slider value: ", value);
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(ssid, pswd);

    Slider1.attach(slider1_callback);
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

        Slider1.color("#FFFFFF");
        Slider1.print(random(0, 128));
    }
}
