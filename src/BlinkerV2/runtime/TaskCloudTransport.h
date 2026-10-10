#ifndef BLINKER_RUNTIME_TASKCLOUDTRANSPORT_H
#define BLINKER_RUNTIME_TASKCLOUDTRANSPORT_H

#include "../provisioning/TaskCommandAdapter.h"
#include "../transport/ManagedMqttTransport.h"
#include <string.h>

namespace blinker {

// Optional physical WiFi management carrier on the SAME MQTT client/topic.
// Service is the sole cloud management peer; never unwrap App Route/Delivery.
// Ownership remains enforced by the signed Session grant, not MQTT auth.
// The caller owns one 212-byte workspace (exclusive while this object lives).
// No second Session, queue, key store, task thread, or native SDK dependency.
template <typename Cloud = ManagedMqttTransport>
class TaskCloudTransport final : public IFrameTransport {
public:
    TaskCloudTransport(Cloud& cloud, MqttFrameTransport& mqtt, IClock& clock,
        TaskCommandAdapter& adapter, MutableByteSpan workspace)
        : cloud_(cloud), mqtt_(mqtt), clock_(clock), adapter_(adapter), workspace_(workspace),
          receiver_(nullptr), receiverContext_(nullptr), connected_(nullptr), disconnected_(nullptr), sessionContext_(nullptr),
          source_(), size_(0U), receivedAt_(0U), rootGeneration_(0U), sequence_(0U), stage_(Empty),
          started_(false), negotiated_(false), polling_(false), processing_(false), error_(ErrorCode::Ok) {
        cloud_.setReceiver(&receiveThunk, this);
        cloud_.setSessionHandlers(&connectedThunk, &disconnectedThunk, this);
    }
    ~TaskCloudTransport() override {
        stop(); cloud_.setReceiver(nullptr, nullptr); cloud_.setSessionHandlers(nullptr, nullptr, nullptr);
    }
    Result start() override {
        if (started_) return Result::success();
        if (workspace_.data == nullptr || workspace_.size < kTaskCarrierFrameMaxSize)
            return Result::failure(ErrorCode::BufferTooSmall);
        const uint16_t flags = cloud_.capabilities().features;
        if ((flags & (TransportFeatureCloud | TransportFeatureAuthenticated | TransportFeatureLocal)) !=
            (TransportFeatureCloud | TransportFeatureAuthenticated)) return Result::failure(ErrorCode::InvalidArgument);
        Result result = cloud_.start();
        if (result) { started_ = true; error_ = ErrorCode::Ok; }
        return result;
    }
    void stop() override { started_ = false; resetConnection(); cloud_.stop(); }
    void setNetworkAvailable(bool available) {
        if (!available) resetConnection();
        cloud_.setNetworkAvailable(available);
    }
    bool networkAvailable() const { return cloud_.networkAvailable(); }
    TransportState state() const override { return cloud_.state(); }
    ErrorCode lastError() const { return cloud_.lastError(); }
    ErrorCode managementError() const { return error_; }
    TransportCapabilities capabilities() const override { return cloud_.capabilities(); }
    uint32_t protocolFeatures() const override {
        return workspace_.data != nullptr && workspace_.size >= kTaskCarrierFrameMaxSize
            ? static_cast<uint32_t>(bbp2::FeatureTaskManagement) : 0U;
    }
    // Called by Runtime only after accepting Hello, never by native RX parsing.
    void protocolNegotiated(const RxContext& rx, uint32_t features, uint16_t maximum) override {
        resetConnection();
        if (!started_ || state() != TransportState::Online || !rx.authenticated || rx.sessionId != 1U ||
            (features & bbp2::FeatureTaskManagement) == 0U || maximum < kTaskCarrierFrameMaxSize) return;
        source_.transport = rx.transportId; source_.session = rx.sessionId;
        source_.generation = mqtt_.connectionGeneration();
        negotiated_ = source_.generation != 0U;
    }
    Result send(ByteView frame, const SendTarget& target) override { return cloud_.send(frame, target); }
    void setReceiver(FrameReceiver receiver, void* context) override { receiver_ = receiver; receiverContext_ = context; }
    void setSessionHandlers(FrameSessionHandler connected, FrameSessionHandler disconnected, void* context) override {
        connected_ = connected; disconnected_ = disconnected; sessionContext_ = context;
    }
    void poll(uint32_t budgetMicros) override {
        if (!started_ || polling_) return;
        PollGuard guard(*this);
        adapter_.poll();
        cloud_.poll(budgetMicros);
        adapter_.poll();
        if (!current()) { resetConnection(); return; }
        if (stage_ == Empty) return;
        if (rootGeneration_ != adapter_.rootGeneration() ||
            static_cast<uint32_t>(clock_.monotonicMillis() - receivedAt_) >= 1000U) {
            adapter_.disconnect(source_); clear(); error_ = ErrorCode::StateConflict; return;
        }
        if (stage_ == Request) {
            Guard process(processing_);
            TaskWireOperation operation; ByteView inner;
            Result result = decodeTaskCarrier(ByteView(workspace_.data + bbp2::kBaseHeaderSize, size_), operation, inner);
            ByteView response;
            if (result) result = adapter_.receive(source_, static_cast<TaskCommand>(operation), inner,
                MutableByteSpan(workspace_.data + 15U, kTaskManagementResultMaxSize), response);
            // Root/connection/stop events during storage or signer callbacks
            // suppress this response, even if the durable mutation succeeded.
            if (!current() || stage_ == Empty || rootGeneration_ != adapter_.rootGeneration()) {
                clear(); return;
            }
            error_ = result.code();
            uint8_t failure[3] = {0x82U, static_cast<uint8_t>(operation), static_cast<uint8_t>(wireError(result.code()))};
            TaskWireOperation reply = operation == TaskWireOperation::Commit ? TaskWireOperation::Result : TaskWireOperation::Challenge;
            if (!result) { reply = TaskWireOperation::Failure; response = ByteView(failure, sizeof(failure)); }
            ByteView encoded;
            result = encodeTaskCarrier(reply, response, MutableByteSpan(workspace_.data + 10U, workspace_.size - 10U), encoded);
            bbp2::FrameHeader header; header.kind = static_cast<uint8_t>(bbp2::MessageKind::TaskManagement);
            header.flags = bbp2::FlagIsResponse; header.sequence = sequence_;
            header.bodyLength = static_cast<uint16_t>(encoded.size);
            if (result) result = bbp2::encodeHeader(header, workspace_);
            if (!result) { error_ = result.code(); clear(); return; }
            size_ = static_cast<uint16_t>(10U + encoded.size); stage_ = Reply;
        }
        if (static_cast<uint32_t>(clock_.monotonicMillis() - receivedAt_) >= 1000U) {
            adapter_.disconnect(source_); clear(); error_ = ErrorCode::StateConflict; return;
        }
        // One retryable bounded reply, no repeated Engine commit or RX queue.
        SendTarget target; target.kind = SendTargetKind::Session;
        target.transportId = source_.transport; target.sessionId = source_.session;
        const Result sent = cloud_.send(ByteView(workspace_.data, size_), target);
        if (!sent) error_ = sent.code();
        if (sent || sent.code() != ErrorCode::WouldBlock) clear();
    }
private:
    enum Stage : uint8_t { Empty, Request, Reply };
    struct Guard { bool& value; explicit Guard(bool& v) : value(v) { value = true; } ~Guard() { value = false; } };
    struct PollGuard {
        TaskCloudTransport& self;
        explicit PollGuard(TaskCloudTransport& value) : self(value) { self.polling_ = true; }
        ~PollGuard() { self.polling_ = false; if (self.stage_ == Empty) self.clear(); }
    };
    bool current() const {
        return started_ && negotiated_ && state() == TransportState::Online &&
            source_.generation == mqtt_.connectionGeneration();
    }
    void clear() {
        stage_ = Empty;
        // Retain size only while a callback is unwinding. Idle polls need not
        // repeatedly erase an already empty workspace.
        if (!processing_) {
            if (size_ != 0U && workspace_.data != nullptr) secureZero(workspace_);
            size_ = 0U;
        }
    }
    void resetConnection() {
        adapter_.disconnect(source_); negotiated_ = false; clear();
    }
    static TaskWireFailure wireError(ErrorCode code) {
        switch (code) {
        case ErrorCode::AuthenticationRequired: return TaskWireFailure::Unauthorized;
        case ErrorCode::WouldBlock: return TaskWireFailure::Busy;
        case ErrorCode::StateConflict: case ErrorCode::SequenceConflict: case ErrorCode::AlreadyExists:
        case ErrorCode::NotFound: return TaskWireFailure::Conflict;
        case ErrorCode::CapacityExceeded: case ErrorCode::BufferTooSmall: return TaskWireFailure::Capacity;
        case ErrorCode::NotConfigured: case ErrorCode::InternalError: case ErrorCode::NotConnected: return TaskWireFailure::Unavailable;
        default: return TaskWireFailure::Malformed;
        }
    }
    static void receiveThunk(void* context, ByteView frame, const RxContext& rx) {
        static_cast<TaskCloudTransport*>(context)->receive(frame, rx);
    }
    static void connectedThunk(void* context, const RxContext& rx) {
        TaskCloudTransport& self = *static_cast<TaskCloudTransport*>(context);
        self.resetConnection();
        if (self.connected_ != nullptr) self.connected_(self.sessionContext_, rx);
    }
    static void disconnectedThunk(void* context, const RxContext& rx) {
        TaskCloudTransport& self = *static_cast<TaskCloudTransport*>(context);
        self.resetConnection();
        if (self.disconnected_ != nullptr) self.disconnected_(self.sessionContext_, rx);
    }
    void receive(ByteView encoded, const RxContext& rx) {
        if (encoded.data == nullptr || encoded.size < bbp2::kBaseHeaderSize) return;
        if (encoded.data[3] != static_cast<uint8_t>(bbp2::MessageKind::TaskManagement)) {
            if (encoded.data[3] == static_cast<uint8_t>(bbp2::MessageKind::Hello)) resetConnection();
            if (receiver_ != nullptr) receiver_(receiverContext_, encoded, rx);
            return;
        }
        // Borrowed native buffer: no Flash, signing, sends or nested poll here.
        if (!current() || !rx.authenticated || rx.sessionId != source_.session || stage_ != Empty || processing_) return;
        bbp2::FrameView frame; TaskWireOperation operation; ByteView body;
        if (!bbp2::parseFrame(encoded, frame) || frame.header.flags != 0U || frame.header.sequence == 0U ||
            frame.header.headerLength != bbp2::kBaseHeaderSize ||
            !decodeTaskCarrier(frame.body, operation, body) || !taskWireRequest(operation)) return;
        memcpy(workspace_.data + 10U, frame.body.data, frame.body.size);
        size_ = static_cast<uint16_t>(frame.body.size); sequence_ = frame.header.sequence;
        receivedAt_ = clock_.monotonicMillis(); rootGeneration_ = adapter_.rootGeneration(); stage_ = Request;
    }
    Cloud& cloud_; MqttFrameTransport& mqtt_; IClock& clock_; TaskCommandAdapter& adapter_;
    MutableByteSpan workspace_;
    FrameReceiver receiver_; void* receiverContext_;
    FrameSessionHandler connected_; FrameSessionHandler disconnected_; void* sessionContext_;
    TaskCommandSource source_;
    uint16_t size_; uint32_t receivedAt_, rootGeneration_; uint16_t sequence_; Stage stage_;
    bool started_, negotiated_, polling_, processing_; ErrorCode error_;
    TaskCloudTransport(const TaskCloudTransport&);
    TaskCloudTransport& operator=(const TaskCloudTransport&);
};
} // namespace blinker
#endif
