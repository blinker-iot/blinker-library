#define BLINKER_PRINT	Serial
#define BLINKER_BLE

#include <Blinker.h>

#include <modules/Ultrasonic.h>

Ultrasonic SR04(9, 8);// Trig, Echo

void setup()
{
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
	
	Blinker.begin();
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

    BLINKER_LOG3("SR04: ", SR04.cm(), " cm");

    Blinker.print("SR04", SR04.cm());

    Blinker.delay(1000);
}