#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure networkClient;
#elif defined(ARDUINO_ARCH_RENESAS)
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
WiFiSSLClient networkClient;
#else
#error "This compile test supports ESP32 and UNO R4 WiFi."
#endif

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
