#define BLINKER_PRINT	Serial
#define BLINKER_WIFI

#define BUTTON_1		"ButtonKey"
#define TAP_EXAMPLE

#include <Blinker.h>

char ssid[] = "<Your WiFi network SSID or name>";
char pswd[] = "<Your WiFi network WPA password or WEP key>";

void setup()
{
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

        Blinker.vibrate();
        
        uint32_t BlinkerTime = millis();
        Blinker.print(BlinkerTime);
        Blinker.print("millis", BlinkerTime);
    }
    
#if defined(TAP_EXAMPLE)
    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    } // simple tap
#else
    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, LOW);
    }
    else {
        digitalWrite(LED_BUILTIN, HIGH);
    } //long press
#endif
}
