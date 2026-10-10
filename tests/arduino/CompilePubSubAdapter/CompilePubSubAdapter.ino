#include <BlinkerV2Advanced.h>
#include <PubSubMqtt.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_RENESAS)
#include <WiFiS3.h>
#else
#error "This compile test supports ESP32, ESP8266 and UNO R4 WiFi."
#endif

WiFiClient networkClient;
PubSubClient nativeMqtt(networkClient);

blinker::PubSubClientAdapter<0> mqttAdapter(
    nativeMqtt,
    blinker::MqttSecurity::PlainTcp,
    1152);

class CompileClock : public blinker::IClock
{
public:
    uint32_t monotonicMillis() const override
    {
        return millis();
    }

    bool unixTime(uint64_t&) const override
    {
        return false;
    }
};

CompileClock compileClock;
blinker::MqttFrameTransport mqttTransport(mqttAdapter, compileClock);

void setup()
{
    blinker::SessionCredentials credentials;
    credentials.broker = blinker::BrokerKind::Blinker;
    credentials.host = blinker::StringView("broker.example.com", 18);
    credentials.port = 1883;
    credentials.security = blinker::MqttSecurity::PlainTcp;
    credentials.deviceName = blinker::StringView("compile-device", 14);
    credentials.clientId = credentials.deviceName;
    credentials.username = blinker::StringView("compile-user", 12);
    credentials.password = blinker::StringView("compile-password", 16);
    credentials.publishTopic =
        blinker::StringView("/device/compile-device/s", 24);
    credentials.subscribeTopic =
        blinker::StringView("/device/compile-device/r", 24);

    mqttTransport.configure(credentials);
}

void loop()
{
    mqttTransport.poll(1000);
}
