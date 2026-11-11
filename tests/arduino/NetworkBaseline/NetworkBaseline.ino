#if defined(ESP32)
#include <WiFi.h>
#elif defined(ARDUINO_ARCH_RENESAS)
#include <WiFiS3.h>
#else
#error "This compile test supports ESP32 and UNO R4 WiFi."
#endif

WiFiClient networkClient;

void setup()
{
    if (networkClient.connected())
    {
        networkClient.stop();
    }
}

void loop()
{
}
