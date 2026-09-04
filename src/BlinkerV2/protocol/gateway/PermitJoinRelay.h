#ifndef BLINKER_PROTOCOL_GATEWAY_PERMITJOINRELAY_H
#define BLINKER_PROTOCOL_GATEWAY_PERMITJOINRELAY_H

#include "Contracts.h"

namespace blinker {
namespace gateway {

enum : size_t {
    kPermitJoinCandidateTokenMaximumSize = 16U,
    kPermitJoinCandidateMaximumCount = 4U,
    kPermitJoinRelayPacketMaximumSize = 20U,
    kPermitJoinRelayBatchMaximumPackets = 16U,
    kPermitJoinCandidateSnapshotMaximumEncodedSize = 125U,
    kPermitJoinSelectCommandMaximumEncodedSize = 37U,
    kPermitJoinSelectResultMaximumEncodedSize = 41U,
    kPermitJoinRelayBatchMaximumEncodedSize = 363U,
    kPermitJoinRelayAckMaximumEncodedSize = 32U
};

enum class PermitJoinRelayDirection : uint8_t {
    Down = 1U,
    Up = 2U
};

enum class PermitJoinSelectStatus : uint8_t {
    Connecting = 1U,
    Connected = 2U,
    NotFound = 3U,
    Disconnected = 4U,
    Rejected = 5U
};

enum class PermitJoinRelayAckStatus : uint8_t {
    Accepted = 1U,
    Rejected = 2U
};

enum class PermitJoinRelayMessageKind : uint8_t {
    Unknown = 0U,
    CandidateSnapshot = 7U,
    SelectCommand = 8U,
    SelectResult = 9U,
    RelayBatch = 10U,
    RelayAck = 11U
};

struct PermitJoinCandidateView {
    ByteView token;
    uint8_t wireVersion;
    uint16_t capabilities;
    uint8_t signalQuality;

    PermitJoinCandidateView();
};

struct PermitJoinCandidateSnapshotView {
    uint8_t version;
    ByteView operationId;
    uint32_t revision;
    PermitJoinCandidateView candidates[kPermitJoinCandidateMaximumCount];
    size_t candidateCount;

    PermitJoinCandidateSnapshotView();
};

struct PermitJoinSelectCommandView {
    uint8_t version;
    ByteView operationId;
    ByteView candidateToken;

    PermitJoinSelectCommandView();
};

struct PermitJoinSelectResultView {
    uint8_t version;
    ByteView operationId;
    ByteView candidateToken;
    PermitJoinSelectStatus status;
    uint16_t maxPacketSize;

    PermitJoinSelectResultView();
};

struct PermitJoinRelayBatchView {
    uint8_t version;
    ByteView operationId;
    PermitJoinRelayDirection direction;
    uint32_t sequence;
    ByteView packets[kPermitJoinRelayBatchMaximumPackets];
    size_t packetCount;

    PermitJoinRelayBatchView();
};

struct PermitJoinRelayAckView {
    uint8_t version;
    ByteView operationId;
    PermitJoinRelayDirection direction;
    uint32_t sequence;
    PermitJoinRelayAckStatus status;

    PermitJoinRelayAckView();
};

Result encodePermitJoinCandidateSnapshot(
    const PermitJoinCandidateSnapshotView& value,
    MutableByteSpan output,
    size_t& written);
Result decodePermitJoinCandidateSnapshot(
    ByteView encoded,
    PermitJoinCandidateSnapshotView& value);
Result encodePermitJoinSelectCommand(
    const PermitJoinSelectCommandView& value,
    MutableByteSpan output,
    size_t& written);
Result decodePermitJoinSelectCommand(
    ByteView encoded,
    PermitJoinSelectCommandView& value);
Result encodePermitJoinSelectResult(
    const PermitJoinSelectResultView& value,
    MutableByteSpan output,
    size_t& written);
Result decodePermitJoinSelectResult(
    ByteView encoded,
    PermitJoinSelectResultView& value);
Result encodePermitJoinRelayBatch(
    const PermitJoinRelayBatchView& value,
    MutableByteSpan output,
    size_t& written);
Result decodePermitJoinRelayBatch(
    ByteView encoded,
    PermitJoinRelayBatchView& value);
Result encodePermitJoinRelayAck(
    const PermitJoinRelayAckView& value,
    MutableByteSpan output,
    size_t& written);
Result decodePermitJoinRelayAck(
    ByteView encoded,
    PermitJoinRelayAckView& value);
PermitJoinRelayMessageKind permitJoinRelayMessageKind(ByteView encoded);

} // namespace gateway
} // namespace blinker

#endif
