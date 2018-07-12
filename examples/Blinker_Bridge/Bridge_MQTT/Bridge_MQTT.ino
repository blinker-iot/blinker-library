#define BLINKER_PRINT Serial
#define BLINKER_MQTT

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

char bridgeKey[] = "Your MQTT Secret Key of bridge to device";

#define BUTTON_1 "ButtonKey"

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
    Blinker.bridge(bridgeKey);
    Blinker.wInit(BUTTON_1, W_BUTTON);

    while (!Blinker.connected()) {
        Blinker.delay(10);
    }
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

    if (Blinker.bridgeAvailable(bridgeKey)) {
        BLINKER_LOG2("Blinker.bridgeRead(): ", Blinker.bridgeRead(bridgeKey));
    }

    if (Blinker.button(BUTTON_1)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

        uint32_t BlinkerTime = millis();

        Blinker.bridgeBeginFormat();
        Blinker.bridgePrint(bridgeKey, "Hello", "Blinker");
        Blinker.bridgePrint(bridgeKey, "millis", BlinkerTime);
        Blinker.bridgeEndFormat();

        Blinker.delay(60000);
    }
}