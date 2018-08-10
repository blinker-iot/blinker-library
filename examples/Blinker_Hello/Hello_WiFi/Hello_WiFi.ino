#define BLINKER_PRINT Serial
#define BLINKER_WIFI

#include <Blinker.h>

char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

BlinkerButton Button1("btn-abc");
BlinkerNumber Number1('num-abc');

int counter = 0;

void button1_callback(const String & state) {
	BLINKER_LOG2("get button state: ", state);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Blinker.begin(ssid, pswd);
    Button1.attach(button1_callback);
}

void loop() {
    Blinker.run();
    
    if (Blinker.available()) {
        BLINKER_LOG2("Blinker.readString(): ", Blinker.readString());
		counter++;
		Number1.print(counter);
    }
}