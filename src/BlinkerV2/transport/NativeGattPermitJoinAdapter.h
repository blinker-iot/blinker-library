#ifndef BLINKER_TRANSPORT_NATIVEGATTPERMITJOINADAPTER_H
#define BLINKER_TRANSPORT_NATIVEGATTPERMITJOINADAPTER_H

#include "../interface/IBleCentralPort.h"
#include "../interface/IClock.h"
#include "../interface/IGatewayPermitJoinAdapter.h"
#include "../interface/IGatewayPermitJoinRelayAdapter.h"
#include "../protocol/SetupSession.h"
#include "../protocol/ble/Mode.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

#ifndef BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES
#define BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES 4
#endif

#ifndef BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE
#define BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE 20
#endif

#ifndef BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH
#define BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH 32
#endif

#if BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES < 1 || \
    BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES > 4
#error "BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES must be 1..4"
#endif
#if BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE != 20
#error "BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE must be 20"
#endif
#if BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH < 4 || \
    BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH > 32
#error "BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH must be 4..32"
#endif
struct NativeGattPermitJoinConfig {
    uint32_t candidateTtlMillis;

    NativeGattPermitJoinConfig() : candidateTtlMillis(10000U) {}
};

// Candidate identity is the rotating setup-session or authorized-presence
// locator. Native BLE addresses deliberately stay inside IBleCentralPort.
// One Native GATT permit-join window. It filters only the public provisioning
// or authorized Direct advertisement and forwards bounded BLE fragments
// unchanged. It never
// parses Noise, grants, receipts, Controller credentials or BBP/2.
class NativeGattPermitJoinAdapter final
    : public IGatewayPermitJoinAdapter,
      public IGatewayPermitJoinRelayAdapter {
public:
    NativeGattPermitJoinAdapter(
        IBleCentralPort& port,
        IClock& clock,
        IGatewayPermitJoinPortLease& lease,
        const NativeGattPermitJoinConfig& config =
            NativeGattPermitJoinConfig());
    ~NativeGattPermitJoinAdapter() override;

    uint16_t adapterId() const override {
        return gateway::kGatewaySouthboundAdapterNativeGatt;
    }
    Result openWindow() override;
    void closeWindow() override;
    void poll(uint32_t budgetMicros) override;
    GatewayPermitJoinAdapterState state() const override {
        return state_;
    }
    ErrorCode lastError() const override { return lastError_; }

    size_t relayCandidateCount() const override;
    Result readRelayCandidate(
        size_t index,
        GatewayPermitJoinCandidate& output) const override;
    Result selectRelayCandidate(ByteView token) override;
    void clearRelaySelection() override;

    Result queueRelayPacket(ByteView packet) override;
    Result takeRelayPacket(
        MutableByteSpan output,
        size_t& written) override;
    size_t relayPacketCount() const override { return rxCount_; }
    bool relayConnected() const override {
        return connectedAttemptId_ != 0U;
    }
    uint16_t relayPacketSize() const override { return maxPacketSize_; }
    uint32_t droppedPacketCount() const { return droppedPackets_; }

private:
    struct CandidateSlot {
        GatewayPermitJoinCandidate candidate;
        int16_t rssi;
        uint32_t lastSeenMillis;
        bool occupied;

        CandidateSlot()
            : candidate(), rssi(0), lastSeenMillis(0U), occupied(false) {}
    };

    struct PacketSlot {
        uint8_t bytes[BLINKER_GATEWAY_PERMIT_JOIN_PACKET_SIZE];
        uint8_t size;

        PacketSlot() : bytes(), size(0U) {}
    };

    static bool advertisementThunk(
        void* context,
        const BleCentralAdvertisement& advertisement);
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

    bool onAdvertisement(
        const BleCentralAdvertisement& advertisement);
    void onPacket(
        const BleCentralConnectionInfo& connection,
        ByteView packet);
    void onConnected(const BleCentralConnectionInfo& connection);
    void onDisconnected(const BleCentralConnectionInfo& connection);
    Result startScan();
    void expireCandidates();
    void fail(ErrorCode error);
    void clearRuntime();
    bool validConfig() const;
    bool selected(ByteView locator) const;
    static bool sameLocator(const uint8_t* left, ByteView right);

    IBleCentralPort& port_;
    IClock& clock_;
    IGatewayPermitJoinPortLease& lease_;
    NativeGattPermitJoinConfig config_;
    CandidateSlot candidates_[BLINKER_GATEWAY_PERMIT_JOIN_CANDIDATES];
    PacketSlot rx_[BLINKER_GATEWAY_PERMIT_JOIN_RX_PACKET_QUEUE_DEPTH];
    uint8_t selectedLocator_[kSetupSessionLocatorSize];
    uint8_t rxHead_;
    uint8_t rxTail_;
    uint8_t rxCount_;
    uint32_t nextAttemptId_;
    uint32_t activeAttemptId_;
    uint32_t connectedAttemptId_;
    uint32_t droppedPackets_;
    uint16_t maxPacketSize_;
    GatewayPermitJoinAdapterState state_;
    ErrorCode lastError_;
    bool leased_;
    bool selectionSet_;

    NativeGattPermitJoinAdapter(const NativeGattPermitJoinAdapter&);
    NativeGattPermitJoinAdapter& operator=(
        const NativeGattPermitJoinAdapter&);
};

} // namespace blinker

#endif
