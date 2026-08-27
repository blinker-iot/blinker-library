#ifndef BLINKER_TRANSPORT_BLEFRAMETRANSPORT_H
#define BLINKER_TRANSPORT_BLEFRAMETRANSPORT_H

#include "../protocol/bbp2/Frame.h"
#include "../security/DirectSecureSession.h"
#include "BleRecordTransport.h"

namespace blinker {

struct BleFrameTransportConfig {
    uint16_t maxFrameSize;
    uint32_t reassemblyTimeoutMillis;
    uint32_t authenticationTimeoutMillis;
    uint8_t maxPacketsPerPoll;

    BleFrameTransportConfig()
        : maxFrameSize(512U),
          reassemblyTimeoutMillis(2000U),
          authenticationTimeoutMillis(15000U),
          maxPacketsPerPoll(4U) {}
};

// BBP/2 compatibility wrapper over the protocol-neutral BLE record transport.
// All fragment ordering, timeout, MTU and queue logic lives in one place.
class BleFrameTransport :
    public IFrameTransport,
    public security::IDirectSecureSessionController {
public:
    BleFrameTransport(
        IBleLink& link,
        IClock& clock,
        IX25519AesGcmCryptoProvider& crypto,
        MutableByteSpan rxStorage,
        MutableByteSpan txStorage,
        MutableByteSpan packetScratch,
        MutableByteSpan plaintextScratch,
        const BleFrameTransportConfig& config = BleFrameTransportConfig());

    Result start() override;
    void stop() override;
    void poll(uint32_t budgetMicros) override;
    TransportState state() const override;
    TransportCapabilities capabilities() const override;
    Result send(ByteView frame, const SendTarget& target) override;
    Result disconnectSession(uint32_t sessionId) override;
    void setReceiver(FrameReceiver receiver, void* context) override;
    void setSessionHandlers(
        FrameSessionHandler connected,
        FrameSessionHandler disconnected,
        void* context) override;

    Result prepare(
        uint32_t sessionId,
        ByteView controllerSecret,
        const security::DirectSecureContext& context) override;
    void commit(uint32_t sessionId) override;
    void cancel(uint32_t sessionId) override;

    size_t queuedFrameCount() const { return records_.queuedRecordCount(); }
    uint32_t droppedFragmentCount() const {
        return records_.droppedFragmentCount();
    }
    uint32_t completedFrameCount() const {
        return records_.completedRecordCount();
    }

private:
    enum class SecureState : uint8_t {
        Empty = 0U,
        Pending,
        Prepared,
        Active
    };

    struct SecureSlot {
        uint32_t sessionId;
        uint32_t authenticationDeadlineMillis;
        security::DirectSecureSession session;
        SecureState state;

        SecureSlot()
            : sessionId(0U), authenticationDeadlineMillis(0U), session(),
              state(SecureState::Empty) {}
        void clear() {
            session.clear();
            sessionId = 0U;
            authenticationDeadlineMillis = 0U;
            state = SecureState::Empty;
        }
    };

    struct SealContext {
        BleFrameTransport* owner;
        SecureSlot* slot;
        ByteView frame;
        bool attempted;

        SealContext()
            : owner(nullptr), slot(nullptr), frame(), attempted(false) {}
    };

    static BleRecordFormat frameFormat();
    static BleRecordTransportConfig recordConfig(
        const BleFrameTransportConfig& config);
    static Result decodeFrameSize(ByteView prefix, size_t& frameSize);
    static Result validateFrame(ByteView frame);
    static void recordThunk(
        void* context,
        ByteView record,
        const RxContext& rx);
    static void connectedThunk(void* context, const RxContext& rx);
    static void disconnectedThunk(void* context, const RxContext& rx);
    static void faultThunk(
        void* context,
        ErrorCode error,
        const RxContext& rx);
    static Result sealThunk(
        void* context,
        MutableByteSpan storage,
        ByteView& record);

    void onRecord(ByteView record, const RxContext& rx);
    void onConnected(const RxContext& rx);
    void onDisconnected(const RxContext& rx);
    void onFault(ErrorCode error, const RxContext& rx);
    void retire(uint32_t sessionId, ErrorCode error);
    void expireUnauthenticatedSessions(uint32_t nowMillis);
    void clearSessions();
    bool hasSecureSession() const;
    SecureSlot* findSecure(uint32_t sessionId, bool create);
    const SecureSlot* findSecure(uint32_t sessionId) const;

    BleRecordTransport records_;
    IClock& clock_;
    IX25519AesGcmCryptoProvider& crypto_;
    MutableByteSpan plaintextScratch_;
    uint16_t maxFrameSize_;
    uint32_t authenticationTimeoutMillis_;
    SecureSlot secure_[BLINKER_BLE_MAX_SESSIONS];
    FrameReceiver receiver_;
    void* receiverContext_;
    FrameSessionHandler sessionConnected_;
    FrameSessionHandler sessionDisconnected_;
    void* sessionContext_;
};

} // namespace blinker

#endif
