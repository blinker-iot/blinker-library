#define BLINKER_PRINT	Serial
#define BLINKER_WIFI

#include <Blinker.h>

void setup()
{
	Serial.begin(115200);

	Blinker.begin();
}

void loop()
{
	Blinker.run();
}