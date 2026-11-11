#include <BlinkerV2Advanced.h>
#include <PubSubMqtt.h>

#if defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure networkClient;
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
BearSSL::WiFiClientSecure networkClient;
#elif defined(ARDUINO_ARCH_RENESAS)
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
WiFiSSLClient networkClient;
#else
#error "This compile test supports ESP32, ESP8266 and UNO R4 WiFi."
#endif

PubSubClient nativeMqtt(networkClient);

blinker::PubSubClientAdapter<0> mqttAdapter(
    nativeMqtt,
    blinker::MqttSecurity::Tls,
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
    credentials.port = 8883;
    credentials.security = blinker::MqttSecurity::Tls;
    credentials.deviceName = blinker::StringView("compile-device", 14);
    credentials.clientId = credentials.deviceName;
    credentials.username = blinker::StringView("compile-user", 12);
    credentials.password = blinker::StringView("compile-password", 16);
    credentials.publishTopic =
        blinker::StringView("/device/compile-device/s", 24);
    credentials.subscribeTopic =
        blinker::StringView("/device/compile-device/r", 24);

    mqttTransport.configure(credentials);

    // Deliberately do not connect to the test domain.
    if (mqttAdapter.clientSecurity() != credentials.security)
    {
        networkClient.stop();
    }
}

void loop()
{
    mqttTransport.poll(1000);
}
