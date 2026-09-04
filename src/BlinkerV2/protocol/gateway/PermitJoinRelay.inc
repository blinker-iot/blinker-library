#include "PermitJoinRelay.h"

#include "../cbor/Cbor.h"

namespace blinker {
namespace gateway {
namespace permit_join_relay_detail {

const uint8_t kContractVersion = 1U;
const uint8_t kCandidateSnapshotKind = 7U;
const uint8_t kSelectCommandKind = 8U;
const uint8_t kSelectResultKind = 9U;
const uint8_t kRelayBatchKind = 10U;
const uint8_t kRelayAckKind = 11U;

bool exactNonZero(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validToken(ByteView value) {
    if (value.data == nullptr || value.empty() ||
        value.size > kPermitJoinCandidateTokenMaximumSize) {
        return false;
    }
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validDirection(PermitJoinRelayDirection value) {
    return value == PermitJoinRelayDirection::Down ||
           value == PermitJoinRelayDirection::Up;
}

Result relayInvalidArgument() {
    return Result::failure(ErrorCode::InvalidArgument);
}

Result relayInvalidEncoding() {
    return Result::failure(ErrorCode::InvalidEncoding);
}

Result decodeHeader(
    cbor::Reader& reader,
    size_t expectedCount,
    uint8_t expectedKind) {
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != expectedCount) result = relayInvalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kContractVersion) result = relayInvalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != expectedKind) result = relayInvalidEncoding();
    return result;
}

Result readOperationId(cbor::Reader& reader, ByteView& output) {
    Result result = reader.readBytes(output);
    return result && exactNonZero(output, kOperationIdSize)
               ? result
               : relayInvalidEncoding();
}

Result readToken(cbor::Reader& reader, ByteView& output) {
    Result result = reader.readBytes(output);
    return result && validToken(output) ? result : relayInvalidEncoding();
}

Result readRelayUnsigned8(cbor::Reader& reader, uint8_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > 0xFFU) result = relayInvalidEncoding();
    if (result) output = static_cast<uint8_t>(value);
    return result;
}

Result readRelayUnsigned16(cbor::Reader& reader, uint16_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > 0xFFFFU) result = relayInvalidEncoding();
    if (result) output = static_cast<uint16_t>(value);
    return result;
}

Result readRelayUnsigned32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > 0xFFFFFFFFULL) result = relayInvalidEncoding();
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

cbor::Limits limits(size_t maximumBytes, size_t items, uint8_t depth) {
    cbor::Limits value;
    value.maxByteStringLength = maximumBytes;
    value.maxContainerItems = items;
    value.maxDepth = depth;
    return value;
}

Result finish(cbor::Reader& reader, Result result) {
    if (result && !reader.finished()) {
        return Result::failure(ErrorCode::TrailingData);
    }
    return result;
}

} // namespace permit_join_relay_detail

PermitJoinCandidateView::PermitJoinCandidateView()
    : token(), wireVersion(0U), capabilities(0U), signalQuality(0U) {}

PermitJoinCandidateSnapshotView::PermitJoinCandidateSnapshotView()
    : version(permit_join_relay_detail::kContractVersion), operationId(),
      revision(0U), candidates(), candidateCount(0U) {}

PermitJoinSelectCommandView::PermitJoinSelectCommandView()
    : version(permit_join_relay_detail::kContractVersion), operationId(),
      candidateToken() {}

PermitJoinSelectResultView::PermitJoinSelectResultView()
    : version(permit_join_relay_detail::kContractVersion), operationId(),
      candidateToken(), status(PermitJoinSelectStatus::Rejected),
      maxPacketSize(0U) {}

PermitJoinRelayBatchView::PermitJoinRelayBatchView()
    : version(permit_join_relay_detail::kContractVersion), operationId(),
      direction(PermitJoinRelayDirection::Down), sequence(0U), packets(),
      packetCount(0U) {}

PermitJoinRelayAckView::PermitJoinRelayAckView()
    : version(permit_join_relay_detail::kContractVersion), operationId(),
      direction(PermitJoinRelayDirection::Down), sequence(0U),
      status(PermitJoinRelayAckStatus::Rejected) {}

