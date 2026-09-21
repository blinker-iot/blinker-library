#ifndef BLINKER_RUNTIME_LOCALACCESSCLOUDTRANSPORT_H
#define BLINKER_RUNTIME_LOCALACCESSCLOUDTRANSPORT_H

#include "LocalAccessControl.h"
#include "../control/HttpDeviceKeySessionProvider.h"
#include "../transport/ManagedMqttTransport.h"

namespace blinker {

// Optional product composition around the EXISTING cloud transport. No second
// MQTT callback/client/topic, SDK, child directory or component state. A stopped
// product owns its boot reset; a cloud reconnect only resets the inbox/probe.
template <size_t Capacity, typename Cloud = ManagedMqttTransport,
          typename Provider = HttpDeviceKeySessionProvider,
          typename Mqtt = MqttFrameTransport,
          local_access::SecurityProfile Profile = local_access::SecurityProfile::NoiseNnPsk0AesGcmSha256>
class LocalAccessCloudTransport final : public IFrameTransport {
public:
    static constexpr local_access::SecurityProfile securityProfile = Profile;
    LocalAccessCloudTransport(Cloud& cloud, Mqtt& mqtt, Provider& provider,
        const DeviceInstanceId& instance, IDeviceKeySource& keys,
        IRandom& random, IClock& clock, ILocalAccessTargetPolicy& targets)
        : cloud_(cloud), mqtt_(mqtt), provider_(provider), instance_(instance), clock_(clock), random_(random),
          authority_(keys, targets), control_(authority_, keys, random), receiver_(nullptr),
          receiverContext_(nullptr), inbox_(), inboxSize_(0U), inboxGeneration_(0U),
          inboxConnection_(0U), keyVersion_(0U), generation_(0U), connection_(0U),
          nonce_(), probeAt_(0U), sentAt_(0U), progressAt_(0U), refreshAt_(0U), confirmedSequence_(0U),
          confirmedRevision_(0U), attempts_(0U), started_(false), refreshAttempted_(false),
          probePending_(false), refreshRequested_(false), error_(ErrorCode::Ok) {
        cloud_.setReceiver(&receiveThunk, this);
    }
    ~LocalAccessCloudTransport() override {
        stop();
        cloud_.setReceiver(nullptr, nullptr);
    }

    Result start() override {
        if (started_) return Result::success();
        Result result = cloud_.start();
        if (result) { started_ = true; error_ = ErrorCode::Ok; }
        return result;
    }
    void stop() override {
        if (!started_) return;
        cloud_.stop();
        control_.clear();
        clearInbox();
        keyVersion_ = generation_ = connection_ = 0U;
        resetProbe(clock_.monotonicMillis());
        confirmedSequence_ = confirmedRevision_ = 0U;
        refreshRequested_ = false;
        started_ = refreshAttempted_ = false;
        error_ = ErrorCode::Ok;
    }
    void setNetworkAvailable(bool available) {
        cloud_.setNetworkAvailable(available);
        if (!available) { clearInbox(); resetProbe(clock_.monotonicMillis()); }
    }
    bool networkAvailable() const { return cloud_.networkAvailable(); }
    TransportState state() const override { return cloud_.state(); }
    ErrorCode lastError() const { return cloud_.lastError(); }
    ErrorCode controlError() const { return error_; }
    LocalAccessAuthority<Capacity>& authority() { return authority_; }
    TransportCapabilities capabilities() const override { return cloud_.capabilities(); }
    Result send(ByteView frame, const SendTarget& target) override { return cloud_.send(frame, target); }
    void setReceiver(FrameReceiver receiver, void* context) override {
        receiver_ = receiver; receiverContext_ = context;
    }
    void setSessionHandlers(FrameSessionHandler connected, FrameSessionHandler disconnected,
                            void* context) override {
        cloud_.setSessionHandlers(connected, disconnected, context);
    }

