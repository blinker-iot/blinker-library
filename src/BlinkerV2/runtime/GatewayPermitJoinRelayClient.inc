#include "GatewayPermitJoinRelayClient.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

GatewayPermitJoinRelayClient::GatewayPermitJoinRelayClient(
    GatewayCloudMux& cloud,
    GatewayManagementClient& management,
    GatewayPermitJoinCoordinator& coordinator,
    IGatewayPermitJoinRelayAdapter& adapter,
    IClock& clock)
    : cloud_(cloud), management_(management), coordinator_(coordinator),
      adapter_(adapter), clock_(clock), operationId_(), selectedToken_(),
      selectedTokenSize_(0U), inbound_(), inboundSize_(0U), outbound_(),
      outboundSize_(0U), upstream_(), upstreamSize_(0U), revision_(0U),
      observedCandidateHash_(0U), nextSnapshotAt_(0U),
      nextUpstreamRetryAt_(0U), upstreamCoalesceAt_(0U),
      lastDownSequence_(0U), nextUpSequence_(1U),
      downFragmentCount_(0U), upFragmentCount_(0U),
      rateWindowStartedAt_(0U), downRateCount_(0U), upRateCount_(0U),
      downPacketIndex_(0U),
      operationActive_(false), inboundPending_(false),
      outboundPending_(false), upstreamPending_(false),
      candidateHashKnown_(false), selectionSet_(false),
      selectionWasConnected_(false), lastError_(ErrorCode::Ok) {
    (void)cloud_.bind(
        GatewayCloudChannel::PermitJoinRelay,
        &GatewayPermitJoinRelayClient::messageThunk, this);
}

GatewayPermitJoinRelayClient::~GatewayPermitJoinRelayClient() {
    cloud_.unbind(GatewayCloudChannel::PermitJoinRelay, this);
    clearOperation();
}

void GatewayPermitJoinRelayClient::poll() {
    if (!coordinator_.windowOpen() || !management_.configured() ||
        !management_.subscribed() || cloud_.state() != TransportState::Online) {
        clearOperation();
        return;
    }
    const ByteView active = coordinator_.activeOperationId();
    if (!operationActive_ || !sameOperation(active)) beginOperation(active);

    if (inboundPending_ && !outboundPending_) processInbound();
    reportSelectionState();
    publishPending();
    if (!outboundPending_) {
        publishCandidateSnapshot();
        publishPending();
    }
    if (!upstreamPending_) takeUpstreamBatch();
    publishPending();
}

void GatewayPermitJoinRelayClient::reset() {
    clearOperation();
}

void GatewayPermitJoinRelayClient::messageThunk(
    void* context,
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    GatewayPermitJoinRelayClient* self =
        static_cast<GatewayPermitJoinRelayClient*>(context);
    if (self != nullptr) self->onMessage(topic, payload, info);
}

void GatewayPermitJoinRelayClient::onMessage(
    StringView topic,
    ByteView payload,
    const MqttMessageInfo& info) {
    const gateway::PermitJoinRelayMessageKind kind =
        gateway::permitJoinRelayMessageKind(payload);
    if (!operationActive_ || inboundPending_ || !sameTopic(topic) ||
        payload.data == nullptr || payload.empty() ||
        payload.size > sizeof(inbound_) ||
        (info.qosKnown && info.qos != 1U) ||
        (info.retainedKnown && info.retained) ||
        (kind != gateway::PermitJoinRelayMessageKind::SelectCommand &&
         kind != gateway::PermitJoinRelayMessageKind::RelayBatch &&
         kind != gateway::PermitJoinRelayMessageKind::RelayAck)) {
        return;
    }
    memcpy(inbound_, payload.data, payload.size);
    inboundSize_ = payload.size;
    inboundPending_ = true;
}

void GatewayPermitJoinRelayClient::beginOperation(ByteView operationId) {
    clearOperation();
    if (operationId.data == nullptr ||
        operationId.size != sizeof(operationId_)) {
        lastError_ = ErrorCode::InvalidArgument;
        return;
    }
    memcpy(operationId_, operationId.data, sizeof(operationId_));
    operationActive_ = true;
    nextSnapshotAt_ = clock_.monotonicMillis();
}

