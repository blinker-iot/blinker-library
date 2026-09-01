#include "Contracts.h"

#include <limits.h>

#include "../cbor/Cbor.h"

namespace blinker {
namespace gateway {

namespace {

const uint8_t kGatewayAccessContractVersion = 2U;
const uint8_t kChildRouteContractVersion = 1U;
const uint8_t kGatewayRouteAdmissionKind = 1U;
const uint8_t kGatewayProofContractVersion = 1U;
const uint8_t kGatewayProofCommandKind = 1U;
const uint8_t kGatewayProofResultKind = 2U;
const uint8_t kGatewayRevocationCommandKind = 3U;
const uint8_t kGatewayRevocationResultKind = 4U;

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool exactBytes(ByteView value, size_t size, bool requireNonZero) {
    return value.data != nullptr && value.size == size &&
           (!requireNonZero || nonZero(value));
}

bool emptyBytes(ByteView value) {
    return value.size == 0U;
}

bool validIdentifier(StringView value, size_t maximum) {
    if (value.data == nullptr || value.empty() || value.size > maximum) {
        return false;
    }
    for (size_t index = 0U; index < value.size; ++index) {
        const uint8_t character = static_cast<uint8_t>(value.data[index]);
        if (character < 0x21U || character > 0x7EU ||
            character == '/' || character == '\\' ||
            character == '#' || character == '+') {
            return false;
        }
    }
    return true;
}

Result invalidArgument() {
    return Result::failure(ErrorCode::InvalidArgument);
}

Result invalidEncoding() {
    return Result::failure(ErrorCode::InvalidEncoding);
}

Result normalizedDecodeResult(Result result) {
    return !result && result.code() == ErrorCode::InvalidArgument
               ? invalidEncoding()
               : result;
}

Result validateEnvelopeMetadata(
    const GatewayAccessEnvelopeView& value) {
    if (value.version != kGatewayAccessContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        !validIdentifier(value.environmentId, kMaximumEnvironmentIdSize) ||
        !validIdentifier(value.tenantId, kMaximumTenantIdSize) ||
        !validIdentifier(
            value.edgeHubLogicalDeviceId, kMaximumLogicalDeviceIdSize) ||
        value.deviceKeyVersion == 0U ||
        !validIdentifier(
            value.childLogicalDeviceId, kMaximumLogicalDeviceIdSize) ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U ||
        !exactBytes(value.controllerId, kControllerIdSize, true) ||
        value.credentialVersion == 0U ||
        value.permissions != kGatewayPermissions ||
        value.presenceKeyVersion == 0U ||
        value.expiresAtUnixSeconds == 0U ||
        !exactBytes(value.nonce, kEnvelopeNonceSize, false)) {
        return invalidArgument();
    }
    return Result::success();
}

Result writeEnvelopeMetadata(
    cbor::Writer& writer,
    const GatewayAccessEnvelopeView& value,
    size_t itemCount) {
    Result result = writer.beginArray(itemCount);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeText(value.environmentId);
    if (result) result = writer.writeText(value.tenantId);
    if (result) result = writer.writeText(value.edgeHubLogicalDeviceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeText(value.childLogicalDeviceId);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeBytes(value.controllerId);
    if (result) result = writer.writeUnsigned(value.credentialVersion);
    if (result) result = writer.writeUnsigned(value.permissions);
    if (result) result = writer.writeUnsigned(value.presenceKeyVersion);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) result = writer.writeBytes(value.nonce);
    return result;
}

Result readUnsigned32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (!result) return result;
    if (value == 0U || value > UINT32_MAX) return invalidEncoding();
    output = static_cast<uint32_t>(value);
    return Result::success();
}

Result readExactBytes(
    cbor::Reader& reader,
    size_t size,
    bool requireNonZero,
    ByteView& output) {
    Result result = reader.readBytes(output);
    return result && !exactBytes(output, size, requireNonZero)
               ? invalidEncoding()
               : result;
}

} // namespace

GatewayAccessEnvelopeView::GatewayAccessEnvelopeView()
    : version(kGatewayAccessContractVersion), operationId(), environmentId(), tenantId(),
      edgeHubLogicalDeviceId(), deviceKeyVersion(0U), childLogicalDeviceId(),
      childDeviceInstanceId(), accessEpoch(0U), controllerId(),
      credentialVersion(0U), permissions(kGatewayPermissions),
      presenceKeyVersion(0U),
      expiresAtUnixSeconds(0U), nonce(), ciphertext(), tag() {}

GatewayAccessDeliveryAckView::GatewayAccessDeliveryAckView()
    : version(kGatewayAccessContractVersion), operationId(),
      accessMaterialDigest(),
      storageRevision(0U) {}

ChildRouteEnvelopeView::ChildRouteEnvelopeView()
    : version(kChildRouteContractVersion), topologyVersion(0U), childLogicalDeviceId(),
      childDeviceInstanceId(), accessEpoch(0U), gatewayCredentialVersion(0U),
      correlationId(), innerBbp2Frame() {}

GatewayRouteAdmissionView::GatewayRouteAdmissionView()
    : version(kChildRouteContractVersion), operationId(),
      topologyVersion(0U), childLogicalDeviceId(),
      childDeviceInstanceId(), accessEpoch(0U),
      gatewayCredentialVersion(0U), expiresAtUnixSeconds(0U) {}

GatewayProofCommandView::GatewayProofCommandView()
    : version(kGatewayProofContractVersion), operationId(),
      topologyVersion(0U), childLogicalDeviceId(), childDeviceInstanceId(),
      accessEpoch(0U), gatewayCredentialVersion(0U), presenceKeyVersion(0U),
      accessMaterialDigest(), accessStorageRevision(0U), proofNonce(),
      expiresAtUnixSeconds(0U) {}

GatewayProofResultView::GatewayProofResultView()
    : version(kGatewayProofContractVersion), operationId(),
      topologyVersion(0U), proofNonce(), status(GatewayProofStatus::Secure),
      presenceKeyVersion(0U) {}

GatewayRevocationCommandView::GatewayRevocationCommandView()
    : version(kGatewayProofContractVersion), operationId(),
      topologyVersion(0U), childDeviceInstanceId(), accessEpoch(0U),
      controllerId(), gatewayCredentialVersion(0U),
      accessStorageRevision(0U),
      phase(GatewayRevocationPhase::Prepare), grant(), receiptDigest(),
      expiresAtUnixSeconds(0U) {}

GatewayRevocationResultView::GatewayRevocationResultView()
    : version(kGatewayProofContractVersion), operationId(),
      topologyVersion(0U), phase(GatewayRevocationPhase::Prepare),
      status(GatewayRevocationStatus::NonceReady), payload(),
      receiptDigest() {}

bool validTopologyState(TopologyState state) {
    const uint8_t value = static_cast<uint8_t>(state);
    return value >= static_cast<uint8_t>(TopologyState::Created) &&
           value <= static_cast<uint8_t>(TopologyState::Detached);
}

bool validTopologyTransition(TopologyState from, TopologyState to) {
    if (!validTopologyState(from) || !validTopologyState(to) || from == to) {
        return false;
    }
    switch (from) {
        case TopologyState::Created:
            return to == TopologyState::PendingAccessDelivery ||
                   to == TopologyState::Expired ||
                   to == TopologyState::Cancelled;
        case TopologyState::PendingAccessDelivery:
            return to == TopologyState::PendingChildInstall ||
                   to == TopologyState::Expired ||
                   to == TopologyState::Cancelled;
        case TopologyState::PendingChildInstall:
            return to == TopologyState::PendingGatewayProof ||
                   to == TopologyState::RollbackRequired;
        case TopologyState::PendingGatewayProof:
            return to == TopologyState::Active ||
                   to == TopologyState::RollbackRequired;
        case TopologyState::Active:
            return to == TopologyState::Revoking;
        case TopologyState::RollbackRequired:
            return to == TopologyState::Revoking;
        case TopologyState::Revoking:
            return to == TopologyState::Detached;
        default:
            return false;
    }
}

Result validateGatewayAccessEnvelope(
    const GatewayAccessEnvelopeView& value) {
    Result result = validateEnvelopeMetadata(value);
    if (!result ||
        !exactBytes(value.ciphertext, kGatewayAccessMaterialSize, false) ||
        !exactBytes(value.tag, kEnvelopeTagSize, false)) {
        return result ? invalidArgument() : result;
    }
    return Result::success();
}

Result encodeGatewayAccessEnvelope(
    const GatewayAccessEnvelopeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayAccessEnvelope(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writeEnvelopeMetadata(writer, value, 17U);
    if (result) result = writer.writeBytes(value.ciphertext);
    if (result) result = writer.writeBytes(value.tag);
    if (result) written = writer.size();
    return result;
}

Result encodeGatewayAccessAad(
    const GatewayAccessEnvelopeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateEnvelopeMetadata(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writeEnvelopeMetadata(writer, value, 15U);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayAccessEnvelope(
    ByteView encoded,
    GatewayAccessEnvelopeView& value) {
    value = GatewayAccessEnvelopeView();
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kGatewayAccessMaterialSize;
    limits.maxContainerItems = 17U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 17U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayAccessContractVersion) result = invalidEncoding();
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = reader.readText(value.environmentId);
    if (result) result = reader.readText(value.tenantId);
    if (result) result = reader.readText(value.edgeHubLogicalDeviceId);
    if (result) result = readUnsigned32(reader, value.deviceKeyVersion);
    if (result) result = reader.readText(value.childLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true, value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readExactBytes(
        reader, kControllerIdSize, true, value.controllerId);
    if (result) result = readUnsigned32(reader, value.credentialVersion);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result && decoded > UINT32_MAX) result = invalidEncoding();
        if (result) value.permissions = static_cast<uint32_t>(decoded);
    }
    if (result) result = readUnsigned32(reader, value.presenceKeyVersion);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result) result = readExactBytes(
        reader, kEnvelopeNonceSize, false, value.nonce);
    if (result) result = readExactBytes(
        reader, kGatewayAccessMaterialSize, false, value.ciphertext);
    if (result) result = readExactBytes(
        reader, kEnvelopeTagSize, false, value.tag);
    if (result && !reader.finished()) result = Result::failure(ErrorCode::TrailingData);
    if (result) result = validateGatewayAccessEnvelope(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayAccessDeliveryAck(
    const GatewayAccessDeliveryAckView& value) {
    if (value.version != kGatewayAccessContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        !exactBytes(
            value.accessMaterialDigest,
            kAccessMaterialDigestSize,
            false) ||
        value.storageRevision == 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayAccessDeliveryAck(
    const GatewayAccessDeliveryAckView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayAccessDeliveryAck(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(4U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) result = writer.writeUnsigned(value.storageRevision);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayAccessDeliveryAck(
    ByteView encoded,
    GatewayAccessDeliveryAckView& value) {
    value = GatewayAccessDeliveryAckView();
    cbor::Limits limits;
    limits.maxByteStringLength = kAccessMaterialDigestSize;
    limits.maxContainerItems = 4U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 4U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayAccessContractVersion) result = invalidEncoding();
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) result = readUnsigned32(reader, value.storageRevision);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayAccessDeliveryAck(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayProofCommand(const GatewayProofCommandView& value) {
    if (value.version != kGatewayProofContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        !validIdentifier(
            value.childLogicalDeviceId, kMaximumLogicalDeviceIdSize) ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U || value.gatewayCredentialVersion == 0U ||
        value.presenceKeyVersion == 0U ||
        !exactBytes(
            value.accessMaterialDigest, kAccessMaterialDigestSize, false) ||
        value.accessStorageRevision == 0U ||
        !exactBytes(value.proofNonce, kProofNonceSize, true) ||
        value.expiresAtUnixSeconds == 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayProofCommand(
    const GatewayProofCommandView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayProofCommand(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(13U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayProofCommandKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeText(value.childLogicalDeviceId);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeUnsigned(value.gatewayCredentialVersion);
    if (result) result = writer.writeUnsigned(value.presenceKeyVersion);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) result = writer.writeUnsigned(value.accessStorageRevision);
    if (result) result = writer.writeBytes(value.proofNonce);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayProofCommand(
    ByteView encoded,
    GatewayProofCommandView& value) {
    value = GatewayProofCommandView();
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kAccessMaterialDigestSize;
    limits.maxContainerItems = 13U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 13U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofCommandKind) result = invalidEncoding();
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = reader.readText(value.childLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true, value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readUnsigned32(
        reader, value.gatewayCredentialVersion);
    if (result) result = readUnsigned32(reader, value.presenceKeyVersion);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) result = readUnsigned32(
        reader, value.accessStorageRevision);
    if (result) result = readExactBytes(
        reader, kProofNonceSize, true, value.proofNonce);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayProofCommand(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayProofResult(const GatewayProofResultView& value) {
    const uint8_t status = static_cast<uint8_t>(value.status);
    if (value.version != kGatewayProofContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        !exactBytes(value.proofNonce, kProofNonceSize, true) ||
        status < static_cast<uint8_t>(GatewayProofStatus::Secure) ||
        status > static_cast<uint8_t>(GatewayProofStatus::Expired) ||
        value.presenceKeyVersion == 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayProofResult(
    const GatewayProofResultView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayProofResult(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(7U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayProofResultKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeBytes(value.proofNonce);
    if (result) result = writer.writeUnsigned(
        static_cast<uint8_t>(value.status));
    if (result) result = writer.writeUnsigned(
        value.presenceKeyVersion);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayProofResult(
    ByteView encoded,
    GatewayProofResultView& value) {
    value = GatewayProofResultView();
    cbor::Limits limits;
    limits.maxByteStringLength = kProofNonceSize;
    limits.maxContainerItems = 7U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 7U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofResultKind) result = invalidEncoding();
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = readExactBytes(
        reader, kProofNonceSize, true, value.proofNonce);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(GatewayProofStatus::Secure) ||
             decoded > static_cast<uint8_t>(GatewayProofStatus::Expired))) {
            result = invalidEncoding();
        }
        if (result) value.status = static_cast<GatewayProofStatus>(decoded);
    }
    if (result) result = readUnsigned32(
        reader, value.presenceKeyVersion);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayProofResult(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayRevocationCommand(
    const GatewayRevocationCommandView& value) {
    const uint8_t phase = static_cast<uint8_t>(value.phase);
    if (value.version != kGatewayProofContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U ||
        !exactBytes(value.controllerId, kControllerIdSize, true) ||
        value.gatewayCredentialVersion == 0U ||
        value.accessStorageRevision == 0U ||
        phase < static_cast<uint8_t>(GatewayRevocationPhase::Prepare) ||
        phase > static_cast<uint8_t>(GatewayRevocationPhase::Finalize)) {
        return invalidArgument();
    }
    if (value.phase == GatewayRevocationPhase::Prepare) {
        if (!emptyBytes(value.grant) || !emptyBytes(value.receiptDigest) ||
            value.expiresAtUnixSeconds == 0U) {
            return invalidArgument();
        }
    } else if (value.phase == GatewayRevocationPhase::Apply) {
        if (value.grant.data == nullptr || value.grant.empty() ||
            value.grant.size > kGatewayRevocationGrantMaximumSize ||
            !emptyBytes(value.receiptDigest) ||
            value.expiresAtUnixSeconds == 0U) {
            return invalidArgument();
        }
    } else if (!emptyBytes(value.grant) ||
               !exactBytes(
                   value.receiptDigest,
                   kAccessMaterialDigestSize,
                   true) ||
               value.expiresAtUnixSeconds != 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayRevocationCommand(
    const GatewayRevocationCommandView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayRevocationCommand(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(13U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayRevocationCommandKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeBytes(value.controllerId);
    if (result) result = writer.writeUnsigned(value.gatewayCredentialVersion);
    if (result) result = writer.writeUnsigned(value.accessStorageRevision);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.phase));
    }
    if (result) result = writer.writeBytes(value.grant);
    if (result) result = writer.writeBytes(value.receiptDigest);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result && writer.size() >
                      kGatewayRevocationCommandMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayRevocationCommand(
    ByteView encoded,
    GatewayRevocationCommandView& value) {
    value = GatewayRevocationCommandView();
    if (encoded.size > kGatewayRevocationCommandMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kGatewayRevocationGrantMaximumSize;
    limits.maxContainerItems = 13U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 13U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayRevocationCommandKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true,
        value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readExactBytes(
        reader, kControllerIdSize, true, value.controllerId);
    if (result) result = readUnsigned32(
        reader, value.gatewayCredentialVersion);
    if (result) result = readUnsigned32(
        reader, value.accessStorageRevision);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayRevocationPhase::Prepare) ||
             decoded > static_cast<uint8_t>(
                           GatewayRevocationPhase::Finalize))) {
            result = invalidEncoding();
        }
        if (result) value.phase =
            static_cast<GatewayRevocationPhase>(decoded);
    }
    if (result) result = reader.readBytes(value.grant);
    if (result) result = reader.readBytes(value.receiptDigest);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayRevocationCommand(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayRevocationResult(
    const GatewayRevocationResultView& value) {
    const uint8_t phase = static_cast<uint8_t>(value.phase);
    const uint8_t status = static_cast<uint8_t>(value.status);
    if (value.version != kGatewayProofContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        phase < static_cast<uint8_t>(GatewayRevocationPhase::Prepare) ||
        phase > static_cast<uint8_t>(GatewayRevocationPhase::Finalize) ||
        status < static_cast<uint8_t>(GatewayRevocationStatus::NonceReady) ||
        status > static_cast<uint8_t>(GatewayRevocationStatus::Expired)) {
        return invalidArgument();
    }
    if (value.status == GatewayRevocationStatus::NonceReady) {
        if (value.phase != GatewayRevocationPhase::Prepare ||
            !exactBytes(value.payload, kProofNonceSize, true) ||
            !emptyBytes(value.receiptDigest)) {
            return invalidArgument();
        }
    } else if (value.status == GatewayRevocationStatus::Revoked) {
        if (value.phase != GatewayRevocationPhase::Apply ||
            value.payload.data == nullptr || value.payload.empty() ||
            value.payload.size > kGatewayRevocationReceiptMaximumSize ||
            !exactBytes(
                value.receiptDigest,
                kAccessMaterialDigestSize,
                true)) {
            return invalidArgument();
        }
    } else if (value.status == GatewayRevocationStatus::Finalized) {
        if (value.phase != GatewayRevocationPhase::Finalize ||
            !emptyBytes(value.payload) ||
            !exactBytes(
                value.receiptDigest,
                kAccessMaterialDigestSize,
                true)) {
            return invalidArgument();
        }
    } else if (!emptyBytes(value.payload) ||
               !emptyBytes(value.receiptDigest)) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayRevocationResult(
    const GatewayRevocationResultView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayRevocationResult(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(8U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayRevocationResultKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.phase));
    }
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.status));
    }
    if (result) result = writer.writeBytes(value.payload);
    if (result) result = writer.writeBytes(value.receiptDigest);
    if (result && writer.size() >
                      kGatewayRevocationResultMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayRevocationResult(
    ByteView encoded,
    GatewayRevocationResultView& value) {
    value = GatewayRevocationResultView();
    if (encoded.size > kGatewayRevocationResultMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kGatewayRevocationReceiptMaximumSize;
    limits.maxContainerItems = 8U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 8U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayProofContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayRevocationResultKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayRevocationPhase::Prepare) ||
             decoded > static_cast<uint8_t>(
                           GatewayRevocationPhase::Finalize))) {
            result = invalidEncoding();
        }
        if (result) value.phase =
            static_cast<GatewayRevocationPhase>(decoded);
    }
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayRevocationStatus::NonceReady) ||
             decoded > static_cast<uint8_t>(
                           GatewayRevocationStatus::Expired))) {
            result = invalidEncoding();
        }
        if (result) value.status =
            static_cast<GatewayRevocationStatus>(decoded);
    }
    if (result) result = reader.readBytes(value.payload);
    if (result) result = reader.readBytes(value.receiptDigest);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayRevocationResult(value);
    return normalizedDecodeResult(result);
}

Result validateChildRouteEnvelope(const ChildRouteEnvelopeView& value) {
    if (value.version != kChildRouteContractVersion || value.topologyVersion == 0U ||
        !validIdentifier(
            value.childLogicalDeviceId, kMaximumLogicalDeviceIdSize) ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U || value.gatewayCredentialVersion == 0U ||
        !exactBytes(value.correlationId, kCorrelationIdSize, true) ||
        value.innerBbp2Frame.data == nullptr || value.innerBbp2Frame.empty() ||
        value.innerBbp2Frame.size > kMaximumInnerFrameSize) {
        return invalidArgument();
    }
    return Result::success();
}

Result validateGatewayRouteAdmission(
    const GatewayRouteAdmissionView& value) {
    if (value.version != kChildRouteContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        !validIdentifier(
            value.childLogicalDeviceId, kMaximumLogicalDeviceIdSize) ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U ||
        value.gatewayCredentialVersion == 0U ||
        value.expiresAtUnixSeconds == 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayRouteAdmission(
    const GatewayRouteAdmissionView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayRouteAdmission(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(9U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayRouteAdmissionKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeText(value.childLogicalDeviceId);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeUnsigned(
        value.gatewayCredentialVersion);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayRouteAdmission(
    ByteView encoded,
    GatewayRouteAdmissionView& value) {
    value = GatewayRouteAdmissionView();
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kDeviceInstanceIdSize;
    limits.maxContainerItems = 9U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 9U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kChildRouteContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayRouteAdmissionKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = reader.readText(value.childLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true,
        value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readUnsigned32(
        reader, value.gatewayCredentialVersion);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayRouteAdmission(value);
    return normalizedDecodeResult(result);
}

Result encodeChildRouteEnvelope(
    const ChildRouteEnvelopeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateChildRouteEnvelope(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(8U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeText(value.childLogicalDeviceId);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeUnsigned(value.gatewayCredentialVersion);
    if (result) result = writer.writeBytes(value.correlationId);
    if (result) result = writer.writeBytes(value.innerBbp2Frame);
    if (result) written = writer.size();
    return result;
}

Result decodeChildRouteEnvelope(
    ByteView encoded,
    ChildRouteEnvelopeView& value) {
    value = ChildRouteEnvelopeView();
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kMaximumInnerFrameSize;
    limits.maxContainerItems = 8U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 8U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kChildRouteContractVersion) result = invalidEncoding();
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = reader.readText(value.childLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true, value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readUnsigned32(
        reader, value.gatewayCredentialVersion);
    if (result) result = readExactBytes(
        reader, kCorrelationIdSize, true, value.correlationId);
    if (result) result = reader.readBytes(value.innerBbp2Frame);
    if (result && !reader.finished()) result = Result::failure(ErrorCode::TrailingData);
    if (result) result = validateChildRouteEnvelope(value);
    return normalizedDecodeResult(result);
}

} // namespace gateway
} // namespace blinker
