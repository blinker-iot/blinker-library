#define BLINKER_PRINT	Serial
#define BLINKER_WIFI

#define KEY_1		"Key"

#include <Blinker.h>

char ssid[] = "<Your WiFi network SSID or name>";
char pswd[] = "<Your WiFi network WPA password or WEP key>";

void setup()
{
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);

	Blinker.begin(ssid, pswd);
	Blinker.wInit(KEY_1, W_KEY);
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

	if (Blinker.key(KEY_1)) {
		digitalWrite(LED_BUILTIN, HIGH);
	}
    else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}