void GatewayPermitJoinRelayClient::processInbound() {
    const ByteView encoded(inbound_, inboundSize_);
    const gateway::PermitJoinRelayMessageKind kind =
        gateway::permitJoinRelayMessageKind(encoded);
    bool consumed = true;
    if (kind == gateway::PermitJoinRelayMessageKind::SelectCommand) {
        processSelect(encoded);
    } else if (kind == gateway::PermitJoinRelayMessageKind::RelayBatch) {
        consumed = processDownBatch(encoded);
    } else if (kind == gateway::PermitJoinRelayMessageKind::RelayAck) {
        processUpAck(encoded);
    }
    if (!consumed) return;
    secureZero(MutableByteSpan(inbound_, sizeof(inbound_)));
    inboundSize_ = 0U;
    inboundPending_ = false;
}

void GatewayPermitJoinRelayClient::processSelect(ByteView encoded) {
    gateway::PermitJoinSelectCommandView command;
    Result result = gateway::decodePermitJoinSelectCommand(encoded, command);
    if (!result || !sameOperation(command.operationId)) {
        lastError_ = result ? ErrorCode::AuthenticationRequired : result.code();
        return;
    }
    if (selectionSet_ &&
        (command.candidateToken.size != selectedTokenSize_ ||
         memcmp(command.candidateToken.data, selectedToken_,
                selectedTokenSize_) != 0)) {
        (void)queueSelectResult(
            command.candidateToken,
            gateway::PermitJoinSelectStatus::Rejected, 0U);
        return;
    }
    result = adapter_.selectRelayCandidate(command.candidateToken);
    gateway::PermitJoinSelectStatus status =
        gateway::PermitJoinSelectStatus::Connecting;
    if (!result) {
        status = result.code() == ErrorCode::NotFound
                     ? gateway::PermitJoinSelectStatus::NotFound
                     : gateway::PermitJoinSelectStatus::Rejected;
    } else {
        memcpy(selectedToken_, command.candidateToken.data,
               command.candidateToken.size);
        selectedTokenSize_ =
            static_cast<uint8_t>(command.candidateToken.size);
        selectionSet_ = true;
        if (adapter_.relayConnected()) {
            status = gateway::PermitJoinSelectStatus::Connected;
            selectionWasConnected_ = true;
        }
    }
    (void)queueSelectResult(
        command.candidateToken, status,
        status == gateway::PermitJoinSelectStatus::Connected
            ? adapter_.relayPacketSize()
            : 0U);
}

bool GatewayPermitJoinRelayClient::processDownBatch(ByteView encoded) {
    gateway::PermitJoinRelayBatchView batch;
    Result result = gateway::decodePermitJoinRelayBatch(encoded, batch);
    if (!result || !sameOperation(batch.operationId) ||
        batch.direction != gateway::PermitJoinRelayDirection::Down) {
        lastError_ = result ? ErrorCode::AuthenticationRequired : result.code();
        downPacketIndex_ = 0U;
        return true;
    }
    gateway::PermitJoinRelayAckStatus status =
        gateway::PermitJoinRelayAckStatus::Rejected;
    if (batch.sequence == lastDownSequence_ && lastDownSequence_ != 0U) {
        status = gateway::PermitJoinRelayAckStatus::Accepted;
    } else if (batch.sequence != lastDownSequence_ + 1U) {
        lastError_ = ErrorCode::SequenceConflict;
    } else if (batch.packetCount >
               kMaximumFragmentsPerOperation - downFragmentCount_) {
        lastError_ = ErrorCode::CapacityExceeded;
    } else {
        while (downPacketIndex_ < batch.packetCount) {
            if (!adapter_.relayConnected() || !consumeRate(false)) {
                // Keep the exact encoded batch and current packet index. No
                // cloud ACK is emitted until every southbound Write Response
                // has completed.
                return false;
            }
            const Result accepted = adapter_.queueRelayPacket(
                batch.packets[downPacketIndex_]);
            if (!accepted &&
                (accepted.code() == ErrorCode::WouldBlock ||
                 accepted.code() == ErrorCode::NotConnected)) {
                --downRateCount_;
                return false;
            }
            if (!accepted) {
                lastError_ = accepted.code();
                downPacketIndex_ = 0U;
                (void)queueAck(
                    gateway::PermitJoinRelayDirection::Down,
                    batch.sequence,
                    gateway::PermitJoinRelayAckStatus::Rejected);
                return true;
            }
            ++downPacketIndex_;
            ++downFragmentCount_;
        }
        lastDownSequence_ = batch.sequence;
        downPacketIndex_ = 0U;
        status = gateway::PermitJoinRelayAckStatus::Accepted;
    }
    (void)queueAck(
        gateway::PermitJoinRelayDirection::Down,
        batch.sequence, status);
    return true;
}

