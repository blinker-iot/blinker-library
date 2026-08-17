#ifndef BLINKER_TRANSPORT_BLERECORDTRANSPORT_H
#define BLINKER_TRANSPORT_BLERECORDTRANSPORT_H

#include "../core/ResourceProfile.h"
#include "../interface/IBleLink.h"
#include "../interface/IClock.h"
#include "../protocol/ble/Fragment.h"
#include "IFrameTransport.h"

namespace blinker {

typedef Result (*BleRecordSizeDecoder)(
    ByteView prefix,
    size_t& recordSize);
typedef Result (*BleRecordValidator)(ByteView record);

typedef void (*BleRecordReceiver)(
    void* context,
    ByteView record,
    const RxContext& rx);

typedef void (*BleRecordSessionHandler)(
    void* context,
    const RxContext& rx);

struct BleRecordFormat {
    uint16_t minimumHeaderSize;
    BleRecordSizeDecoder decodeSize;
    BleRecordValidator validate;

    BleRecordFormat()
        : minimumHeaderSize(0U), decodeSize(nullptr), validate(nullptr) {}
};

struct BleRecordTransportConfig {
    uint16_t maxRecordSize;
    uint32_t reassemblyTimeoutMillis;
    uint8_t maxPacketsPerPoll;
    uint8_t sessionCapacity;
    uint8_t txRecordCapacity;

    BleRecordTransportConfig()
        : maxRecordSize(512U),
          reassemblyTimeoutMillis(2000U),
          maxPacketsPerPoll(4U),
          sessionCapacity(BLINKER_BLE_MAX_SESSIONS),
          txRecordCapacity(BLINKER_BLE_MAX_TX_FRAMES) {}
};

typedef void (*BleRecordFaultHandler)(
    void* context,
    ErrorCode error,
    const RxContext& rx);

// Protocol-neutral logical-record transport over the frozen 4-byte BLE
// fragment header. A BleRecordFormat supplies only record-length decoding and
// complete-record validation; this class owns all ordering, stride, timeout,
// MTU-restart, queue and per-session behavior.
class BleRecordTransport {
public:
    // rxStorage requires maxRecordSize * sessionCapacity bytes. txStorage
    // requires maxRecordSize * txRecordCapacity bytes. packetScratch is used
    // only during sendPacket and must hold one negotiated ATT payload.
    BleRecordTransport(
        IBleLink& link,
        IClock& clock,
        const BleRecordFormat& format,
        MutableByteSpan rxStorage,
        MutableByteSpan txStorage,
        MutableByteSpan packetScratch,
        const BleRecordTransportConfig& config =
            BleRecordTransportConfig());

    Result start();
    void stop();
    void poll(uint32_t budgetMicros);
    TransportState state() const;
    // Checks transport/session/queue capacity without requiring a record to
    // have been materialized yet. Secure callers use this before advancing a
    // handshake transcript or transport nonce.
    Result canQueue(size_t recordSize, const SendTarget& target) const;
    Result canSend(ByteView record, const SendTarget& target) const;
    Result send(ByteView record, const SendTarget& target);
    void resetSession(uint32_t sessionId);

    void setReceiver(BleRecordReceiver receiver, void* context);
    void setSessionHandlers(
        BleRecordSessionHandler connected,
        BleRecordSessionHandler disconnected,
        void* context);
    void setFaultHandler(BleRecordFaultHandler handler, void* context);

    uint16_t maxRecordSize() const { return config_.maxRecordSize; }
    size_t queuedRecordCount() const { return txCount_; }
    uint32_t droppedFragmentCount() const { return droppedFragments_; }
    uint32_t completedRecordCount() const { return completedRecords_; }

private:
    struct RxSession {
        BleSessionInfo info;
        uint16_t totalLength;
        uint16_t receivedLength;
        uint16_t fragmentPayloadCapacity;
        uint16_t nextFragmentIndex;
        uint32_t lastFragmentMillis;
        uint8_t recordId;
        uint8_t lastRecordId;
        uint8_t nextTxRecordId;
        bool occupied;
        bool assembling;
        bool hasLastRecordId;

        RxSession()
            : info(),
              totalLength(0U),
              receivedLength(0U),
              fragmentPayloadCapacity(0U),
              nextFragmentIndex(0U),
              lastFragmentMillis(0U),
              recordId(0U),
              lastRecordId(0U),
              nextTxRecordId(1U),
              occupied(false),
              assembling(false),
              hasLastRecordId(false) {}
    };

    struct TxRecord {
        uint32_t sessionId;
        uint16_t recordLength;
        uint16_t offset;
        uint16_t fragmentPayloadCapacity;
        uint8_t recordId;
        uint8_t fragmentIndex;
        bool occupied;

        TxRecord()
            : sessionId(0U),
              recordLength(0U),
              offset(0U),
              fragmentPayloadCapacity(0U),
              recordId(0U),
              fragmentIndex(0U),
              occupied(false) {}
    };

    static void packetThunk(
        void* context,
        const BleSessionInfo& session,
        ByteView packet);
    static void connectedThunk(
        void* context,
        const BleSessionInfo& session);
    static void disconnectedThunk(
        void* context,
        const BleSessionInfo& session);

    void onPacket(const BleSessionInfo& session, ByteView packet);
    void onConnected(const BleSessionInfo& session);
    void onDisconnected(const BleSessionInfo& session);
    void reconcileSessions();
    void retireSession(
        RxSession& session,
        ErrorCode error,
        const BleSessionInfo& faultInfo);
    void flushTx();
    void expireReassembly(uint32_t now);
    void abortRx(RxSession& session, ErrorCode error);
    void emitFault(const BleSessionInfo& session, ErrorCode error);
    RxSession* findRx(uint32_t sessionId, bool create);
    const RxSession* findRx(uint32_t sessionId) const;
    size_t rxIndex(const RxSession* session) const;
    MutableByteSpan rxBuffer(size_t index) const;
    MutableByteSpan txBuffer(size_t index) const;
    Result enqueue(const BleSessionInfo& session, ByteView record);
    void popTx();
    void dropQueuedForSession(uint32_t sessionId);
    uint8_t nextRecordId(RxSession& session);
    size_t txCapacity() const;
    size_t packetPayloadCapacity(const BleSessionInfo& session) const;
    static size_t fragmentCount(size_t recordSize, size_t payloadCapacity);
    static bool usable(const BleSessionInfo& session);
    static bool securityDowngraded(
        const BleSessionInfo& admitted,
        const BleSessionInfo& current);
    static RxContext makeRxContext(const BleSessionInfo& session);

    IBleLink& link_;
    IClock& clock_;
    BleRecordFormat format_;
    MutableByteSpan rxStorage_;
    MutableByteSpan txStorage_;
    MutableByteSpan packetScratch_;
    BleRecordTransportConfig config_;
    RxSession rxSessions_[BLINKER_BLE_MAX_SESSIONS];
    TxRecord txRecords_[BLINKER_BLE_MAX_TX_FRAMES];
    size_t txHead_;
    size_t txTail_;
    size_t txCount_;
    BleRecordReceiver receiver_;
    void* receiverContext_;
    BleRecordSessionHandler sessionConnected_;
    BleRecordSessionHandler sessionDisconnected_;
    void* sessionContext_;
    BleRecordFaultHandler faultHandler_;
    void* faultContext_;
    uint32_t droppedFragments_;
    uint32_t completedRecords_;
    bool started_;
};

} // namespace blinker

#endif
