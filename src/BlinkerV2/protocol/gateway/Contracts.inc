#include "Contracts.h"

#include <limits.h>

#include "../cbor/Cbor.h"

namespace blinker {
namespace gateway {

namespace {

const uint8_t kGatewayAccessContractVersion = 2U;
const uint8_t kChildRouteContractVersion = 1U;
const uint8_t kGatewayRouteAdmissionKind = 1U;
const uint8_t kGatewayManagementContractVersion = 1U;
const uint8_t kGatewayProofCommandKind = 1U;
const uint8_t kGatewayProofResultKind = 2U;
const uint8_t kGatewayRevocationCommandKind = 3U;
const uint8_t kGatewayRevocationResultKind = 4U;
const uint8_t kGatewayPermitJoinCommandKind = 5U;
const uint8_t kGatewayPermitJoinResultKind = 6U;
// Management kinds 7..11 are the permit-join discovery/relay family.
const uint8_t kGatewayCredentialRenewalCommandKind = 12U;
const uint8_t kGatewayCredentialRenewalResultKind = 13U;
const uint8_t kGatewayCredentialRenewalDeliveryAckKind = 14U;
const uint8_t kGatewayCredentialRenewalContractVersion = 1U;
const uint64_t kGatewayMaximumUnixSeconds = 9007199254740991ULL;

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

Result readOptionalUnsigned32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (!result) return result;
    if (value > UINT32_MAX) return invalidEncoding();
    output = static_cast<uint32_t>(value);
    return Result::success();
}

Result readUnsigned16(cbor::Reader& reader, uint16_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (!result) return result;
    if (value == 0U || value > UINT16_MAX) return invalidEncoding();
    output = static_cast<uint16_t>(value);
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

GatewayCredentialRenewalEnvelopeView::
GatewayCredentialRenewalEnvelopeView()
    : version(kGatewayCredentialRenewalContractVersion),
      renewalOperationId(), topologyOperationId(), environmentId(),
      tenantId(), edgeHubLogicalDeviceId(), deviceKeyVersion(0U),
      childLogicalDeviceId(), childDeviceInstanceId(), accessEpoch(0U),
      controllerId(), expectedCredentialVersion(0U), credentialVersion(0U),
      permissions(kGatewayPermissions), presenceKeyVersion(0U),
      accessMaterialDigest(), expiresAtUnixSeconds(0U), nonce(),
      ciphertext(), tag() {}

GatewayCredentialRenewalDeliveryAckView::
GatewayCredentialRenewalDeliveryAckView()
    : version(kGatewayCredentialRenewalContractVersion),
      renewalOperationId(), topologyOperationId(), accessMaterialDigest(),
      pendingStorageRevision(0U) {}

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
    : version(kGatewayManagementContractVersion), operationId(),
      topologyVersion(0U), childLogicalDeviceId(), childDeviceInstanceId(),
      accessEpoch(0U), gatewayCredentialVersion(0U), presenceKeyVersion(0U),
      accessMaterialDigest(), accessStorageRevision(0U), proofNonce(),
      expiresAtUnixSeconds(0U) {}

GatewayProofResultView::GatewayProofResultView()
    : version(kGatewayManagementContractVersion), operationId(),
      topologyVersion(0U), proofNonce(), status(GatewayProofStatus::Secure),
      presenceKeyVersion(0U) {}

GatewayRevocationCommandView::GatewayRevocationCommandView()
    : version(kGatewayManagementContractVersion), operationId(),
      topologyVersion(0U), childDeviceInstanceId(), accessEpoch(0U),
      controllerId(), gatewayCredentialVersion(0U),
      accessStorageRevision(0U),
      phase(GatewayRevocationPhase::Prepare), grant(), receiptDigest(),
      expiresAtUnixSeconds(0U) {}

GatewayRevocationResultView::GatewayRevocationResultView()
    : version(kGatewayManagementContractVersion), operationId(),
      topologyVersion(0U), phase(GatewayRevocationPhase::Prepare),
      status(GatewayRevocationStatus::NonceReady), payload(),
      receiptDigest() {}

GatewayCredentialRenewalCommandView::
GatewayCredentialRenewalCommandView()
    : version(kGatewayManagementContractVersion), renewalOperationId(),
      topologyOperationId(), topologyVersion(0U), childDeviceInstanceId(),
      accessEpoch(0U), controllerId(), expectedCredentialVersion(0U),
      credentialVersion(0U), pendingStorageRevision(0U),
      activeStorageRevision(0U), accessMaterialDigest(),
      phase(GatewayCredentialRenewalPhase::Prepare), grant(),
      receiptDigest(), expiresAtUnixSeconds(0U) {}

GatewayCredentialRenewalResultView::
GatewayCredentialRenewalResultView()
    : version(kGatewayManagementContractVersion), renewalOperationId(),
      topologyOperationId(), topologyVersion(0U),
      phase(GatewayCredentialRenewalPhase::Prepare),
      status(GatewayCredentialRenewalStatus::NonceReady),
      pendingStorageRevision(0U), activeStorageRevision(0U),
      credentialVersion(0U), accessMaterialDigest(), payload(),
      receiptDigest() {}

GatewayPermitJoinCommandView::GatewayPermitJoinCommandView()
    : version(kGatewayManagementContractVersion), operationId(),
      adapterId(0U), action(GatewayPermitJoinAction::Open),
      expiresAtUnixSeconds(0U) {}

GatewayPermitJoinResultView::GatewayPermitJoinResultView()
    : version(kGatewayManagementContractVersion), operationId(),
      adapterId(0U), status(GatewayPermitJoinStatus::Ready),
      expiresAtUnixSeconds(0U) {}

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

namespace {

Result validateRenewalEnvelopeMetadata(
    const GatewayCredentialRenewalEnvelopeView& value) {
    if (value.version != kGatewayCredentialRenewalContractVersion ||
        !exactBytes(value.renewalOperationId, kOperationIdSize, true) ||
        !exactBytes(value.topologyOperationId, kOperationIdSize, true) ||
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
        value.expectedCredentialVersion == 0U ||
        value.expectedCredentialVersion == UINT32_MAX ||
        value.credentialVersion != value.expectedCredentialVersion + 1U ||
        value.permissions != kGatewayPermissions ||
        value.presenceKeyVersion == 0U ||
        !exactBytes(
            value.accessMaterialDigest,
            kAccessMaterialDigestSize,
            false) ||
        value.expiresAtUnixSeconds == 0U ||
        value.expiresAtUnixSeconds > kGatewayMaximumUnixSeconds ||
        !exactBytes(value.nonce, kEnvelopeNonceSize, false)) {
        return invalidArgument();
    }
    return Result::success();
}

Result writeRenewalEnvelopeMetadata(
    cbor::Writer& writer,
    const GatewayCredentialRenewalEnvelopeView& value,
    size_t itemCount) {
    Result result = writer.beginArray(itemCount);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeBytes(value.renewalOperationId);
    if (result) result = writer.writeBytes(value.topologyOperationId);
    if (result) result = writer.writeText(value.environmentId);
    if (result) result = writer.writeText(value.tenantId);
    if (result) result = writer.writeText(value.edgeHubLogicalDeviceId);
    if (result) result = writer.writeUnsigned(value.deviceKeyVersion);
    if (result) result = writer.writeText(value.childLogicalDeviceId);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeBytes(value.controllerId);
    if (result) {
        result = writer.writeUnsigned(value.expectedCredentialVersion);
    }
    if (result) result = writer.writeUnsigned(value.credentialVersion);
    if (result) result = writer.writeUnsigned(value.permissions);
    if (result) result = writer.writeUnsigned(value.presenceKeyVersion);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) result = writer.writeBytes(value.nonce);
    return result;
}

} // namespace

Result validateGatewayCredentialRenewalEnvelope(
    const GatewayCredentialRenewalEnvelopeView& value) {
    Result result = validateRenewalEnvelopeMetadata(value);
    if (!result ||
        !exactBytes(value.ciphertext, kGatewaySecretSize, false) ||
        !exactBytes(value.tag, kEnvelopeTagSize, false)) {
        return result ? invalidArgument() : result;
    }
    return Result::success();
}

Result encodeGatewayCredentialRenewalEnvelope(
    const GatewayCredentialRenewalEnvelopeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayCredentialRenewalEnvelope(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writeRenewalEnvelopeMetadata(writer, value, 20U);
    if (result) result = writer.writeBytes(value.ciphertext);
    if (result) result = writer.writeBytes(value.tag);
    if (result && writer.size() >
                      kGatewayCredentialRenewalEnvelopeMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result encodeGatewayCredentialRenewalAad(
    const GatewayCredentialRenewalEnvelopeView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateRenewalEnvelopeMetadata(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writeRenewalEnvelopeMetadata(writer, value, 18U);
    if (result && writer.size() >
                      kGatewayCredentialRenewalAadMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayCredentialRenewalEnvelope(
    ByteView encoded,
    GatewayCredentialRenewalEnvelopeView& value) {
    value = GatewayCredentialRenewalEnvelopeView();
    if (encoded.size >
        kGatewayCredentialRenewalEnvelopeMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxTextLength = kMaximumLogicalDeviceIdSize;
    limits.maxByteStringLength = kAccessMaterialDigestSize;
    limits.maxContainerItems = 20U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 20U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayCredentialRenewalContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.renewalOperationId);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.topologyOperationId);
    if (result) result = reader.readText(value.environmentId);
    if (result) result = reader.readText(value.tenantId);
    if (result) result = reader.readText(value.edgeHubLogicalDeviceId);
    if (result) result = readUnsigned32(reader, value.deviceKeyVersion);
    if (result) result = reader.readText(value.childLogicalDeviceId);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true,
        value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readExactBytes(
        reader, kControllerIdSize, true, value.controllerId);
    if (result) result = readUnsigned32(
        reader, value.expectedCredentialVersion);
    if (result) result = readUnsigned32(reader, value.credentialVersion);
    if (result) result = readOptionalUnsigned32(reader, value.permissions);
    if (result) result = readUnsigned32(reader, value.presenceKeyVersion);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result) result = readExactBytes(
        reader, kEnvelopeNonceSize, false, value.nonce);
    if (result) result = readExactBytes(
        reader, kGatewaySecretSize, false, value.ciphertext);
    if (result) result = readExactBytes(
        reader, kEnvelopeTagSize, false, value.tag);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayCredentialRenewalEnvelope(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayCredentialRenewalDeliveryAck(
    const GatewayCredentialRenewalDeliveryAckView& value) {
    if (value.version != kGatewayCredentialRenewalContractVersion ||
        !exactBytes(value.renewalOperationId, kOperationIdSize, true) ||
        !exactBytes(value.topologyOperationId, kOperationIdSize, true) ||
        !exactBytes(
            value.accessMaterialDigest,
            kAccessMaterialDigestSize,
            false) ||
        value.pendingStorageRevision == 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayCredentialRenewalDeliveryAck(
    const GatewayCredentialRenewalDeliveryAckView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayCredentialRenewalDeliveryAck(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) {
        result = writer.writeUnsigned(
            kGatewayCredentialRenewalDeliveryAckKind);
    }
    if (result) result = writer.writeBytes(value.renewalOperationId);
    if (result) result = writer.writeBytes(value.topologyOperationId);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) result = writer.writeUnsigned(value.pendingStorageRevision);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayCredentialRenewalDeliveryAck(
    ByteView encoded,
    GatewayCredentialRenewalDeliveryAckView& value) {
    value = GatewayCredentialRenewalDeliveryAckView();
    if (encoded.size >
        kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kAccessMaterialDigestSize;
    limits.maxContainerItems = 6U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 6U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayCredentialRenewalContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayCredentialRenewalDeliveryAckKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.renewalOperationId);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.topologyOperationId);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) result = readUnsigned32(
        reader, value.pendingStorageRevision);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayCredentialRenewalDeliveryAck(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayProofCommand(const GatewayProofCommandView& value) {
    if (value.version != kGatewayManagementContractVersion ||
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
    if (result && decoded != kGatewayManagementContractVersion) {
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
    if (value.version != kGatewayManagementContractVersion ||
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
    if (result && decoded != kGatewayManagementContractVersion) {
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
    if (value.version != kGatewayManagementContractVersion ||
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
    if (result && decoded != kGatewayManagementContractVersion) {
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
    if (value.version != kGatewayManagementContractVersion ||
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
    if (result && decoded != kGatewayManagementContractVersion) {
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

Result validateGatewayCredentialRenewalCommand(
    const GatewayCredentialRenewalCommandView& value) {
    const uint8_t phase = static_cast<uint8_t>(value.phase);
    if (value.version != kGatewayManagementContractVersion ||
        !exactBytes(value.renewalOperationId, kOperationIdSize, true) ||
        !exactBytes(value.topologyOperationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        !exactBytes(
            value.childDeviceInstanceId, kDeviceInstanceIdSize, true) ||
        value.accessEpoch == 0U ||
        !exactBytes(value.controllerId, kControllerIdSize, true) ||
        value.expectedCredentialVersion == 0U ||
        value.expectedCredentialVersion == UINT32_MAX ||
        value.credentialVersion != value.expectedCredentialVersion + 1U ||
        value.pendingStorageRevision == 0U ||
        !exactBytes(
            value.accessMaterialDigest,
            kAccessMaterialDigestSize,
            false) ||
        phase < static_cast<uint8_t>(
                    GatewayCredentialRenewalPhase::Prepare) ||
        phase > static_cast<uint8_t>(
                    GatewayCredentialRenewalPhase::Cancel)) {
        return invalidArgument();
    }
    const bool validDeadline = value.expiresAtUnixSeconds != 0U &&
        value.expiresAtUnixSeconds <= kGatewayMaximumUnixSeconds;
    if (value.phase == GatewayCredentialRenewalPhase::Prepare) {
        if (value.activeStorageRevision != 0U ||
            !emptyBytes(value.grant) || !emptyBytes(value.receiptDigest) ||
            !validDeadline) {
            return invalidArgument();
        }
    } else if (value.phase == GatewayCredentialRenewalPhase::Apply) {
        if (value.activeStorageRevision != 0U ||
            value.grant.data == nullptr || value.grant.empty() ||
            value.grant.size > kGatewayCredentialRenewalGrantMaximumSize ||
            !emptyBytes(value.receiptDigest) || !validDeadline) {
            return invalidArgument();
        }
    } else if (value.phase == GatewayCredentialRenewalPhase::Finalize) {
        if (value.activeStorageRevision == 0U ||
            !emptyBytes(value.grant) ||
            !exactBytes(
                value.receiptDigest,
                kAccessMaterialDigestSize,
                true) ||
            value.expiresAtUnixSeconds != 0U) {
            return invalidArgument();
        }
    } else if (value.activeStorageRevision != 0U ||
               !emptyBytes(value.grant) ||
               !emptyBytes(value.receiptDigest) ||
               value.expiresAtUnixSeconds != 0U) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayCredentialRenewalCommand(
    const GatewayCredentialRenewalCommandView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayCredentialRenewalCommand(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(17U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) {
        result = writer.writeUnsigned(kGatewayCredentialRenewalCommandKind);
    }
    if (result) result = writer.writeBytes(value.renewalOperationId);
    if (result) result = writer.writeBytes(value.topologyOperationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) result = writer.writeBytes(value.childDeviceInstanceId);
    if (result) result = writer.writeUnsigned(value.accessEpoch);
    if (result) result = writer.writeBytes(value.controllerId);
    if (result) {
        result = writer.writeUnsigned(value.expectedCredentialVersion);
    }
    if (result) result = writer.writeUnsigned(value.credentialVersion);
    if (result) result = writer.writeUnsigned(value.pendingStorageRevision);
    if (result) result = writer.writeUnsigned(value.activeStorageRevision);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.phase));
    }
    if (result) result = writer.writeBytes(value.grant);
    if (result) result = writer.writeBytes(value.receiptDigest);
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result && writer.size() >
                      kGatewayCredentialRenewalCommandMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayCredentialRenewalCommand(
    ByteView encoded,
    GatewayCredentialRenewalCommandView& value) {
    value = GatewayCredentialRenewalCommandView();
    if (encoded.size >
        kGatewayCredentialRenewalCommandMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kGatewayCredentialRenewalGrantMaximumSize;
    limits.maxContainerItems = 17U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 17U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayManagementContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayCredentialRenewalCommandKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.renewalOperationId);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.topologyOperationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) result = readExactBytes(
        reader, kDeviceInstanceIdSize, true,
        value.childDeviceInstanceId);
    if (result) result = readUnsigned32(reader, value.accessEpoch);
    if (result) result = readExactBytes(
        reader, kControllerIdSize, true, value.controllerId);
    if (result) result = readUnsigned32(
        reader, value.expectedCredentialVersion);
    if (result) result = readUnsigned32(reader, value.credentialVersion);
    if (result) result = readUnsigned32(
        reader, value.pendingStorageRevision);
    if (result) result = readOptionalUnsigned32(
        reader, value.activeStorageRevision);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayCredentialRenewalPhase::Prepare) ||
             decoded > static_cast<uint8_t>(
                           GatewayCredentialRenewalPhase::Cancel))) {
            result = invalidEncoding();
        }
        if (result) value.phase =
            static_cast<GatewayCredentialRenewalPhase>(decoded);
    }
    if (result) result = reader.readBytes(value.grant);
    if (result) result = reader.readBytes(value.receiptDigest);
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayCredentialRenewalCommand(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayCredentialRenewalResult(
    const GatewayCredentialRenewalResultView& value) {
    const uint8_t phase = static_cast<uint8_t>(value.phase);
    const uint8_t status = static_cast<uint8_t>(value.status);
    if (value.version != kGatewayManagementContractVersion ||
        !exactBytes(value.renewalOperationId, kOperationIdSize, true) ||
        !exactBytes(value.topologyOperationId, kOperationIdSize, true) ||
        value.topologyVersion == 0U ||
        value.pendingStorageRevision == 0U ||
        value.credentialVersion == 0U ||
        !exactBytes(
            value.accessMaterialDigest,
            kAccessMaterialDigestSize,
            false) ||
        phase < static_cast<uint8_t>(
                    GatewayCredentialRenewalPhase::Prepare) ||
        phase > static_cast<uint8_t>(
                    GatewayCredentialRenewalPhase::Cancel) ||
        status < static_cast<uint8_t>(
                     GatewayCredentialRenewalStatus::NonceReady) ||
        status > static_cast<uint8_t>(
                     GatewayCredentialRenewalStatus::Expired)) {
        return invalidArgument();
    }
    const bool promoted =
        value.status == GatewayCredentialRenewalStatus::Rotated;
    if ((value.phase == GatewayCredentialRenewalPhase::Prepare ||
         value.phase == GatewayCredentialRenewalPhase::Cancel) &&
        value.activeStorageRevision != 0U) {
        return invalidArgument();
    }
    if (value.phase == GatewayCredentialRenewalPhase::Finalize &&
        value.activeStorageRevision == 0U) {
        return invalidArgument();
    }
    if (value.phase == GatewayCredentialRenewalPhase::Apply &&
        (promoted ? value.activeStorageRevision == 0U
                  : value.activeStorageRevision != 0U)) {
        return invalidArgument();
    }
    if (value.status == GatewayCredentialRenewalStatus::NonceReady) {
        if (value.phase != GatewayCredentialRenewalPhase::Prepare ||
            !exactBytes(value.payload, kProofNonceSize, true) ||
            !emptyBytes(value.receiptDigest)) {
            return invalidArgument();
        }
    } else if (value.status == GatewayCredentialRenewalStatus::Rotated) {
        if (value.phase != GatewayCredentialRenewalPhase::Apply ||
            value.payload.data == nullptr || value.payload.empty() ||
            value.payload.size >
                kGatewayCredentialRenewalReceiptMaximumSize ||
            !exactBytes(
                value.receiptDigest,
                kAccessMaterialDigestSize,
                true)) {
            return invalidArgument();
        }
    } else if (value.status ==
               GatewayCredentialRenewalStatus::Finalized) {
        if (value.phase != GatewayCredentialRenewalPhase::Finalize ||
            !emptyBytes(value.payload) ||
            !exactBytes(
                value.receiptDigest,
                kAccessMaterialDigestSize,
                true)) {
            return invalidArgument();
        }
    } else if (value.status ==
               GatewayCredentialRenewalStatus::Cancelled) {
        if (value.phase != GatewayCredentialRenewalPhase::Cancel ||
            !emptyBytes(value.payload) ||
            !emptyBytes(value.receiptDigest)) {
            return invalidArgument();
        }
    } else if (value.status ==
               GatewayCredentialRenewalStatus::ForwardRecoveryRequired) {
        if (value.phase != GatewayCredentialRenewalPhase::Apply ||
            !emptyBytes(value.payload) ||
            !emptyBytes(value.receiptDigest)) {
            return invalidArgument();
        }
    } else if (!emptyBytes(value.payload) ||
               !emptyBytes(value.receiptDigest)) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayCredentialRenewalResult(
    const GatewayCredentialRenewalResultView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayCredentialRenewalResult(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(13U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) {
        result = writer.writeUnsigned(kGatewayCredentialRenewalResultKind);
    }
    if (result) result = writer.writeBytes(value.renewalOperationId);
    if (result) result = writer.writeBytes(value.topologyOperationId);
    if (result) result = writer.writeUnsigned(value.topologyVersion);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.phase));
    }
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.status));
    }
    if (result) result = writer.writeUnsigned(value.pendingStorageRevision);
    if (result) result = writer.writeUnsigned(value.activeStorageRevision);
    if (result) result = writer.writeUnsigned(value.credentialVersion);
    if (result) result = writer.writeBytes(value.accessMaterialDigest);
    if (result) result = writer.writeBytes(value.payload);
    if (result) result = writer.writeBytes(value.receiptDigest);
    if (result && writer.size() >
                      kGatewayCredentialRenewalResultMaximumEncodedSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayCredentialRenewalResult(
    ByteView encoded,
    GatewayCredentialRenewalResultView& value) {
    value = GatewayCredentialRenewalResultView();
    if (encoded.size >
        kGatewayCredentialRenewalResultMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength =
        kGatewayCredentialRenewalReceiptMaximumSize;
    limits.maxContainerItems = 13U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 13U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayManagementContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayCredentialRenewalResultKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.renewalOperationId);
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.topologyOperationId);
    if (result) result = readUnsigned32(reader, value.topologyVersion);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayCredentialRenewalPhase::Prepare) ||
             decoded > static_cast<uint8_t>(
                           GatewayCredentialRenewalPhase::Cancel))) {
            result = invalidEncoding();
        }
        if (result) value.phase =
            static_cast<GatewayCredentialRenewalPhase>(decoded);
    }
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayCredentialRenewalStatus::NonceReady) ||
             decoded > static_cast<uint8_t>(
                           GatewayCredentialRenewalStatus::Expired))) {
            result = invalidEncoding();
        }
        if (result) value.status =
            static_cast<GatewayCredentialRenewalStatus>(decoded);
    }
    if (result) result = readUnsigned32(
        reader, value.pendingStorageRevision);
    if (result) result = readOptionalUnsigned32(
        reader, value.activeStorageRevision);
    if (result) result = readUnsigned32(reader, value.credentialVersion);
    if (result) result = readExactBytes(
        reader, kAccessMaterialDigestSize, false,
        value.accessMaterialDigest);
    if (result) result = reader.readBytes(value.payload);
    if (result) result = reader.readBytes(value.receiptDigest);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayCredentialRenewalResult(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayPermitJoinCommand(
    const GatewayPermitJoinCommandView& value) {
    const uint8_t action = static_cast<uint8_t>(value.action);
    if (value.version != kGatewayManagementContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.adapterId == 0U ||
        action < static_cast<uint8_t>(GatewayPermitJoinAction::Open) ||
        action > static_cast<uint8_t>(GatewayPermitJoinAction::Close) ||
        value.expiresAtUnixSeconds == 0U ||
        value.expiresAtUnixSeconds > kGatewayMaximumUnixSeconds) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayPermitJoinCommand(
    const GatewayPermitJoinCommandView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayPermitJoinCommand(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayPermitJoinCommandKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.adapterId);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.action));
    }
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayPermitJoinCommand(
    ByteView encoded,
    GatewayPermitJoinCommandView& value) {
    value = GatewayPermitJoinCommandView();
    if (encoded.size > kGatewayPermitJoinCommandMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kOperationIdSize;
    limits.maxContainerItems = 6U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 6U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayManagementContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayPermitJoinCommandKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned16(reader, value.adapterId);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayPermitJoinAction::Open) ||
             decoded > static_cast<uint8_t>(
                           GatewayPermitJoinAction::Close))) {
            result = invalidEncoding();
        }
        if (result) value.action =
            static_cast<GatewayPermitJoinAction>(decoded);
    }
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayPermitJoinCommand(value);
    return normalizedDecodeResult(result);
}

Result validateGatewayPermitJoinResult(
    const GatewayPermitJoinResultView& value) {
    const uint8_t status = static_cast<uint8_t>(value.status);
    if (value.version != kGatewayManagementContractVersion ||
        !exactBytes(value.operationId, kOperationIdSize, true) ||
        value.adapterId == 0U ||
        status < static_cast<uint8_t>(GatewayPermitJoinStatus::Ready) ||
        status > static_cast<uint8_t>(
                     GatewayPermitJoinStatus::Unsupported) ||
        value.expiresAtUnixSeconds == 0U ||
        value.expiresAtUnixSeconds > kGatewayMaximumUnixSeconds) {
        return invalidArgument();
    }
    return Result::success();
}

Result encodeGatewayPermitJoinResult(
    const GatewayPermitJoinResultView& value,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGatewayPermitJoinResult(value);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginArray(6U);
    if (result) result = writer.writeUnsigned(value.version);
    if (result) result = writer.writeUnsigned(kGatewayPermitJoinResultKind);
    if (result) result = writer.writeBytes(value.operationId);
    if (result) result = writer.writeUnsigned(value.adapterId);
    if (result) {
        result = writer.writeUnsigned(static_cast<uint8_t>(value.status));
    }
    if (result) result = writer.writeUnsigned(value.expiresAtUnixSeconds);
    if (result) written = writer.size();
    return result;
}

Result decodeGatewayPermitJoinResult(
    ByteView encoded,
    GatewayPermitJoinResultView& value) {
    value = GatewayPermitJoinResultView();
    if (encoded.size > kGatewayPermitJoinResultMaximumEncodedSize) {
        return invalidEncoding();
    }
    cbor::Limits limits;
    limits.maxByteStringLength = kOperationIdSize;
    limits.maxContainerItems = 6U;
    limits.maxDepth = 1U;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    uint64_t decoded = 0U;
    Result result = reader.readArraySize(count);
    if (result && count != 6U) result = invalidEncoding();
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayManagementContractVersion) {
        result = invalidEncoding();
    }
    if (result) value.version = static_cast<uint8_t>(decoded);
    if (result) result = reader.readUnsigned(decoded);
    if (result && decoded != kGatewayPermitJoinResultKind) {
        result = invalidEncoding();
    }
    if (result) result = readExactBytes(
        reader, kOperationIdSize, true, value.operationId);
    if (result) result = readUnsigned16(reader, value.adapterId);
    if (result) {
        result = reader.readUnsigned(decoded);
        if (result &&
            (decoded < static_cast<uint8_t>(
                           GatewayPermitJoinStatus::Ready) ||
             decoded > static_cast<uint8_t>(
                           GatewayPermitJoinStatus::Unsupported))) {
            result = invalidEncoding();
        }
        if (result) value.status =
            static_cast<GatewayPermitJoinStatus>(decoded);
    }
    if (result) result = reader.readUnsigned(value.expiresAtUnixSeconds);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateGatewayPermitJoinResult(value);
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