void GatewayPermitJoinRelayClient::processUpAck(ByteView encoded) {
    gateway::PermitJoinRelayAckView ack;
    const Result result = gateway::decodePermitJoinRelayAck(encoded, ack);
    if (!result || !sameOperation(ack.operationId) ||
        ack.direction != gateway::PermitJoinRelayDirection::Up ||
        !upstreamPending_ || ack.sequence + 1U != nextUpSequence_) {
        lastError_ = result ? ErrorCode::SequenceConflict : result.code();
        return;
    }
    secureZero(MutableByteSpan(upstream_, sizeof(upstream_)));
    upstreamSize_ = 0U;
    upstreamPending_ = false;
    if (ack.status != gateway::PermitJoinRelayAckStatus::Accepted) {
        lastError_ = ErrorCode::ProtocolError;
    }
}

void GatewayPermitJoinRelayClient::reportSelectionState() {
    if (!selectionSet_ || outboundPending_) return;
    const bool connected = adapter_.relayConnected();
    if (connected == selectionWasConnected_) return;
    selectionWasConnected_ = connected;
    (void)queueSelectResult(
        ByteView(selectedToken_, selectedTokenSize_),
        connected ? gateway::PermitJoinSelectStatus::Connected
                  : gateway::PermitJoinSelectStatus::Disconnected,
        connected ? adapter_.relayPacketSize() : 0U);
}

void GatewayPermitJoinRelayClient::publishCandidateSnapshot() {
    const uint32_t now = clock_.monotonicMillis();
    const uint32_t hash = candidateHash();
    const bool changed = !candidateHashKnown_ || hash != observedCandidateHash_;
    if (!changed && static_cast<int32_t>(now - nextSnapshotAt_) < 0) return;
    if (changed) {
        observedCandidateHash_ = hash;
        candidateHashKnown_ = true;
        ++revision_;
        if (revision_ == 0U) ++revision_;
    }
    gateway::PermitJoinCandidateSnapshotView snapshot;
    snapshot.operationId = ByteView(operationId_, sizeof(operationId_));
    snapshot.revision = revision_ == 0U ? 1U : revision_;
    const size_t count = adapter_.relayCandidateCount();
    snapshot.candidateCount =
        count < gateway::kPermitJoinCandidateMaximumCount
            ? count
            : gateway::kPermitJoinCandidateMaximumCount;
    GatewayPermitJoinCandidate candidate[
        gateway::kPermitJoinCandidateMaximumCount];
    for (size_t index = 0U; index < snapshot.candidateCount; ++index) {
        const Result result = adapter_.readRelayCandidate(index, candidate[index]);
        if (!result || candidate[index].tokenSize == 0U ||
            candidate[index].tokenSize > sizeof(candidate[index].token)) {
            lastError_ = result ? ErrorCode::InvalidEncoding : result.code();
            return;
        }
        snapshot.candidates[index].token = ByteView(
            candidate[index].token, candidate[index].tokenSize);
        snapshot.candidates[index].wireVersion = candidate[index].wireVersion;
        snapshot.candidates[index].capabilities = candidate[index].capabilities;
        snapshot.candidates[index].signalQuality = candidate[index].signalQuality;
    }
    size_t written = 0U;
    const Result encoded = gateway::encodePermitJoinCandidateSnapshot(
        snapshot, MutableByteSpan(outbound_, sizeof(outbound_)), written);
    if (!encoded) {
        lastError_ = encoded.code();
        return;
    }
    outboundSize_ = written;
    outboundPending_ = true;
    nextSnapshotAt_ = now + kSnapshotHeartbeatMillis;
}

