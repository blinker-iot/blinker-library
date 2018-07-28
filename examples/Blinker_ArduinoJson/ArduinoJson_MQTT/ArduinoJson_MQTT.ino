#define BLINKER_PRINT Serial
#define BLINKER_MQTT

#include <Blinker.h>

char auth[] = "Your MQTT Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

void setup()
{
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Blinker.begin(auth, ssid, pswd);
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


        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        String get_weather = Blinker.weather();

        BLINKER_LOG2("weather: ", get_weather);

        DynamicJsonBuffer jsonBuffer;
        JsonObject& weather = jsonBuffer.parseObject(get_weather);

        if (!weather.success()) {
            BLINKER_LOG2(get_weather, " , not a Json buffer!");
        }

        String weather_text = weather["cond_txt"];
        int8_t weather_temp = weather["tmp"];

        BLINKER_LOG5("Local weather is: ", weather_text, " ,temperature is: ", weather_temp, "℃");

        Blinker.delay(60000);
    }
}