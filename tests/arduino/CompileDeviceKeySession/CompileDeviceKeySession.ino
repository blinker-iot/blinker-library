#include <BlinkerV2Advanced.h>
#include <ArduinoClientHttp.h>
#include <HardwareRandom.h>
#include <PubSubMqtt.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFiClientSecure.h>
WiFiClientSecure httpNetwork;
WiFiClientSecure mqttNetwork;
#elif defined(ARDUINO_ARCH_RENESAS_UNO)
#include <WiFiS3.h>
WiFiSSLClient httpNetwork;
WiFiSSLClient mqttNetwork;
#else
#error "CompileDeviceKeySession supports ESP32 and UNO R4 WiFi"
#endif

class CompileClock : public blinker::IClock
{
public:
    uint32_t monotonicMillis() const override { return millis(); }
    bool unixTime(uint64_t&) const override { return false; }
};

PubSubClient nativeMqtt(mqttNetwork);
blinker::PubSubClientAdapter<0> mqtt(
    nativeMqtt,
    blinker::MqttSecurity::Tls,
    1152U);
CompileClock clockSource;
blinker::ArduinoClientHttpAdapter<decltype(httpNetwork)> http(
    httpNetwork,
    blinker::HttpSecurity::Tls,
    clockSource);
blinker::ConfiguredDeviceKeySource deviceKey;
blinker::DeviceInstanceId deviceInstance;
blinker::PlatformHardwareRandom hardwareRandom;
blinker::DeviceKeySessionArena cloudArena;
blinker::HttpDeviceKeySessionConfig cloudConfig;
blinker::MqttFrameTransport mqttTransport(mqtt, clockSource);

void setup()
{
    (void)deviceKey.configure(blinker::StringView(
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8"));
    for (size_t index = 0U;
         index < sizeof(deviceInstance.bytes);
         ++index)
    {
        deviceInstance.bytes[index] = static_cast<uint8_t>(index + 1U);
    }
    cloudConfig.host = blinker::StringView("gateway.example.com");
    cloudConfig.port = 443U;
    cloudConfig.firmwareVersion = blinker::StringView("2.0.0-compile");
    cloudConfig.httpSecurity = blinker::HttpSecurity::Tls;
    cloudConfig.mqttSecurity = blinker::MqttSecurity::Tls;
    static blinker::HttpDeviceKeySessionProvider provider(
        deviceKey,
        deviceInstance,
        hardwareRandom,
        http,
        clockSource,
        cloudConfig,
        cloudArena.requestBuffer(),
        cloudArena.responseBuffer(),
        cloudArena.credentialArena());
    static blinker::ManagedMqttTransport transport(provider, mqttTransport);

    // Compile/link the complete v2 cloud chain without contacting test hosts.
    transport.setNetworkAvailable(false);
    transport.start();
    transport.poll(1000U);
    transport.stop();
}

void loop() {}
