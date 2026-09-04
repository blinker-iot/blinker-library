#include "GatewayCredentialRenewalStore.h"

#include <limits.h>
#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"
#include "../core/Sha256.h"

namespace blinker {

namespace gateway_credential_renewal_store_detail {

const uint8_t kMagic[4] = {'B', 'G', 'C', '1'};
const uint8_t kSchemaVersion = 1U;
const size_t kStorageRevisionOffset = 8U;
const size_t kDeliveryRevisionOffset = 12U;
const size_t kRenewalOperationOffset = 16U;
const size_t kTopologyOperationOffset = 32U;
const size_t kChildInstanceOffset = 48U;
const size_t kAccessEpochOffset = 64U;
const size_t kControllerOffset = 68U;
const size_t kExpectedVersionOffset = 84U;
const size_t kCredentialVersionOffset = 88U;
const size_t kPresenceVersionOffset = 92U;
const size_t kExpiresAtOffset = 96U;
const size_t kSecretOffset = 104U;
const size_t kAccessMaterialDigestOffset = 136U;
const size_t kGrantDigestOffset = 168U;
const size_t kReceiptSizeOffset = 200U;
const size_t kReceiptOffset = 204U;
const size_t kActiveStorageRevisionOffset = 352U;
const size_t kCrcOffset = 356U;

void writeU16(uint8_t* output, uint16_t value) {
    output[0] = static_cast<uint8_t>(value >> 8U);
    output[1] = static_cast<uint8_t>(value);
}

uint16_t readU16(const uint8_t* input) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(input[0]) << 8U) |
        static_cast<uint16_t>(input[1]));
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

void writeU64(uint8_t* output, uint64_t value) {
    for (size_t index = 0U; index < 8U; ++index) {
        output[index] = static_cast<uint8_t>(
            value >> static_cast<unsigned int>((7U - index) * 8U));
    }
}

