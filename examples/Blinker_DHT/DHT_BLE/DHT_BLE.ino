#define BLINKER_PRINT Serial
#define BLINKER_BLE

#include <Blinker.h>

#include <modules/dht/DHT.h>

#define DHTPIN 4

//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    Blinker.begin();
    dht.begin();
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

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        BLINKER_LOG1("Failed to read from DHT sensor!");
        return;
    }

    float hic = dht.computeHeatIndex(t, h, false);

    BLINKER_LOG3("Humidity: ", h, " %");
    BLINKER_LOG3("Temperature: ", t, " *C");
    BLINKER_LOG3("Heat index: ", hic, " *C");

    Blinker.print("humi", h, " %");
    Blinker.print("temp", t, " *C");
    Blinker.print("hic", hic, " *C");

    Blinker.delay(2000);
}