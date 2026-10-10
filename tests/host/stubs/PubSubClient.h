#ifndef TEST_STUB_PUBSUBCLIENT_H
#define TEST_STUB_PUBSUBCLIENT_H

#include <stddef.h>
#include <stdint.h>

#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

class PubSubClient {
public:
    typedef void (*Callback)(char*, uint8_t*, unsigned int);

    PubSubClient()
        : callback(nullptr),
          connectedValue(false),
          setBufferResult(true),
          connectResult(true),
          subscribeResult(true),
          publishResult(true),
          stateValue(MQTT_DISCONNECTED),
          setBufferCalls(0),
          setSocketTimeoutCalls(0),
          connectCalls(0),
          disconnectCalls(0),
          subscribeCalls(0),
          publishCalls(0),
          loopCalls(0),
          bufferSize(0),
          socketTimeout(0),
          serverPort(0),
          keepAlive(0),
          publishedLength(0),
          publishedRetained(false),
          subscribeQos(0),
          serverHost(nullptr),
          clientId(nullptr),
          username(nullptr),
          password(nullptr),
          subscribeTopic(nullptr),
          publishTopic(nullptr) {}

    PubSubClient& setCallback(Callback value) {
        callback = value;
        return *this;
    }

    bool setBufferSize(uint16_t value) {
        ++setBufferCalls;
        bufferSize = value;
        return setBufferResult;
    }

    PubSubClient& setSocketTimeout(uint16_t value) {
        ++setSocketTimeoutCalls;
        socketTimeout = value;
        return *this;
    }

    PubSubClient& setServer(const char* host, uint16_t port) {
        serverHost = host;
        serverPort = port;
        return *this;
    }

    PubSubClient& setKeepAlive(uint16_t value) {
        keepAlive = value;
        return *this;
    }

    bool connect(
        const char* nextClientId,
        const char* nextUsername,
        const char* nextPassword) {
        ++connectCalls;
        clientId = nextClientId;
        username = nextUsername;
        password = nextPassword;
        connectedValue = connectResult;
        stateValue = connectResult ? MQTT_CONNECTED : stateValue;
        return connectResult;
    }

    void disconnect() {
        ++disconnectCalls;
        connectedValue = false;
        stateValue = MQTT_DISCONNECTED;
    }

    bool connected() const {
        return connectedValue;
    }

    bool subscribe(const char* topic, uint8_t qos) {
        ++subscribeCalls;
        subscribeTopic = topic;
        subscribeQos = qos;
        return subscribeResult;
    }

    bool publish(
        const char* topic,
        const uint8_t*,
        unsigned int length,
        bool retained) {
        ++publishCalls;
        publishTopic = topic;
        publishedLength = length;
        publishedRetained = retained;
        return publishResult;
    }

    bool loop() {
        ++loopCalls;
        return connectedValue;
    }

    int state() const {
        return stateValue;
    }

    void inject(char* topic, uint8_t* payload, unsigned int length) {
        if (callback != nullptr) {
            callback(topic, payload, length);
        }
    }

    Callback callback;
    bool connectedValue;
    bool setBufferResult;
    bool connectResult;
    bool subscribeResult;
    bool publishResult;
    int stateValue;
    uint32_t setBufferCalls;
    uint32_t setSocketTimeoutCalls;
    uint32_t connectCalls;
    uint32_t disconnectCalls;
    uint32_t subscribeCalls;
    uint32_t publishCalls;
    uint32_t loopCalls;
    uint16_t bufferSize;
    uint16_t socketTimeout;
    uint16_t serverPort;
    uint16_t keepAlive;
    unsigned int publishedLength;
    bool publishedRetained;
    uint8_t subscribeQos;
    const char* serverHost;
    const char* clientId;
    const char* username;
    const char* password;
    const char* subscribeTopic;
    const char* publishTopic;
};

#endif