uint64_t readU64(const uint8_t* input) {
    uint64_t value = 0U;
    for (size_t index = 0U; index < 8U; ++index) {
        value = (value << 8U) | static_cast<uint64_t>(input[index]);
    }
    return value;
}

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool zero(ByteView value) {
    if (value.data == nullptr) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

bool equal(ByteView first, ByteView second) {
    return constantTimeEqual(first, second);
}

bool validState(GatewayCredentialRenewalState state) {
    return state == GatewayCredentialRenewalState::Staged ||
           state == GatewayCredentialRenewalState::MutationAttempted ||
           state == GatewayCredentialRenewalState::ReceiptStored ||
           state == GatewayCredentialRenewalState::Promoted ||
           state == GatewayCredentialRenewalState::Abandoned;
}

bool validRecord(
    const GatewayCredentialRenewalRecord& value,
    bool durable) {
    if (!validState(value.state) ||
        (durable ? value.storageRevision == 0U ||
                       value.deliveryRevision == 0U ||
                       value.deliveryRevision > value.storageRevision
                 : value.storageRevision != 0U ||
                       value.deliveryRevision != 0U) ||
        !nonZero(ByteView(value.renewalOperationId,
                          sizeof(value.renewalOperationId))) ||
        !nonZero(ByteView(value.topologyOperationId,
                          sizeof(value.topologyOperationId))) ||
        !nonZero(ByteView(value.childDeviceInstanceId,
                          sizeof(value.childDeviceInstanceId))) ||
        value.accessEpoch == 0U ||
        !nonZero(ByteView(value.controllerId,
                          sizeof(value.controllerId))) ||
        value.expectedCredentialVersion == 0U ||
        value.expectedCredentialVersion == UINT32_MAX ||
        value.credentialVersion != value.expectedCredentialVersion + 1U ||
        value.presenceKeyVersion == 0U ||
        value.expiresAtUnixSeconds == 0U ||
        !nonZero(ByteView(value.secret, sizeof(value.secret))) ||
        !nonZero(ByteView(value.accessMaterialDigest,
                          sizeof(value.accessMaterialDigest)))) {
        return false;
    }

    const bool noGrant = zero(ByteView(
        value.grantDigest, sizeof(value.grantDigest)));
    const bool noReceipt = value.receiptSize == 0U &&
        zero(ByteView(value.receipt, sizeof(value.receipt)));
    switch (value.state) {
        case GatewayCredentialRenewalState::Staged:
            return noGrant && noReceipt && value.activeStorageRevision == 0U;
        case GatewayCredentialRenewalState::MutationAttempted:
        case GatewayCredentialRenewalState::Abandoned:
            return !noGrant && noReceipt && value.activeStorageRevision == 0U;
        case GatewayCredentialRenewalState::ReceiptStored:
            return !noGrant && value.receiptSize != 0U &&
                   value.receiptSize <= sizeof(value.receipt) &&
                   value.activeStorageRevision == 0U;
        case GatewayCredentialRenewalState::Promoted:
            return !noGrant && value.receiptSize != 0U &&
                   value.receiptSize <= sizeof(value.receipt) &&
                   value.activeStorageRevision != 0U;
    }
    return false;
}

bool sameIdentity(
    const GatewayCredentialRenewalRecord& value,
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest) {
    return renewalOperationId.size == sizeof(value.renewalOperationId) &&
           topologyOperationId.size == sizeof(value.topologyOperationId) &&
           accessMaterialDigest.size == sizeof(value.accessMaterialDigest) &&
           equal(ByteView(value.renewalOperationId,
                          sizeof(value.renewalOperationId)),
                 renewalOperationId) &&
           equal(ByteView(value.topologyOperationId,
                          sizeof(value.topologyOperationId)),
                 topologyOperationId) &&
           equal(ByteView(value.accessMaterialDigest,
                          sizeof(value.accessMaterialDigest)),
                 accessMaterialDigest);
}

bool sameCandidate(
    const GatewayCredentialRenewalRecord& stored,
    const GatewayCredentialRenewalRecord& candidate) {
    return sameIdentity(
               stored,
               ByteView(candidate.renewalOperationId,
                        sizeof(candidate.renewalOperationId)),
               ByteView(candidate.topologyOperationId,
                        sizeof(candidate.topologyOperationId)),
               ByteView(candidate.accessMaterialDigest,
                        sizeof(candidate.accessMaterialDigest))) &&
           equal(ByteView(stored.childDeviceInstanceId,
                          sizeof(stored.childDeviceInstanceId)),
                 ByteView(candidate.childDeviceInstanceId,
                          sizeof(candidate.childDeviceInstanceId))) &&
           stored.accessEpoch == candidate.accessEpoch &&
           equal(ByteView(stored.controllerId, sizeof(stored.controllerId)),
                 ByteView(candidate.controllerId,
                          sizeof(candidate.controllerId))) &&
           stored.expectedCredentialVersion ==
               candidate.expectedCredentialVersion &&
           stored.credentialVersion == candidate.credentialVersion &&
           stored.presenceKeyVersion == candidate.presenceKeyVersion &&
           stored.expiresAtUnixSeconds == candidate.expiresAtUnixSeconds &&
           equal(ByteView(stored.secret, sizeof(stored.secret)),
                 ByteView(candidate.secret, sizeof(candidate.secret)));
}

bool sameRecord(
    const GatewayCredentialRenewalRecord& first,
    const GatewayCredentialRenewalRecord& second) {
    return first.storageRevision == second.storageRevision &&
           first.deliveryRevision == second.deliveryRevision &&
           first.state == second.state &&
           sameCandidate(first, second) &&
           equal(ByteView(first.grantDigest, sizeof(first.grantDigest)),
                 ByteView(second.grantDigest, sizeof(second.grantDigest))) &&
           first.receiptSize == second.receiptSize &&
           equal(ByteView(first.receipt, sizeof(first.receipt)),
                 ByteView(second.receipt, sizeof(second.receipt))) &&
           first.activeStorageRevision == second.activeStorageRevision;
}

Result replaceVerified(
    IAtomicBlobStore& storage,
    const GatewayCredentialRenewalRecord& value) {
    uint8_t encoded[GatewayCredentialRenewalStore::serializedSize] = {};
    Result result = GatewayCredentialRenewalStore::encode(
        value, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage.replace(ByteView(encoded, sizeof(encoded)));

    uint8_t verifiedBytes[
        GatewayCredentialRenewalStore::serializedSize] = {};
    size_t written = 0U;
    if (result) {
        result = storage.load(
            MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)), written);
    }
    GatewayCredentialRenewalRecord verified;
    if (result && written != sizeof(verifiedBytes)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        result = GatewayCredentialRenewalStore::decode(
            ByteView(verifiedBytes, written), verified);
    }
    if (result && !sameRecord(value, verified)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    clearGatewayCredentialRenewalRecord(verified);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)));
    return result;
}

