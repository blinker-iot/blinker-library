#ifndef BLINKER_TRANSPORT_GATTDIRECTCHILDLINK_H
#define BLINKER_TRANSPORT_GATTDIRECTCHILDLINK_H

#include "../interface/IBleCentralPort.h"
#include "../interface/IBleLink.h"
#include "../interface/IClock.h"
#include "../protocol/DirectRecord.h"
#include "../security/DirectSecureSession.h"
#include "BleRecordTransport.h"

namespace blinker {

struct GattDirectChildLinkConfig {
    uint16_t maxFrameSize;
    uint32_t connectTimeoutMillis;
    uint32_t reassemblyTimeoutMillis;
    uint8_t maxPacketsPerPoll;

    GattDirectChildLinkConfig()
        : maxFrameSize(512U), connectTimeoutMillis(15000U),
          reassemblyTimeoutMillis(2000U), maxPacketsPerPoll(4U) {}
};

typedef void (*GattDirectChildFaultHandler)(
    void* context,
    ErrorCode error,
    uint32_t attemptId);

// One outbound GATT record link. It does not own discovery identity,
// credentials, Method 2 or routing. A caller supplies an authorized
// advertisement matcher and exchanges complete raw BBP/2 or DirectSecure
// records. Existing BleRecordTransport remains the only fragment/reassembly
// implementation.
class GattDirectChildLink {
public:
    GattDirectChildLink(
        IBleCentralPort& port,
        IClock& clock,
        MutableByteSpan rxStorage,
        MutableByteSpan txStorage,
        MutableByteSpan packetScratch,
        const GattDirectChildLinkConfig& config =
            GattDirectChildLinkConfig());

    Result start();
    void stop();
    void poll(uint32_t budgetMicros);
    Result connect(
        BleCentralAdvertisementMatcher matcher,
        void* matcherContext);
    Result disconnect();
    Result sendRecord(ByteView record);

    bool connected() const { return connectedAttemptId_ != 0U; }
    bool connecting() const { return connecting_; }
    uint32_t attemptId() const {
        return connectedAttemptId_ != 0U
                   ? connectedAttemptId_
                   : adapter_.attemptId();
    }
    size_t queuedRecordCount() const {
        return records_.queuedRecordCount();
    }
    uint32_t droppedFragmentCount() const {
        return records_.droppedFragmentCount();
    }
    uint32_t completedRecordCount() const {
        return records_.completedRecordCount();
    }

    void setReceiver(BleRecordReceiver receiver, void* context);
    void setSessionHandlers(
        BleRecordSessionHandler connected,
        BleRecordSessionHandler disconnected,
        void* context);
    void setFaultHandler(
        GattDirectChildFaultHandler handler,
        void* context);

private:
    static BleRecordFormat directRecordFormat() {
        BleRecordFormat format;
        format.minimumHeaderSize = security::kDirectSecureHeaderSize;
        format.decodeSize = &direct::decodeRecordSize;
        format.validate = &direct::validateRecord;
        return format;
    }

    class CentralAdapter final : public IBleLink {
    public:
        explicit CentralAdapter(IBleCentralPort& port);

        Result start() override;
        void stop() override;
        void poll(uint32_t budgetMicros) override;
        BleLinkState state() const override;
        ErrorCode lastError() const override;
        size_t sessionCount() const override;
        Result sessionAt(
            size_t index,
            BleSessionInfo& session) const override;
        Result sendPacket(
            uint32_t sessionId,
            ByteView packet) override;
        Result disconnectSession(uint32_t sessionId) override;
        void setPacketReceiver(
            BlePacketReceiver receiver,
            void* context) override;
        void setSessionHandlers(
            BleSessionHandler connected,
            BleSessionHandler disconnected,
            void* context) override;

        Result connect(const BleCentralConnectRequest& request);
        Result cancelCurrent();
        uint32_t attemptId() const { return attemptId_; }

    private:
        static void packetThunk(
            void* context,
            const BleCentralConnectionInfo& connection,
            ByteView packet);
        static void connectedThunk(
            void* context,
            const BleCentralConnectionInfo& connection);
        static void disconnectedThunk(
            void* context,
            const BleCentralConnectionInfo& connection);

        void onPacket(
            const BleCentralConnectionInfo& connection,
            ByteView packet);
        void onConnected(const BleCentralConnectionInfo& connection);
        void onDisconnected(const BleCentralConnectionInfo& connection);
        void clearConnection();
        static bool validConnection(
            const BleCentralConnectionInfo& connection);

        IBleCentralPort& port_;
        BleSessionInfo session_;
        BlePacketReceiver packetReceiver_;
        void* packetContext_;
        BleSessionHandler connectedHandler_;
        BleSessionHandler disconnectedHandler_;
        void* sessionContext_;
        uint32_t attemptId_;
        ErrorCode lastError_;
        bool started_;
        bool announced_;
        bool closing_;
    };

    static BleRecordTransportConfig recordConfig(
        const GattDirectChildLinkConfig& config);
    Result validateConfig() const;
    void emitFault(ErrorCode error, uint32_t attemptId);
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

    void onRecord(ByteView record, const RxContext& rx);
    void onConnected(const RxContext& rx);
    void onDisconnected(const RxContext& rx);
    void onRecordFault(ErrorCode error, const RxContext& rx);

    CentralAdapter adapter_;
    BleRecordTransport records_;
    IClock& clock_;
    GattDirectChildLinkConfig config_;
    BleRecordReceiver receiver_;
    void* receiverContext_;
    BleRecordSessionHandler connectedHandler_;
    BleRecordSessionHandler disconnectedHandler_;
    void* sessionContext_;
    GattDirectChildFaultHandler faultHandler_;
    void* faultContext_;
    uint32_t nextAttemptId_;
    uint32_t connectStartedMillis_;
    uint32_t connectingAttemptId_;
    uint32_t connectedAttemptId_;
    uint32_t retireAttemptId_;
    uint32_t faultRetirementAttemptId_;
    ErrorCode reportedPortError_;
    bool started_;
    bool connecting_;
};

} // namespace blinker

#endif
