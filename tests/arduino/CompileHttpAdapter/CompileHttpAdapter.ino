#include <BlinkerV2Advanced.h>
#include <ArduinoClientHttp.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFiClientSecure.h>
WiFiClientSecure networkClient;
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
BearSSL::WiFiClientSecure networkClient;
#elif defined(ARDUINO_ARCH_RENESAS_UNO)
#include <WiFiS3.h>
WiFiSSLClient networkClient;
#else
#error Unsupported compile-test architecture
#endif

class CompileClock : public blinker::IClock
{
public:
    uint32_t monotonicMillis() const override { return millis(); }
    bool unixTime(uint64_t&) const override { return false; }
};

CompileClock clockSource;
blinker::ArduinoClientHttpAdapter<decltype(networkClient)> http(
    networkClient,
    blinker::HttpSecurity::Tls,
    clockSource);
uint8_t responseBuffer[256];

void setup()
{
    blinker::HttpRequest request;
    request.host = blinker::StringView("iot.diandeng.tech", 17);
    request.port = 443;
    request.security = blinker::HttpSecurity::Tls;
    request.target = blinker::StringView("/health", 7);
    request.responseBuffer = blinker::MutableByteSpan(
        responseBuffer,
        sizeof(responseBuffer));
    http.start(request);
    http.poll(100);
    http.stop();
}

void loop()
{
}