Result loadMatching(
    GatewayCredentialRenewalStore& store,
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    GatewayCredentialRenewalRecord& output) {
    Result result = store.load(output);
    if (result && !sameIdentity(
                      output, renewalOperationId, topologyOperationId,
                      accessMaterialDigest)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    return result;
}

} // namespace gateway_credential_renewal_store_detail

GatewayCredentialRenewalRecord::GatewayCredentialRenewalRecord()
    : storageRevision(0U), deliveryRevision(0U),
      state(GatewayCredentialRenewalState::Staged), renewalOperationId(),
      topologyOperationId(), childDeviceInstanceId(),
      accessEpoch(0U), controllerId(), expectedCredentialVersion(0U),
      credentialVersion(0U), presenceKeyVersion(0U),
      expiresAtUnixSeconds(0U), secret(), accessMaterialDigest(),
      grantDigest(), receiptSize(0U), receipt(), activeStorageRevision(0U) {}

void clearGatewayCredentialRenewalRecord(
    GatewayCredentialRenewalRecord& value) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&value), sizeof(value)));
}

Result GatewayCredentialRenewalStore::receiptDigest(
    const GatewayCredentialRenewalRecord& value,
    MutableByteSpan output) {
    if (value.receiptSize == 0U ||
        value.receiptSize > sizeof(value.receipt)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < kSha256Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    return sha256(
        ByteView(value.receipt, value.receiptSize), output);
}

Result GatewayCredentialRenewalStore::validateDurable(
    const GatewayCredentialRenewalRecord& value) {
    return gateway_credential_renewal_store_detail::validRecord(value, true)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result GatewayCredentialRenewalStore::encode(
    const GatewayCredentialRenewalRecord& value,
    MutableByteSpan output) {
    namespace detail = gateway_credential_renewal_store_detail;
    if (!detail::validRecord(value, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, detail::kMagic, sizeof(detail::kMagic));
    output.data[4] = detail::kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(value.state);
    detail::writeU32(output.data + detail::kStorageRevisionOffset,
                     value.storageRevision);
    detail::writeU32(output.data + detail::kDeliveryRevisionOffset,
                     value.deliveryRevision);
    memcpy(output.data + detail::kRenewalOperationOffset,
           value.renewalOperationId, sizeof(value.renewalOperationId));
    memcpy(output.data + detail::kTopologyOperationOffset,
           value.topologyOperationId, sizeof(value.topologyOperationId));
    memcpy(output.data + detail::kChildInstanceOffset,
           value.childDeviceInstanceId, sizeof(value.childDeviceInstanceId));
    detail::writeU32(output.data + detail::kAccessEpochOffset,
                     value.accessEpoch);
    memcpy(output.data + detail::kControllerOffset,
           value.controllerId, sizeof(value.controllerId));
    detail::writeU32(output.data + detail::kExpectedVersionOffset,
                     value.expectedCredentialVersion);
    detail::writeU32(output.data + detail::kCredentialVersionOffset,
                     value.credentialVersion);
    detail::writeU32(output.data + detail::kPresenceVersionOffset,
                     value.presenceKeyVersion);
    detail::writeU64(output.data + detail::kExpiresAtOffset,
                     value.expiresAtUnixSeconds);
    memcpy(output.data + detail::kSecretOffset,
           value.secret, sizeof(value.secret));
    memcpy(output.data + detail::kAccessMaterialDigestOffset,
           value.accessMaterialDigest, sizeof(value.accessMaterialDigest));
    memcpy(output.data + detail::kGrantDigestOffset,
           value.grantDigest, sizeof(value.grantDigest));
    detail::writeU16(output.data + detail::kReceiptSizeOffset,
                     value.receiptSize);
    memcpy(output.data + detail::kReceiptOffset,
           value.receipt, value.receiptSize);
    detail::writeU32(output.data + detail::kActiveStorageRevisionOffset,
                     value.activeStorageRevision);
    detail::writeU32(output.data + detail::kCrcOffset,
                     computeCrc32(ByteView(output.data,
                                           detail::kCrcOffset)));
    return Result::success();
}

Result GatewayCredentialRenewalStore::decode(
    ByteView input,
    GatewayCredentialRenewalRecord& output) {
    namespace detail = gateway_credential_renewal_store_detail;
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, detail::kMagic, sizeof(detail::kMagic)) != 0 ||
        input.data[4] != detail::kSchemaVersion || input.data[6] != 0U ||
        input.data[7] != 0U ||
        detail::readU32(input.data + detail::kCrcOffset) !=
            computeCrc32(ByteView(input.data, detail::kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    GatewayCredentialRenewalRecord decoded;
    decoded.state = static_cast<GatewayCredentialRenewalState>(input.data[5]);
    decoded.storageRevision = detail::readU32(
        input.data + detail::kStorageRevisionOffset);
    decoded.deliveryRevision = detail::readU32(
        input.data + detail::kDeliveryRevisionOffset);
    memcpy(decoded.renewalOperationId,
           input.data + detail::kRenewalOperationOffset,
           sizeof(decoded.renewalOperationId));
    memcpy(decoded.topologyOperationId,
           input.data + detail::kTopologyOperationOffset,
           sizeof(decoded.topologyOperationId));
    memcpy(decoded.childDeviceInstanceId,
           input.data + detail::kChildInstanceOffset,
           sizeof(decoded.childDeviceInstanceId));
    decoded.accessEpoch = detail::readU32(
        input.data + detail::kAccessEpochOffset);
    memcpy(decoded.controllerId, input.data + detail::kControllerOffset,
           sizeof(decoded.controllerId));
    decoded.expectedCredentialVersion = detail::readU32(
        input.data + detail::kExpectedVersionOffset);
    decoded.credentialVersion = detail::readU32(
        input.data + detail::kCredentialVersionOffset);
    decoded.presenceKeyVersion = detail::readU32(
        input.data + detail::kPresenceVersionOffset);
    decoded.expiresAtUnixSeconds = detail::readU64(
        input.data + detail::kExpiresAtOffset);
    memcpy(decoded.secret, input.data + detail::kSecretOffset,
           sizeof(decoded.secret));
    memcpy(decoded.accessMaterialDigest,
           input.data + detail::kAccessMaterialDigestOffset,
           sizeof(decoded.accessMaterialDigest));
    memcpy(decoded.grantDigest, input.data + detail::kGrantDigestOffset,
           sizeof(decoded.grantDigest));
    decoded.receiptSize = detail::readU16(
        input.data + detail::kReceiptSizeOffset);
    if (decoded.receiptSize <= sizeof(decoded.receipt)) {
        memcpy(decoded.receipt, input.data + detail::kReceiptOffset,
               decoded.receiptSize);
    }
    const size_t receiptEnd = detail::kReceiptOffset + decoded.receiptSize;
    if (decoded.receiptSize > sizeof(decoded.receipt) ||
        receiptEnd > detail::kActiveStorageRevisionOffset) {
        clearGatewayCredentialRenewalRecord(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    for (size_t index = receiptEnd;
         index < detail::kActiveStorageRevisionOffset; ++index) {
        if (input.data[index] != 0U) {
            clearGatewayCredentialRenewalRecord(decoded);
            return Result::failure(ErrorCode::NonCanonicalEncoding);
        }
    }
    decoded.activeStorageRevision = detail::readU32(
        input.data + detail::kActiveStorageRevisionOffset);
    if (!detail::validRecord(decoded, true)) {
        clearGatewayCredentialRenewalRecord(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    output = decoded;
    clearGatewayCredentialRenewalRecord(decoded);
    return Result::success();
}

Result GatewayCredentialRenewalStore::load(
    GatewayCredentialRenewalRecord& output) {
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    GatewayCredentialRenewalRecord decoded;
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearGatewayCredentialRenewalRecord(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result GatewayCredentialRenewalStore::commit(
    const GatewayCredentialRenewalRecord& candidate,
    GatewayCredentialRenewalCommitOutcome& outcome) {
    namespace detail = gateway_credential_renewal_store_detail;
    outcome = GatewayCredentialRenewalCommitOutcome();
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!detail::validRecord(candidate, false) ||
        candidate.state != GatewayCredentialRenewalState::Staged) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    GatewayCredentialRenewalRecord current;
    Result result = load(current);
    if (result) {
        if (!detail::sameCandidate(current, candidate)) {
            clearGatewayCredentialRenewalRecord(current);
            return Result::failure(ErrorCode::SequenceConflict);
        }
        outcome.kind = GatewayCredentialRenewalCommitKind::ExactReplay;
        outcome.pendingStorageRevision = current.deliveryRevision;
        clearGatewayCredentialRenewalRecord(current);
        return Result::success();
    }
    if (result.code() != ErrorCode::NotFound) {
        clearGatewayCredentialRenewalRecord(current);
        return result;
    }

    GatewayCredentialRenewalRecord next = candidate;
    next.storageRevision = 1U;
    next.deliveryRevision = 1U;
    result = detail::replaceVerified(storage_, next);
    if (result) outcome.pendingStorageRevision = next.deliveryRevision;
    clearGatewayCredentialRenewalRecord(current);
    clearGatewayCredentialRenewalRecord(next);
    return result;
}

Result GatewayCredentialRenewalStore::markMutationAttempted(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    ByteView grantDigest,
    uint32_t& storageRevision) {
    namespace detail = gateway_credential_renewal_store_detail;
    storageRevision = 0U;
    if (grantDigest.size != gateway::kAccessMaterialDigestSize ||
        !detail::nonZero(grantDigest)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayCredentialRenewalRecord current;
    Result result = detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    if (result && current.state != GatewayCredentialRenewalState::Staged) {
        if (!detail::equal(ByteView(current.grantDigest,
                            sizeof(current.grantDigest)), grantDigest)) {
            result = Result::failure(ErrorCode::SequenceConflict);
        } else {
            storageRevision = current.storageRevision;
        }
        clearGatewayCredentialRenewalRecord(current);
        return result;
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        memcpy(current.grantDigest, grantDigest.data, grantDigest.size);
        current.state = GatewayCredentialRenewalState::MutationAttempted;
        ++current.storageRevision;
        result = detail::replaceVerified(storage_, current);
        if (result) storageRevision = current.storageRevision;
    }
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::markAbandoned(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    ByteView grantDigest,
    uint32_t& storageRevision) {
    namespace detail = gateway_credential_renewal_store_detail;
    storageRevision = 0U;
    if (grantDigest.size != gateway::kAccessMaterialDigestSize ||
        !detail::nonZero(grantDigest)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayCredentialRenewalRecord current;
    Result result = detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    if (result && !detail::equal(
                      ByteView(current.grantDigest,
                               sizeof(current.grantDigest)),
                      grantDigest)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && current.state ==
                      GatewayCredentialRenewalState::Abandoned) {
        storageRevision = current.storageRevision;
        clearGatewayCredentialRenewalRecord(current);
        return Result::success();
    }
    if (result && current.state !=
                      GatewayCredentialRenewalState::MutationAttempted) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        current.state = GatewayCredentialRenewalState::Abandoned;
        ++current.storageRevision;
        result = detail::replaceVerified(storage_, current);
        if (result) storageRevision = current.storageRevision;
    }
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::storeReceipt(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    ByteView grantDigest,
    ByteView exactReceipt,
    uint32_t& storageRevision) {
    namespace detail = gateway_credential_renewal_store_detail;
    storageRevision = 0U;
    if (grantDigest.size != gateway::kAccessMaterialDigestSize ||
        !detail::nonZero(grantDigest) || exactReceipt.data == nullptr ||
        exactReceipt.empty() ||
        exactReceipt.size >
            gateway::kGatewayCredentialRenewalReceiptMaximumSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayCredentialRenewalRecord current;
    Result result = detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    if (result && current.state ==
                      GatewayCredentialRenewalState::Staged) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && !detail::equal(ByteView(current.grantDigest,
                                  sizeof(current.grantDigest)),
                         grantDigest)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result &&
        (current.state == GatewayCredentialRenewalState::ReceiptStored ||
         current.state == GatewayCredentialRenewalState::Promoted)) {
        if (current.receiptSize != exactReceipt.size ||
            !detail::equal(ByteView(current.receipt, current.receiptSize),
                   exactReceipt)) {
            result = Result::failure(ErrorCode::SequenceConflict);
        } else {
            storageRevision = current.storageRevision;
        }
        clearGatewayCredentialRenewalRecord(current);
        return result;
    }
    if (result && current.state !=
                      GatewayCredentialRenewalState::MutationAttempted) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        memcpy(current.receipt, exactReceipt.data, exactReceipt.size);
        current.receiptSize = static_cast<uint16_t>(exactReceipt.size);
        current.state = GatewayCredentialRenewalState::ReceiptStored;
        ++current.storageRevision;
        result = detail::replaceVerified(storage_, current);
        if (result) storageRevision = current.storageRevision;
    }
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::markPromoted(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    ByteView expectedReceiptDigest,
    uint32_t activeStorageRevision,
    uint32_t& storageRevision) {
    namespace detail = gateway_credential_renewal_store_detail;
    storageRevision = 0U;
    if (expectedReceiptDigest.size != kSha256Size ||
        activeStorageRevision == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayCredentialRenewalRecord current;
    Result result = detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    uint8_t digest[kSha256Size] = {};
    if (result &&
        current.state != GatewayCredentialRenewalState::ReceiptStored &&
        current.state != GatewayCredentialRenewalState::Promoted) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = receiptDigest(current,
        MutableByteSpan(digest, sizeof(digest)));
    if (result && !detail::equal(ByteView(digest, sizeof(digest)),
                         expectedReceiptDigest)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && current.state ==
                      GatewayCredentialRenewalState::Promoted) {
        if (current.activeStorageRevision != activeStorageRevision) {
            result = Result::failure(ErrorCode::SequenceConflict);
        } else {
            storageRevision = current.storageRevision;
        }
        secureZero(MutableByteSpan(digest, sizeof(digest)));
        clearGatewayCredentialRenewalRecord(current);
        return result;
    }
    if (result && current.state !=
                      GatewayCredentialRenewalState::ReceiptStored) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        current.state = GatewayCredentialRenewalState::Promoted;
        current.activeStorageRevision = activeStorageRevision;
        ++current.storageRevision;
        result = detail::replaceVerified(storage_, current);
        if (result) storageRevision = current.storageRevision;
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::cancel(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest) {
    GatewayCredentialRenewalRecord current;
    Result result = gateway_credential_renewal_store_detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    if (result && current.state != GatewayCredentialRenewalState::Staged &&
        current.state != GatewayCredentialRenewalState::Abandoned) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = storage_.clear();
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::finalize(
    ByteView renewalOperationId,
    ByteView topologyOperationId,
    ByteView accessMaterialDigest,
    ByteView expectedReceiptDigest,
    uint32_t activeStorageRevision) {
    namespace detail = gateway_credential_renewal_store_detail;
    if (expectedReceiptDigest.size != kSha256Size ||
        activeStorageRevision == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayCredentialRenewalRecord current;
    Result result = detail::loadMatching(
        *this, renewalOperationId, topologyOperationId,
        accessMaterialDigest, current);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    uint8_t digest[kSha256Size] = {};
    if (result &&
        (current.state != GatewayCredentialRenewalState::Promoted ||
         current.activeStorageRevision != activeStorageRevision)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = receiptDigest(
        current, MutableByteSpan(digest, sizeof(digest)));
    if (result && !detail::equal(ByteView(digest, sizeof(digest)),
                         expectedReceiptDigest)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result) result = storage_.clear();
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    clearGatewayCredentialRenewalRecord(current);
    return result;
}

Result GatewayCredentialRenewalStore::clear() {
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound ? Result::success() : result;
}

} // namespace blinker
