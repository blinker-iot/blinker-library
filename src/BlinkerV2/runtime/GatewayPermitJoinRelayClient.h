#ifndef BLINKER_RUNTIME_GATEWAYPERMITJOINRELAYCLIENT_H
#define BLINKER_RUNTIME_GATEWAYPERMITJOINRELAYCLIENT_H

#include "../interface/IClock.h"
#include "../interface/IGatewayPermitJoinRelayAdapter.h"
#include "../protocol/gateway/PermitJoinRelay.h"
#include "GatewayCloudMux.h"
#include "GatewayManagementClient.h"
#include "GatewayPermitJoinCoordinator.h"

namespace blinker {

// Operation-bound opaque enrollment relay. Each cloud slot carries at most
// sixteen original ATT packets; the adapter still owns BLE framing and this
// layer never parses Noise, grants, receipts or BBP/2.
class GatewayPermitJoinRelayClient {
public:
    GatewayPermitJoinRelayClient(
        GatewayCloudMux& cloud,
        GatewayManagementClient& management,
        GatewayPermitJoinCoordinator& coordinator,
        IGatewayPermitJoinRelayAdapter& adapter,
        IClock& clock);
    ~GatewayPermitJoinRelayClient();

    void poll();
    void reset();
    ErrorCode lastError() const { return lastError_; }
    bool active() const { return operationActive_; }
    uint32_t downFragmentCount() const { return downFragmentCount_; }
    uint32_t upFragmentCount() const { return upFragmentCount_; }
    bool waitingUpstreamAck() const { return upstreamPending_; }

private:
    enum : uint32_t {
        kSnapshotHeartbeatMillis = 2000U,
        kFragmentRetryMillis = 500U,
        kUpstreamCoalesceMillis = 20U,
        kRateWindowMillis = 1000U,
        kMaximumFragmentsPerOperation = 4096U
    };
    enum : uint8_t { kMaximumFragmentsPerSecond = 16U };

    static void messageThunk(
        void* context,
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);
    void onMessage(
        StringView topic,
        ByteView payload,
        const MqttMessageInfo& info);
    void beginOperation(ByteView operationId);
    void processInbound();
    void processSelect(ByteView encoded);
    bool processDownBatch(ByteView encoded);
    void processUpAck(ByteView encoded);
    void reportSelectionState();
    void publishCandidateSnapshot();
    void takeUpstreamBatch();
    void publishPending();
    Result queueSelectResult(
        ByteView token,
        gateway::PermitJoinSelectStatus status,
        uint16_t maxPacketSize);
    Result queueAck(
        gateway::PermitJoinRelayDirection direction,
        uint32_t sequence,
        gateway::PermitJoinRelayAckStatus status);
    bool sameOperation(ByteView operationId) const;
    bool sameTopic(StringView topic) const;
    bool consumeRate(bool upstream);
    uint32_t candidateHash() const;
    void clearOperation();

    GatewayCloudMux& cloud_;
    GatewayManagementClient& management_;
    GatewayPermitJoinCoordinator& coordinator_;
    IGatewayPermitJoinRelayAdapter& adapter_;
    IClock& clock_;
    uint8_t operationId_[gateway::kOperationIdSize];
    uint8_t selectedToken_[gateway::kPermitJoinCandidateTokenMaximumSize];
    uint8_t selectedTokenSize_;
    uint8_t inbound_[gateway::kPermitJoinRelayBatchMaximumEncodedSize];
    size_t inboundSize_;
    uint8_t outbound_[gateway::kPermitJoinCandidateSnapshotMaximumEncodedSize];
    size_t outboundSize_;
    uint8_t upstream_[gateway::kPermitJoinRelayBatchMaximumEncodedSize];
    size_t upstreamSize_;
    uint32_t revision_;
    uint32_t observedCandidateHash_;
    uint32_t nextSnapshotAt_;
    uint32_t nextUpstreamRetryAt_;
    uint32_t upstreamCoalesceAt_;
    uint32_t lastDownSequence_;
    uint32_t nextUpSequence_;
    uint32_t downFragmentCount_;
    uint32_t upFragmentCount_;
    uint32_t rateWindowStartedAt_;
    uint8_t downRateCount_;
    uint8_t upRateCount_;
    uint8_t downPacketIndex_;
    bool operationActive_;
    bool inboundPending_;
    bool outboundPending_;
    bool upstreamPending_;
    bool candidateHashKnown_;
    bool selectionSet_;
    bool selectionWasConnected_;
    ErrorCode lastError_;

    GatewayPermitJoinRelayClient(const GatewayPermitJoinRelayClient&);
    GatewayPermitJoinRelayClient& operator=(
        const GatewayPermitJoinRelayClient&);
};

} // namespace blinker

#endif