void GatewayPermitJoinRelayClient::takeUpstreamBatch() {
    if (!adapter_.relayConnected() ||
        upFragmentCount_ >= kMaximumFragmentsPerOperation) {
        upstreamCoalesceAt_ = 0U;
        return;
    }
    const uint32_t now = clock_.monotonicMillis();
    if (static_cast<uint32_t>(now - rateWindowStartedAt_) >=
        kRateWindowMillis) {
        rateWindowStartedAt_ = now;
        downRateCount_ = 0U;
        upRateCount_ = 0U;
    }
    const size_t available = adapter_.relayPacketCount();
    if (available == 0U) {
        upstreamCoalesceAt_ = 0U;
        return;
    }
    if (available < gateway::kPermitJoinRelayBatchMaximumPackets) {
        if (upstreamCoalesceAt_ == 0U) {
            upstreamCoalesceAt_ = now + kUpstreamCoalesceMillis;
            return;
        }
        if (static_cast<int32_t>(now - upstreamCoalesceAt_) < 0) return;
    }
    upstreamCoalesceAt_ = 0U;
    const size_t rateCredit =
        kMaximumFragmentsPerSecond - upRateCount_;
    if (rateCredit == 0U) return;
    size_t packetCount = available;
    if (packetCount > gateway::kPermitJoinRelayBatchMaximumPackets) {
        packetCount = gateway::kPermitJoinRelayBatchMaximumPackets;
    }
    if (packetCount > rateCredit) packetCount = rateCredit;
    const size_t operationCredit =
        kMaximumFragmentsPerOperation - upFragmentCount_;
    if (packetCount > operationCredit) packetCount = operationCredit;

    uint8_t packetBytes[gateway::kPermitJoinRelayBatchMaximumPackets]
                       [gateway::kPermitJoinRelayPacketMaximumSize] = {};
    gateway::PermitJoinRelayBatchView batch;
    batch.operationId = ByteView(operationId_, sizeof(operationId_));
    batch.direction = gateway::PermitJoinRelayDirection::Up;
    batch.sequence = nextUpSequence_;
    for (size_t index = 0U; index < packetCount; ++index) {
        size_t written = 0U;
        const Result result = adapter_.takeRelayPacket(
            MutableByteSpan(packetBytes[index], sizeof(packetBytes[index])),
            written);
        if (!result) {
            if (result.code() != ErrorCode::NotFound) lastError_ = result.code();
            packetCount = index;
            break;
        }
        batch.packets[index] = ByteView(packetBytes[index], written);
    }
    batch.packetCount = packetCount;
    if (packetCount == 0U) {
        secureZero(MutableByteSpan(
            &packetBytes[0][0], sizeof(packetBytes)));
        return;
    }
    size_t encodedSize = 0U;
    const Result encoded = gateway::encodePermitJoinRelayBatch(
        batch, MutableByteSpan(upstream_, sizeof(upstream_)), encodedSize);
    secureZero(MutableByteSpan(&packetBytes[0][0], sizeof(packetBytes)));
    if (!encoded) {
        lastError_ = encoded.code();
        return;
    }
    upstreamSize_ = encodedSize;
    upstreamPending_ = true;
    ++nextUpSequence_;
    upFragmentCount_ += static_cast<uint32_t>(packetCount);
    upRateCount_ = static_cast<uint8_t>(upRateCount_ + packetCount);
    nextUpstreamRetryAt_ = clock_.monotonicMillis();
}

void GatewayPermitJoinRelayClient::publishPending() {
    ByteView payload;
    bool ordinary = false;
    if (outboundPending_) {
        payload = ByteView(outbound_, outboundSize_);
        ordinary = true;
    } else if (upstreamPending_ &&
               static_cast<int32_t>(
                   clock_.monotonicMillis() - nextUpstreamRetryAt_) >= 0) {
        payload = ByteView(upstream_, upstreamSize_);
    } else {
        return;
    }
    MqttPublishOptions options;
    options.qos = 0U;
    options.retained = false;
    const Result result = cloud_.publish(
        management_.publishTopic(), payload, options);
    if (!result) {
        lastError_ = result.code();
        return;
    }
    if (ordinary) {
        secureZero(MutableByteSpan(outbound_, sizeof(outbound_)));
        outboundSize_ = 0U;
        outboundPending_ = false;
    } else {
        nextUpstreamRetryAt_ =
            clock_.monotonicMillis() + kFragmentRetryMillis;
    }
    lastError_ = ErrorCode::Ok;
}

Result GatewayPermitJoinRelayClient::queueSelectResult(
    ByteView token,
    gateway::PermitJoinSelectStatus status,
    uint16_t maxPacketSize) {
    if (outboundPending_) return Result::failure(ErrorCode::WouldBlock);
    gateway::PermitJoinSelectResultView value;
    value.operationId = ByteView(operationId_, sizeof(operationId_));
    value.candidateToken = token;
    value.status = status;
    value.maxPacketSize = maxPacketSize;
    size_t written = 0U;
    const Result result = gateway::encodePermitJoinSelectResult(
        value, MutableByteSpan(outbound_, sizeof(outbound_)), written);
    if (result) {
        outboundSize_ = written;
        outboundPending_ = true;
    }
    return result;
}