    void poll(uint32_t budgetMicros) override {
        if (!started_) return;
        authority_.poll(clock_.monotonicMillis());
        cloud_.poll(budgetMicros);
        const uint32_t now = clock_.monotonicMillis();
        authority_.poll(now);
        const uint32_t version = provider_.authenticatedCredentialVersion();
        if (version == 0U) {
            // Failure/stop retires identity; transient refresh retains the old
            // provider metadata and does not arrive here.
            if (keyVersion_ != 0U) control_.clear();
            clearInbox(); keyVersion_ = generation_ = connection_ = 0U;
            resetProbe(now); confirmedSequence_ = confirmedRevision_ = 0U; refreshRequested_ = false;
            return;
        }
        if (keyVersion_ != version) {
            control_.clear(); clearInbox();
            const Result begun = control_.begin(ByteView(instance_.bytes, sizeof(instance_.bytes)), version);
            if (!begun) { error_ = begun.code(); return; }
            keyVersion_ = version; generation_ = connection_ = 0U;
            confirmedSequence_ = confirmedRevision_ = 0U;
        }
        if (cloud_.state() != TransportState::Online) { clearInbox(); resetProbe(now); return; }
        if (generation_ != provider_.credentialGeneration() || connection_ != mqtt_.connectionGeneration()) {
            if (generation_ != provider_.credentialGeneration()) { progressAt_ = now; refreshRequested_ = false; }
            generation_ = provider_.credentialGeneration(); connection_ = mqtt_.connectionGeneration();
            clearInbox(); resetProbe(now);
        }
        // An observed expired request cannot become fresh after millis wraps.
        if (probePending_ && static_cast<uint32_t>(now - probeAt_) >= 10000U) {
            probePending_ = false; secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
        }
        uint8_t output[bbp2::kBaseHeaderSize + local_access::kControlMaximumEncodedSize];
        MutableByteSpan body(output + bbp2::kBaseHeaderSize, local_access::kControlMaximumEncodedSize);
        size_t written = 0U;
        if (inboxSize_ != 0U) {
            Result result = Result::failure(ErrorCode::StateConflict);
            const bool status = inboxSize_ > 2U && inbox_[2] == static_cast<uint8_t>(local_access::ProgressKind::Status);
            if (inboxGeneration_ == generation_ && inboxConnection_ == connection_) {
                if (status && probePending_) {
                    local_access::ProgressView value;
                    result = control_.progress(ByteView(inbox_, inboxSize_), ByteView(nonce_, sizeof(nonce_)), provider_.proofSessionId(), value);
                    if (result) {
                        if (value.state == local_access::ProgressState::Synchronized || value.sequence > confirmedSequence_ ||
                            value.authorityRevision > confirmedRevision_) progressAt_ = now;
                        confirmedSequence_ = value.sequence; confirmedRevision_ = value.authorityRevision;
                        refreshRequested_ = value.state == local_access::ProgressState::Refresh;
                        probePending_ = false; secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
                    }
                } else if (!status) result = control_.accept(ByteView(inbox_, inboxSize_), body, written);
            }
            clearInbox();
            // An applied command is not confirmation that Service got the ACK.
            if (result && !status) result = publish(output, written);
            error_ = result.code();
            secureZero(MutableByteSpan(output, sizeof(output)));
            return;
        }
        if (provider_.state() == SessionProviderState::Ready &&
            (refreshRequested_ || static_cast<uint32_t>(now - progressAt_) >= 240000U) &&
            (!refreshAttempted_ || static_cast<uint32_t>(now - refreshAt_) >= 300000U)) {
            // Existing login refresh, not revokeSession or a global authority
            // reset. Silence AND signed exhausted status can recover after sync.
            refreshAttempted_ = true; refreshAt_ = now;
            error_ = provider_.requestRefresh().code();
        } else if (provider_.state() == SessionProviderState::Ready) {
            if (static_cast<uint32_t>(now - probeAt_) >= 60000U) resetProbe(now);
            if (attempts_ == 0U) {
                Result result = random_.fill(MutableByteSpan(nonce_, sizeof(nonce_)));
                if (!result) { attempts_ = kProbeAttempts; error_ = result.code(); }
                else probePending_ = true;
            }
            if (probePending_ && attempts_ < kProbeAttempts && (attempts_ == 0U ||
                static_cast<uint32_t>(now - sentAt_) >= (1000UL << (attempts_ - 1U)))) {
                Result result = control_.query(ByteView(nonce_, sizeof(nonce_)), provider_.proofSessionId(), body, written);
                if (result) result = publish(output, written);
                error_ = result.code(); sentAt_ = now; ++attempts_;
            }
        }
        secureZero(MutableByteSpan(output, sizeof(output)));
    }

private:
    enum : uint8_t { kProbeAttempts = 4U };
    void resetProbe(uint32_t now) {
        secureZero(MutableByteSpan(nonce_, sizeof(nonce_)));
        attempts_ = 0U; probePending_ = false; probeAt_ = now;
    }
    static void receiveThunk(void* context, ByteView frame, const RxContext& rx) {
        static_cast<LocalAccessCloudTransport*>(context)->receive(frame, rx);
    }
    void receive(ByteView encoded, const RxContext& rx) {
        // All callbacks run inside the existing cooperative MQTT poll. Never
        // load Key, apply authority or publish while borrowing its RX buffer.
        bbp2::FrameView frame;
        if (encoded.size < bbp2::kBaseHeaderSize || encoded.data == nullptr) return;
        if (encoded.data[3] != static_cast<uint8_t>(bbp2::MessageKind::LocalAccessControl)) {
            if (receiver_ != nullptr) receiver_(receiverContext_, encoded, rx);
            return;
        }
        if (!started_ || !rx.authenticated || inboxSize_ != 0U ||
            !bbp2::parseFrame(encoded, frame) || frame.header.flags != 0U ||
            frame.header.sequence != 0U || frame.header.headerLength != bbp2::kBaseHeaderSize ||
            frame.body.empty() || frame.body.size > sizeof(inbox_)) return;
        memcpy(inbox_, frame.body.data, frame.body.size);
        inboxSize_ = static_cast<uint8_t>(frame.body.size);
        inboxGeneration_ = provider_.credentialGeneration();
        inboxConnection_ = mqtt_.connectionGeneration();
    }
    Result publish(uint8_t* output, size_t size) {
        bbp2::FrameHeader header;
        header.kind = static_cast<uint8_t>(bbp2::MessageKind::LocalAccessControl);
        header.bodyLength = static_cast<uint16_t>(size);
        Result result = bbp2::encodeHeader(header, MutableByteSpan(output, bbp2::kBaseHeaderSize));
        if (result) result = cloud_.send(ByteView(output, bbp2::kBaseHeaderSize + size), SendTarget());
        return result;
    }
    void clearInbox() {
        secureZero(MutableByteSpan(inbox_, sizeof(inbox_)));
        inboxSize_ = 0U; inboxGeneration_ = inboxConnection_ = 0U;
    }

    Cloud& cloud_; Mqtt& mqtt_; Provider& provider_;
    const DeviceInstanceId& instance_; IClock& clock_; IRandom& random_;
    LocalAccessAuthority<Capacity> authority_;
    LocalAccessControlReceiver<Capacity, Profile> control_;
    FrameReceiver receiver_; void* receiverContext_;
    uint8_t inbox_[local_access::kControlMaximumEncodedSize];
    uint8_t inboxSize_;
    uint32_t inboxGeneration_, inboxConnection_, keyVersion_, generation_, connection_;
    uint8_t nonce_[local_access::kIdSize];
    uint32_t probeAt_, sentAt_, progressAt_, refreshAt_, confirmedSequence_, confirmedRevision_;
    uint8_t attempts_;
    bool started_, refreshAttempted_, probePending_, refreshRequested_;
    ErrorCode error_;
    LocalAccessCloudTransport(const LocalAccessCloudTransport&);
    LocalAccessCloudTransport& operator=(const LocalAccessCloudTransport&);
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(LocalAccessCloudTransport<1U>) <= 488U,
              "Single-slot local control wrapper exceeds its 32-bit ABI gate");
#endif
} // namespace blinker
#endif