PermitJoinRelayMessageKind permitJoinRelayMessageKind(ByteView encoded) {
    using namespace permit_join_relay_detail;
    cbor::Reader reader(encoded, limits(0U, 6U, 1U));
    size_t count = 0U;
    uint64_t version = 0U;
    uint64_t kind = 0U;
    Result result = reader.readArraySize(count);
    if (result) result = reader.readUnsigned(version);
    if (result) result = reader.readUnsigned(kind);
    if (!result || version != kContractVersion || count < 4U || count > 6U ||
        kind < kCandidateSnapshotKind || kind > kRelayAckKind) {
        return PermitJoinRelayMessageKind::Unknown;
    }
    return static_cast<PermitJoinRelayMessageKind>(kind);
}

Result encodePermitJoinCandidateSnapshot(
    const PermitJoinCandidateSnapshotView& value,
    MutableByteSpan output,
    size_t& written) {
    using namespace permit_join_relay_detail;
    written = 0U;
    if (value.version != kContractVersion ||
        !exactNonZero(value.operationId, kOperationIdSize) ||
        value.revision == 0U ||
        value.candidateCount > kPermitJoinCandidateMaximumCount) {
        return relayInvalidArgument();
    }
    for (size_t index = 0U; index < value.candidateCount; ++index) {
        const PermitJoinCandidateView& candidate = value.candidates[index];
        if (!validToken(candidate.token) || candidate.wireVersion == 0U ||
            candidate.signalQuality > 100U) {
            return relayInvalidArgument();
        }
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(5U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kCandidateSnapshotKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.revision);
    if (result) result = writer.beginArray(value.candidateCount);
    for (size_t index = 0U; result && index < value.candidateCount; ++index) {
        const PermitJoinCandidateView& candidate = value.candidates[index];
        result = writer.beginArray(4U);
        if (result) result = writer.writeBytes(candidate.token);
        if (result) result = writer.writeUnsigned(candidate.wireVersion);
        if (result) result = writer.writeUnsigned(candidate.capabilities);
        if (result) result = writer.writeUnsigned(candidate.signalQuality);
    }
    if (result) written = writer.size();
    return result;
}

Result decodePermitJoinCandidateSnapshot(
    ByteView encoded,
    PermitJoinCandidateSnapshotView& value) {
    using namespace permit_join_relay_detail;
    value = PermitJoinCandidateSnapshotView();
    if (encoded.size > kPermitJoinCandidateSnapshotMaximumEncodedSize) {
        return relayInvalidEncoding();
    }
    cbor::Reader reader(
        encoded,
        limits(kPermitJoinCandidateTokenMaximumSize, 5U, 2U));
    Result result = decodeHeader(reader, 5U, kCandidateSnapshotKind);
    if (result) result = readOperationId(reader, value.operationId);
    if (result) result = readRelayUnsigned32(reader, value.revision);
    size_t count = 0U;
    if (result) result = reader.readArraySize(count);
    if (result && count > kPermitJoinCandidateMaximumCount) {
        result = relayInvalidEncoding();
    }
    for (size_t index = 0U; result && index < count; ++index) {
        size_t fields = 0U;
        result = reader.readArraySize(fields);
        if (result && fields != 4U) result = relayInvalidEncoding();
        PermitJoinCandidateView& candidate = value.candidates[index];
        if (result) result = readToken(reader, candidate.token);
        if (result) result = readRelayUnsigned8(reader, candidate.wireVersion);
        if (result) result = readRelayUnsigned16(reader, candidate.capabilities);
        if (result) result = readRelayUnsigned8(reader, candidate.signalQuality);
        if (result &&
            (candidate.wireVersion == 0U || candidate.signalQuality > 100U)) {
            result = relayInvalidEncoding();
        }
    }
    if (result && value.revision == 0U) result = relayInvalidEncoding();
    if (result) value.candidateCount = count;
    return finish(reader, result);
}

Result encodePermitJoinSelectCommand(
    const PermitJoinSelectCommandView& value,
    MutableByteSpan output,
    size_t& written) {
    using namespace permit_join_relay_detail;
    written = 0U;
    if (value.version != kContractVersion ||
        !exactNonZero(value.operationId, kOperationIdSize) ||
        !validToken(value.candidateToken)) {
        return relayInvalidArgument();
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(4U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kSelectCommandKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeBytes(value.candidateToken);
    if (result) written = writer.size();
    return result;
}

Result decodePermitJoinSelectCommand(
    ByteView encoded,
    PermitJoinSelectCommandView& value) {
    using namespace permit_join_relay_detail;
    value = PermitJoinSelectCommandView();
    if (encoded.size > kPermitJoinSelectCommandMaximumEncodedSize) {
        return relayInvalidEncoding();
    }
    cbor::Reader reader(
        encoded,
        limits(kPermitJoinCandidateTokenMaximumSize, 4U, 1U));
    Result result = decodeHeader(reader, 4U, kSelectCommandKind);
    if (result) result = readOperationId(reader, value.operationId);
    if (result) result = readToken(reader, value.candidateToken);
    return finish(reader, result);
}

Result encodePermitJoinSelectResult(
    const PermitJoinSelectResultView& value,
    MutableByteSpan output,
    size_t& written) {
    using namespace permit_join_relay_detail;
    written = 0U;
    const uint8_t status = static_cast<uint8_t>(value.status);
    const bool connected = value.status == PermitJoinSelectStatus::Connected;
    if (value.version != kContractVersion ||
        !exactNonZero(value.operationId, kOperationIdSize) ||
        !validToken(value.candidateToken) || status < 1U || status > 5U ||
        (connected && (value.maxPacketSize == 0U ||
                       value.maxPacketSize > kPermitJoinRelayPacketMaximumSize)) ||
        (!connected && value.maxPacketSize != 0U)) {
        return relayInvalidArgument();
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kSelectResultKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeBytes(value.candidateToken);
    if (result) result = writer.writeUnsigned(status);
    if (result) result = writer.writeUnsigned(value.maxPacketSize);
    if (result) written = writer.size();
    return result;
}

Result decodePermitJoinSelectResult(
    ByteView encoded,
    PermitJoinSelectResultView& value) {
    using namespace permit_join_relay_detail;
    value = PermitJoinSelectResultView();
    if (encoded.size > kPermitJoinSelectResultMaximumEncodedSize) {
        return relayInvalidEncoding();
    }
    cbor::Reader reader(
        encoded,
        limits(kPermitJoinCandidateTokenMaximumSize, 6U, 1U));
    Result result = decodeHeader(reader, 6U, kSelectResultKind);
    if (result) result = readOperationId(reader, value.operationId);
    if (result) result = readToken(reader, value.candidateToken);
    uint8_t status = 0U;
    if (result) result = readRelayUnsigned8(reader, status);
    if (result && (status < 1U || status > 5U)) {
        result = relayInvalidEncoding();
    }
    if (result) value.status = static_cast<PermitJoinSelectStatus>(status);
    if (result) result = readRelayUnsigned16(reader, value.maxPacketSize);
    const bool connected = value.status == PermitJoinSelectStatus::Connected;
    if (result &&
        ((connected && (value.maxPacketSize == 0U ||
                        value.maxPacketSize > kPermitJoinRelayPacketMaximumSize)) ||
         (!connected && value.maxPacketSize != 0U))) {
        result = relayInvalidEncoding();
    }
    return finish(reader, result);
}

Result encodePermitJoinRelayBatch(
    const PermitJoinRelayBatchView& value,
    MutableByteSpan output,
    size_t& written) {
    using namespace permit_join_relay_detail;
    written = 0U;
    if (value.version != kContractVersion ||
        !exactNonZero(value.operationId, kOperationIdSize) ||
        !validDirection(value.direction) || value.sequence == 0U ||
        value.packetCount == 0U ||
        value.packetCount > kPermitJoinRelayBatchMaximumPackets) {
        return relayInvalidArgument();
    }
    for (size_t index = 0U; index < value.packetCount; ++index) {
        if (value.packets[index].data == nullptr ||
            value.packets[index].empty() ||
            value.packets[index].size > kPermitJoinRelayPacketMaximumSize) {
            return relayInvalidArgument();
        }
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kRelayBatchKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.direction));
    }
    if (result) result = writer.writeUnsigned(value.sequence);
    if (result) result = writer.beginArray(value.packetCount);
    for (size_t index = 0U; result && index < value.packetCount; ++index) {
        result = writer.writeBytes(value.packets[index]);
    }
    if (result) written = writer.size();
    return result;
}

Result decodePermitJoinRelayBatch(
    ByteView encoded,
    PermitJoinRelayBatchView& value) {
    using namespace permit_join_relay_detail;
    value = PermitJoinRelayBatchView();
    if (encoded.size > kPermitJoinRelayBatchMaximumEncodedSize) {
        return relayInvalidEncoding();
    }
    cbor::Reader reader(
        encoded, limits(
            kPermitJoinRelayPacketMaximumSize,
            kPermitJoinRelayBatchMaximumPackets,
            2U));
    Result result = decodeHeader(reader, 6U, kRelayBatchKind);
    if (result) result = readOperationId(reader, value.operationId);
    uint8_t direction = 0U;
    if (result) result = readRelayUnsigned8(reader, direction);
    if (result) value.direction =
        static_cast<PermitJoinRelayDirection>(direction);
    if (result && !validDirection(value.direction)) {
        result = relayInvalidEncoding();
    }
    if (result) result = readRelayUnsigned32(reader, value.sequence);
    size_t packetCount = 0U;
    if (result) result = reader.readArraySize(packetCount);
    if (result && (value.sequence == 0U || packetCount == 0U ||
                   packetCount > kPermitJoinRelayBatchMaximumPackets)) {
        result = relayInvalidEncoding();
    }
    for (size_t index = 0U; result && index < packetCount; ++index) {
        result = reader.readBytes(value.packets[index]);
        if (result && (value.packets[index].empty() ||
                       value.packets[index].size >
                           kPermitJoinRelayPacketMaximumSize)) {
            result = relayInvalidEncoding();
        }
    }
    if (result) value.packetCount = packetCount;
    return finish(reader, result);
}

Result encodePermitJoinRelayAck(
    const PermitJoinRelayAckView& value,
    MutableByteSpan output,
    size_t& written) {
    using namespace permit_join_relay_detail;
    written = 0U;
    const uint8_t status = static_cast<uint8_t>(value.status);
    if (value.version != kContractVersion ||
        !exactNonZero(value.operationId, kOperationIdSize) ||
        !validDirection(value.direction) || value.sequence == 0U ||
        status < 1U || status > 2U) {
        return relayInvalidArgument();
    }
    cbor::Writer writer(output);
    Result result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kRelayAckKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.direction));
    }
    if (result) result = writer.writeUnsigned(value.sequence);
    if (result) result = writer.writeUnsigned(status);
    if (result) written = writer.size();
    return result;
}

Result decodePermitJoinRelayAck(
    ByteView encoded,
    PermitJoinRelayAckView& value) {
    using namespace permit_join_relay_detail;
    value = PermitJoinRelayAckView();
    if (encoded.size > kPermitJoinRelayAckMaximumEncodedSize) {
        return relayInvalidEncoding();
    }
    cbor::Reader reader(encoded, limits(kOperationIdSize, 6U, 1U));
    Result result = decodeHeader(reader, 6U, kRelayAckKind);
    if (result) result = readOperationId(reader, value.operationId);
    uint8_t direction = 0U;
    if (result) result = readRelayUnsigned8(reader, direction);
    if (result) value.direction =
        static_cast<PermitJoinRelayDirection>(direction);
    if (result && !validDirection(value.direction)) {
        result = relayInvalidEncoding();
    }
    if (result) result = readRelayUnsigned32(reader, value.sequence);
    uint8_t status = 0U;
    if (result) result = readRelayUnsigned8(reader, status);
    if (result && (value.sequence == 0U || status < 1U || status > 2U)) {
        result = relayInvalidEncoding();
    }
    if (result) value.status = static_cast<PermitJoinRelayAckStatus>(status);
    return finish(reader, result);
}

} // namespace gateway
} // namespace blinker