Result GatewayPermitJoinRelayClient::queueAck(
    gateway::PermitJoinRelayDirection direction,
    uint32_t sequence,
    gateway::PermitJoinRelayAckStatus status) {
    if (outboundPending_) return Result::failure(ErrorCode::WouldBlock);
    gateway::PermitJoinRelayAckView value;
    value.operationId = ByteView(operationId_, sizeof(operationId_));
    value.direction = direction;
    value.sequence = sequence;
    value.status = status;
    size_t written = 0U;
    const Result result = gateway::encodePermitJoinRelayAck(
        value, MutableByteSpan(outbound_, sizeof(outbound_)), written);
    if (result) {
        outboundSize_ = written;
        outboundPending_ = true;
    }
    return result;
}

bool GatewayPermitJoinRelayClient::sameOperation(ByteView operationId) const {
    return operationActive_ && operationId.data != nullptr &&
        operationId.size == sizeof(operationId_) &&
        memcmp(operationId.data, operationId_, sizeof(operationId_)) == 0;
}

bool GatewayPermitJoinRelayClient::sameTopic(StringView topic) const {
    const StringView expected = management_.subscribeTopic();
    return topic.data != nullptr && expected.data != nullptr &&
        topic.size == expected.size &&
        memcmp(topic.data, expected.data, topic.size) == 0;
}

bool GatewayPermitJoinRelayClient::consumeRate(bool upstream) {
    const uint32_t now = clock_.monotonicMillis();
    if (static_cast<uint32_t>(now - rateWindowStartedAt_) >=
        kRateWindowMillis) {
        rateWindowStartedAt_ = now;
        downRateCount_ = 0U;
        upRateCount_ = 0U;
    }
    uint8_t& count = upstream ? upRateCount_ : downRateCount_;
    if (count >= kMaximumFragmentsPerSecond) return false;
    ++count;
    return true;
}

uint32_t GatewayPermitJoinRelayClient::candidateHash() const {
    uint32_t hash = 2166136261UL;
    const size_t count = adapter_.relayCandidateCount();
    const size_t bounded = count < gateway::kPermitJoinCandidateMaximumCount
                               ? count
                               : gateway::kPermitJoinCandidateMaximumCount;
    for (size_t index = 0U; index < bounded; ++index) {
        GatewayPermitJoinCandidate candidate;
        if (!adapter_.readRelayCandidate(index, candidate)) {
            return hash ^ 0xFFFFFFFFUL;
        }
        if (candidate.tokenSize == 0U ||
            candidate.tokenSize > sizeof(candidate.token)) {
            return hash ^ 0x7FFFFFFFUL;
        }
        const uint8_t prefix[] = {
            candidate.tokenSize,
            candidate.wireVersion,
            static_cast<uint8_t>(candidate.capabilities >> 8U),
            static_cast<uint8_t>(candidate.capabilities),
            candidate.signalQuality
        };
        for (size_t offset = 0U; offset < sizeof(prefix); ++offset) {
            hash = (hash ^ prefix[offset]) * 16777619UL;
        }
        for (size_t offset = 0U; offset < candidate.tokenSize; ++offset) {
            hash = (hash ^ candidate.token[offset]) * 16777619UL;
        }
    }
    return (hash ^ static_cast<uint32_t>(bounded)) * 16777619UL;
}

void GatewayPermitJoinRelayClient::clearOperation() {
    secureZero(MutableByteSpan(operationId_, sizeof(operationId_)));
    secureZero(MutableByteSpan(selectedToken_, sizeof(selectedToken_)));
    secureZero(MutableByteSpan(inbound_, sizeof(inbound_)));
    secureZero(MutableByteSpan(outbound_, sizeof(outbound_)));
    secureZero(MutableByteSpan(upstream_, sizeof(upstream_)));
    selectedTokenSize_ = 0U;
    inboundSize_ = outboundSize_ = upstreamSize_ = 0U;
    revision_ = observedCandidateHash_ = nextSnapshotAt_ = 0U;
    nextUpstreamRetryAt_ = upstreamCoalesceAt_ = lastDownSequence_ = 0U;
    nextUpSequence_ = 1U;
    downFragmentCount_ = upFragmentCount_ = rateWindowStartedAt_ = 0U;
    downRateCount_ = upRateCount_ = downPacketIndex_ = 0U;
    operationActive_ = inboundPending_ = outboundPending_ = false;
    upstreamPending_ = candidateHashKnown_ = selectionSet_ = false;
    selectionWasConnected_ = false;
    lastError_ = ErrorCode::Ok;
}

} // namespace blinker
