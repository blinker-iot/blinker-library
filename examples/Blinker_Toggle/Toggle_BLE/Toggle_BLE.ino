#define BLINKER_PRINT	Serial
#define BLINKER_BLE

#define TOGGLE_1		"ToggleKey"

#include <Blinker.h>

void setup()
{
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
	
	Blinker.begin();
	Blinker.wInit(TOGGLE_1, W_TOGGLE);
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

	if (Blinker.toggle(TOGGLE_1)) {
		digitalWrite(LED_BUILTIN, HIGH);
	}
    else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}