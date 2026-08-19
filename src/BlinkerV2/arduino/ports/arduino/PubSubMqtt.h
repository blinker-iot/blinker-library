#ifndef BLINKER_V2_ARDUINO_PORTS_PUBSUB_MQTT_H
#define BLINKER_V2_ARDUINO_PORTS_PUBSUB_MQTT_H

#include <BlinkerV2/interface/IMqttClient.h>
#include <PubSubClient.h>

#include <limits.h>
#include <string.h>

namespace blinker {

template <
    uint8_t CallbackSlot = 0,
    size_t HostCapacity = 127,
    size_t ClientIdCapacity = 95,
    size_t UsernameCapacity = 127,
    size_t PasswordCapacity = 191,
    size_t TopicCapacity = 127>
class PubSubClientAdapter : public IMqttClient {
public:
    PubSubClientAdapter(
        PubSubClient& client,
        MqttSecurity clientSecurity,
        uint16_t packetBufferSize,
        uint16_t socketTimeoutSeconds = 2)
        : client_(client),
          clientSecurity_(clientSecurity),
          packetBufferSize_(packetBufferSize),
          socketTimeoutSeconds_(socketTimeoutSeconds),
          handler_(nullptr),
          handlerContext_(nullptr),
          ownsCallbackSlot_(false),
          bufferConfigured_(false) {
        host_[0] = '\0';
        clientId_[0] = '\0';
        username_[0] = '\0';
        password_[0] = '\0';
        topic_[0] = '\0';

        if (slotInstance() == nullptr) {
            slotInstance() = this;
            ownsCallbackSlot_ = true;
            client_.setCallback(&PubSubClientAdapter::callbackThunk);
        }
    }

    ~PubSubClientAdapter() override {
        if (ownsCallbackSlot_ && slotInstance() == this) {
            slotInstance() = nullptr;
        }
    }

    PubSubClientAdapter(const PubSubClientAdapter&) = delete;
    PubSubClientAdapter& operator=(const PubSubClientAdapter&) = delete;

    Result validatePacketCapacity(
        StringView topic,
        size_t payloadSize,
        uint8_t qos,
        MqttPacketDirection direction) const override {
        if (!valid(topic) || payloadSize == 0) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if ((direction == MqttPacketDirection::Outbound && qos != 0) ||
            (direction == MqttPacketDirection::Inbound && qos > 1)) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (topic.size > TopicCapacity ||
            !packetFits(topic.size, payloadSize, qos)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
        return Result::success();
    }

    Result connect(const MqttConnectOptions& options) override {
        if (!ownsCallbackSlot_) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        // The adapter owns PubSubClient's single message callback.
        client_.setCallback(&PubSubClientAdapter::callbackThunk);
        if (options.security != clientSecurity_) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (!options.cleanSession) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (options.port == 0 || socketTimeoutSeconds_ == 0 ||
            !valid(options.host) ||
            !valid(options.clientId) ||
            !valid(options.username) ||
            !valid(options.password)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (!copyText(options.host, host_, HostCapacity) ||
            !copyText(options.clientId, clientId_, ClientIdCapacity) ||
            !copyText(options.username, username_, UsernameCapacity) ||
            !copyText(options.password, password_, PasswordCapacity)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }

        if (!bufferConfigured_) {
            if (packetBufferSize_ < 32 ||
                !client_.setBufferSize(packetBufferSize_)) {
                return Result::failure(ErrorCode::BufferTooSmall);
            }
            client_.setSocketTimeout(socketTimeoutSeconds_);
            bufferConfigured_ = true;
        }

        client_.setServer(host_, options.port);
        client_.setKeepAlive(options.keepAliveSeconds);

        if (!client_.connect(clientId_, username_, password_)) {
            return Result::failure(mapState(client_.state()));
        }
        return Result::success();
    }

    void disconnect() override {
        client_.disconnect();
    }

    bool connected() const override {
        return client_.connected();
    }

    Result subscribe(StringView topic, uint8_t qos) override {
        if (qos > 1) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (!valid(topic)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (!copyText(topic, topic_, TopicCapacity)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
        if (!client_.subscribe(topic_, qos)) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        return Result::success();
    }

    Result publish(
        StringView topic,
        ByteView payload,
        const MqttPublishOptions& options) override {
        if (options.qos != 0) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (!valid(topic) ||
            payload.data == nullptr ||
            payload.size == 0 ||
            payload.size > UINT_MAX) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        if (!copyText(topic, topic_, TopicCapacity)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }

        if (!packetFits(topic.size, payload.size, options.qos)) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }

        if (!client_.publish(
                topic_,
                payload.data,
                static_cast<unsigned int>(payload.size),
                options.retained)) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        return Result::success();
    }

    void poll(uint32_t) override {
        client_.loop();
    }

    void setMessageHandler(
        MqttMessageHandler handler,
        void* context) override {
        handler_ = handler;
        handlerContext_ = context;
    }

    uint16_t packetBufferSize() const {
        return packetBufferSize_;
    }

    uint16_t socketTimeoutSeconds() const {
        return socketTimeoutSeconds_;
    }

    MqttSecurity clientSecurity() const {
        return clientSecurity_;
    }

private:
    static PubSubClientAdapter*& slotInstance() {
        static PubSubClientAdapter* instance = nullptr;
        return instance;
    }

    static void callbackThunk(
        char* topic,
        uint8_t* payload,
        unsigned int length) {
        PubSubClientAdapter* instance = slotInstance();
        if (instance != nullptr) {
            instance->onMessage(topic, payload, length);
        }
    }

    void onMessage(
        char* topic,
        uint8_t* payload,
        unsigned int length) {
        if (handler_ == nullptr || topic == nullptr) {
            return;
        }
        handler_(
            handlerContext_,
            StringView(topic, strlen(topic)),
            ByteView(payload, static_cast<size_t>(length)),
            MqttMessageInfo());
    }

    static bool copyText(
        StringView source,
        char* destination,
        size_t capacity) {
        if (destination == nullptr ||
            source.data == nullptr ||
            memchr(source.data, '\0', source.size) != nullptr ||
            source.size > capacity) {
            return false;
        }
        memcpy(destination, source.data, source.size);
        destination[source.size] = '\0';
        return true;
    }

    static bool valid(StringView value) {
        return value.data != nullptr &&
               value.size != 0 &&
               memchr(value.data, '\0', value.size) == nullptr;
    }

    bool packetFits(
        size_t topicSize,
        size_t payloadSize,
        uint8_t qos) const {
        // PubSubClient stores one complete MQTT packet. Five bytes is its
        // maximum fixed header, followed by a two-byte topic length and an
        // inbound packet id when QoS is non-zero.
        const size_t fixedOverhead = 5U + 2U + (qos == 0 ? 0U : 2U);
        if (packetBufferSize_ < fixedOverhead) {
            return false;
        }
        size_t remaining = packetBufferSize_ - fixedOverhead;
        if (topicSize > remaining) {
            return false;
        }
        remaining -= topicSize;
        return payloadSize <= remaining;
    }

    static ErrorCode mapState(int state) {
        switch (state) {
            case MQTT_CONNECT_BAD_CREDENTIALS:
            case MQTT_CONNECT_UNAUTHORIZED:
                return ErrorCode::AuthenticationRequired;
            case MQTT_CONNECT_BAD_PROTOCOL:
            case MQTT_CONNECT_BAD_CLIENT_ID:
                return ErrorCode::ProtocolError;
            case MQTT_CONNECTION_TIMEOUT:
            case MQTT_CONNECTION_LOST:
            case MQTT_CONNECT_FAILED:
            case MQTT_DISCONNECTED:
            case MQTT_CONNECT_UNAVAILABLE:
                return ErrorCode::NotConnected;
            default:
                return ErrorCode::InternalError;
        }
    }

    PubSubClient& client_;
    MqttSecurity clientSecurity_;
    uint16_t packetBufferSize_;
    uint16_t socketTimeoutSeconds_;
    MqttMessageHandler handler_;
    void* handlerContext_;
    bool ownsCallbackSlot_;
    bool bufferConfigured_;
    char host_[HostCapacity + 1];
    char clientId_[ClientIdCapacity + 1];
    char username_[UsernameCapacity + 1];
    char password_[PasswordCapacity + 1];
    char topic_[TopicCapacity + 1];
};

} // namespace blinker

#endif
