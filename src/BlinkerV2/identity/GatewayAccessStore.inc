#include "GatewayAccessStore.h"
#include "GatewayCredentialRenewalStore.h"

#include <limits.h>
#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace gateway_access_store_detail {

const uint8_t kMagic[4] = {'B', 'G', 'A', '2'};
const uint8_t kSchemaVersion = 2U;
const uint8_t kRevocationMagic[4] = {'B', 'G', 'R', '1'};
const uint8_t kRevocationSchemaVersion = 1U;
const size_t kRevisionOffset = 8U;
const size_t kOperationOffset = 12U;
const size_t kEnvelopeDigestOffset = 28U;
const size_t kAccessMaterialDigestOffset = 60U;
const size_t kChildInstanceOffset = 92U;
const size_t kAccessEpochOffset = 108U;
const size_t kControllerOffset = 112U;
const size_t kCredentialVersionOffset = 128U;
const size_t kPermissionsOffset = 132U;
const size_t kExpiresAtOffset = 136U;
const size_t kSecretOffset = 144U;
const size_t kActivePresenceVersionOffset = 176U;
const size_t kActivePresenceKeyOffset = 180U;
const size_t kPendingPresenceVersionOffset = 196U;
const size_t kPendingPresenceKeyOffset = 200U;
const size_t kChildLogicalIdOffset = 216U;
const size_t kDeliveryRevisionOffset = 344U;
const size_t kCrcOffset = GatewayAccessStore::serializedSize - 4U;

const size_t kRevocationRevisionOffset = 8U;
const size_t kRevocationOperationOffset = 12U;
const size_t kRevocationTopologyOffset = 28U;
const size_t kRevocationGrantDigestOffset = 32U;
const size_t kRevocationReceiptDigestOffset = 64U;
const size_t kRevocationReceiptSizeOffset = 96U;
const size_t kRevocationReceiptOffset = 100U;

enum class StoredKind : uint8_t {
    Active = 1U,
    Revocation = 2U
};

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

Result storedKind(ByteView input, StoredKind& kind) {
    if (input.data == nullptr ||
        input.size != GatewayAccessStore::serializedSize) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (memcmp(input.data, kMagic, sizeof(kMagic)) == 0) {
        kind = StoredKind::Active;
        return Result::success();
    }
    if (memcmp(input.data, kRevocationMagic,
               sizeof(kRevocationMagic)) == 0) {
        kind = StoredKind::Revocation;
        return Result::success();
    }
    return Result::failure(ErrorCode::InvalidEncoding);
}

bool validPresence(const GatewayAccessRecord& value, bool durable) {
    if (value.activePresence.version == 0U ||
        !nonZero(ByteView(
            value.activePresence.key,
            sizeof(value.activePresence.key)))) {
        return false;
    }
    const bool pendingEmpty = value.pendingPresence.version == 0U &&
        zero(ByteView(
            value.pendingPresence.key,
            sizeof(value.pendingPresence.key)));
    if (pendingEmpty) return true;
    return durable &&
           value.pendingPresence.version > value.activePresence.version &&
           nonZero(ByteView(
               value.pendingPresence.key,
               sizeof(value.pendingPresence.key)));
}

bool validIdentifier(const GatewayAccessRecord& value) {
    if (value.childLogicalDeviceIdSize == 0U ||
        value.childLogicalDeviceIdSize >
            gateway::kMaximumLogicalDeviceIdSize) {
        return false;
    }
    for (size_t index = 0U; index < value.childLogicalDeviceIdSize; ++index) {
        const uint8_t character = static_cast<uint8_t>(
            value.childLogicalDeviceId[index]);
        if (character < 0x21U || character > 0x7EU ||
            character == '/' || character == '\\' ||
            character == '#' || character == '+') {
            return false;
        }
    }
    return true;
}

bool validRecord(const GatewayAccessRecord& value, bool durable) {
    return (!durable ||
            (value.storageRevision != 0U &&
             value.deliveryRevision != 0U &&
             value.deliveryRevision <= value.storageRevision)) &&
           (durable ||
            (value.storageRevision == 0U && value.deliveryRevision == 0U)) &&
           nonZero(ByteView(value.operationId, sizeof(value.operationId))) &&
           validIdentifier(value) &&
           nonZero(ByteView(
               value.childDeviceInstanceId,
               sizeof(value.childDeviceInstanceId))) &&
           value.accessEpoch != 0U &&
           nonZero(ByteView(value.controllerId, sizeof(value.controllerId))) &&
           value.credentialVersion != 0U &&
           value.permissions == gateway::kGatewayPermissions &&
           value.expiresAtUnixSeconds != 0U &&
           nonZero(ByteView(value.secret, sizeof(value.secret))) &&
           validPresence(value, durable);
}

bool equalBytes(const void* first, const void* second, size_t size) {
    return constantTimeEqual(
        ByteView(static_cast<const uint8_t*>(first), size),
        ByteView(static_cast<const uint8_t*>(second), size));
}

bool sameChild(const GatewayAccessRecord& first,
               const GatewayAccessRecord& second) {
    return first.childLogicalDeviceIdSize ==
               second.childLogicalDeviceIdSize &&
           memcmp(first.childLogicalDeviceId,
                  second.childLogicalDeviceId,
                  first.childLogicalDeviceIdSize) == 0 &&
           equalBytes(first.childDeviceInstanceId,
                      second.childDeviceInstanceId,
                      sizeof(first.childDeviceInstanceId));
}

bool sameControllerCredential(const GatewayAccessRecord& first,
                              const GatewayAccessRecord& second) {
    return sameChild(first, second) &&
           first.accessEpoch == second.accessEpoch &&
           equalBytes(first.controllerId, second.controllerId,
                      sizeof(first.controllerId)) &&
           first.credentialVersion == second.credentialVersion &&
           first.permissions == second.permissions &&
           first.expiresAtUnixSeconds == second.expiresAtUnixSeconds &&
           equalBytes(first.secret, second.secret, sizeof(first.secret));
}

bool samePresence(const GatewayPresenceLocator& first,
                  const GatewayPresenceLocator& second) {
    return first.version == second.version &&
           equalBytes(first.key, second.key, sizeof(first.key));
}

bool deliveredPresenceMatches(const GatewayAccessRecord& stored,
                              const GatewayAccessRecord& delivered) {
    return samePresence(
        stored.pendingPresence.version != 0U
            ? stored.pendingPresence
            : stored.activePresence,
        delivered.activePresence);
}

bool sameRecord(const GatewayAccessRecord& first,
                const GatewayAccessRecord& second) {
    return first.storageRevision == second.storageRevision &&
           first.deliveryRevision == second.deliveryRevision &&
           equalBytes(first.operationId, second.operationId,
                      sizeof(first.operationId)) &&
           equalBytes(first.envelopeDigest, second.envelopeDigest,
                      sizeof(first.envelopeDigest)) &&
           equalBytes(
               first.accessMaterialDigest,
               second.accessMaterialDigest,
               sizeof(first.accessMaterialDigest)) &&
           sameControllerCredential(first, second) &&
           samePresence(first.activePresence, second.activePresence) &&
           samePresence(first.pendingPresence, second.pendingPresence);
}

bool validRevocation(
    const GatewayRevocationTombstone& value,
    bool durable) {
    return (durable ? value.storageRevision != 0U
                    : value.storageRevision == 0U) &&
           nonZero(ByteView(value.operationId, sizeof(value.operationId))) &&
           value.topologyVersion != 0U &&
           nonZero(ByteView(
               value.grantDigest, sizeof(value.grantDigest))) &&
           nonZero(ByteView(
               value.receiptDigest, sizeof(value.receiptDigest))) &&
           value.receiptSize != 0U &&
           value.receiptSize <=
               GatewayRevocationTombstone::maximumReceiptSize;
}

bool sameRevocation(
    const GatewayRevocationTombstone& first,
    const GatewayRevocationTombstone& second) {
    return first.storageRevision == second.storageRevision &&
           first.topologyVersion == second.topologyVersion &&
           first.receiptSize == second.receiptSize &&
           equalBytes(first.operationId, second.operationId,
                      sizeof(first.operationId)) &&
           equalBytes(first.grantDigest, second.grantDigest,
                      sizeof(first.grantDigest)) &&
           equalBytes(first.receiptDigest, second.receiptDigest,
                      sizeof(first.receiptDigest)) &&
           equalBytes(first.receipt, second.receipt,
                      first.receiptSize);
}

Result encodeRevocation(
    const GatewayRevocationTombstone& value,
    MutableByteSpan output) {
    if (!validRevocation(value, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr ||
        output.size < GatewayAccessStore::serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, GatewayAccessStore::serializedSize);
    memcpy(output.data, kRevocationMagic, sizeof(kRevocationMagic));
    output.data[4] = kRevocationSchemaVersion;
    writeU32(output.data + kRevocationRevisionOffset,
             value.storageRevision);
    memcpy(output.data + kRevocationOperationOffset,
           value.operationId, sizeof(value.operationId));
    writeU32(output.data + kRevocationTopologyOffset,
             value.topologyVersion);
    memcpy(output.data + kRevocationGrantDigestOffset,
           value.grantDigest, sizeof(value.grantDigest));
    memcpy(output.data + kRevocationReceiptDigestOffset,
           value.receiptDigest, sizeof(value.receiptDigest));
    writeU16(output.data + kRevocationReceiptSizeOffset,
             value.receiptSize);
    memcpy(output.data + kRevocationReceiptOffset,
           value.receipt, value.receiptSize);
    writeU32(output.data + kCrcOffset,
             computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result decodeRevocation(
    ByteView input,
    GatewayRevocationTombstone& output) {
    if (input.data == nullptr ||
        input.size != GatewayAccessStore::serializedSize ||
        memcmp(input.data, kRevocationMagic,
               sizeof(kRevocationMagic)) != 0 ||
        input.data[4] != kRevocationSchemaVersion ||
        input.data[5] != 0U || input.data[6] != 0U ||
        input.data[7] != 0U || input.data[98] != 0U ||
        input.data[99] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    GatewayRevocationTombstone decoded;
    decoded.storageRevision = readU32(
        input.data + kRevocationRevisionOffset);
    memcpy(decoded.operationId,
           input.data + kRevocationOperationOffset,
           sizeof(decoded.operationId));
    decoded.topologyVersion = readU32(
        input.data + kRevocationTopologyOffset);
    memcpy(decoded.grantDigest,
           input.data + kRevocationGrantDigestOffset,
           sizeof(decoded.grantDigest));
    memcpy(decoded.receiptDigest,
           input.data + kRevocationReceiptDigestOffset,
           sizeof(decoded.receiptDigest));
    decoded.receiptSize = readU16(
        input.data + kRevocationReceiptSizeOffset);
    if (decoded.receiptSize != 0U &&
        decoded.receiptSize <=
            GatewayRevocationTombstone::maximumReceiptSize) {
        memcpy(decoded.receipt,
               input.data + kRevocationReceiptOffset,
               decoded.receiptSize);
    }
    const size_t receiptEnd =
        kRevocationReceiptOffset + decoded.receiptSize;
    for (size_t index = receiptEnd; index < kCrcOffset; ++index) {
        if (input.data[index] != 0U) {
            clearGatewayRevocationTombstone(decoded);
            return Result::failure(ErrorCode::NonCanonicalEncoding);
        }
    }
    if (!validRevocation(decoded, true)) {
        clearGatewayRevocationTombstone(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    output = decoded;
    clearGatewayRevocationTombstone(decoded);
    return Result::success();
}

Result replaceVerified(
    IAtomicBlobStore& storage,
    const GatewayAccessRecord& value) {
    uint8_t encoded[GatewayAccessStore::serializedSize] = {};
    Result result = GatewayAccessStore::encode(
        value, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage.replace(ByteView(encoded, sizeof(encoded)));

    uint8_t verifiedBytes[GatewayAccessStore::serializedSize] = {};
    size_t written = 0U;
    if (result) {
        result = storage.load(
            MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)), written);
    }
    GatewayAccessRecord verified;
    if (result && written != sizeof(verifiedBytes)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        result = GatewayAccessStore::decode(
            ByteView(verifiedBytes, written), verified);
    }
    if (result && !sameRecord(value, verified)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    clearGatewayAccessRecord(verified);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)));
    return result;
}

Result replaceRevocationVerified(
    IAtomicBlobStore& storage,
    const GatewayRevocationTombstone& value) {
    uint8_t encoded[GatewayAccessStore::serializedSize] = {};
    Result result = encodeRevocation(
        value, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage.replace(ByteView(encoded, sizeof(encoded)));

    uint8_t verifiedBytes[GatewayAccessStore::serializedSize] = {};
    size_t written = 0U;
    if (result) {
        result = storage.load(
            MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)), written);
    }
    GatewayRevocationTombstone verified;
    if (result && written != sizeof(verifiedBytes)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        result = decodeRevocation(
            ByteView(verifiedBytes, written), verified);
    }
    if (result && !sameRevocation(value, verified)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    clearGatewayRevocationTombstone(verified);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(verifiedBytes, sizeof(verifiedBytes)));
    return result;
}

} // namespace gateway_access_store_detail

GatewayAccessRecord::GatewayAccessRecord()
    : storageRevision(0U), deliveryRevision(0U), operationId(),
      envelopeDigest(), accessMaterialDigest(),
      childLogicalDeviceIdSize(0U), childLogicalDeviceId(),
      childDeviceInstanceId(), accessEpoch(0U), controllerId(),
      credentialVersion(0U), permissions(0U), expiresAtUnixSeconds(0U),
      secret(), activePresence(), pendingPresence() {}

GatewayRevocationTombstone::GatewayRevocationTombstone()
    : storageRevision(0U), operationId(), topologyVersion(0U),
      grantDigest(), receiptDigest(), receiptSize(0U), receipt() {}

void clearGatewayAccessRecord(GatewayAccessRecord& value) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&value), sizeof(value)));
}

void clearGatewayRevocationTombstone(
    GatewayRevocationTombstone& value) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&value), sizeof(value)));
}

Result makeGatewayControllerCredential(
    const GatewayAccessRecord& record,
    uint64_t unixSeconds,
    ControllerCredential& output) {
    clearControllerCredential(output);
    if (!gateway_access_store_detail::validRecord(record, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (unixSeconds == 0U || unixSeconds >= record.expiresAtUnixSeconds) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }

    ControllerCredential credential;
    credential.domain = ControllerCredentialDomain::PlatformAccess;
    credential.ownershipGeneration = record.accessEpoch;
    credential.credentialVersion = record.credentialVersion;
    credential.permissions = record.permissions;
    credential.suite = ControllerCredentialSuite::HmacSha256_32;
    memcpy(
        credential.controllerId,
        record.controllerId,
        sizeof(credential.controllerId));
    memcpy(credential.secret, record.secret, sizeof(credential.secret));
    Result result = validateControllerCredential(credential);
    if (result) output = credential;
    clearControllerCredential(credential);
    return result;
}

Result GatewayAccessStore::encode(
    const GatewayAccessRecord& value,
    MutableByteSpan output) {
    if (!gateway_access_store_detail::validRecord(value, true)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, gateway_access_store_detail::kMagic,
           sizeof(gateway_access_store_detail::kMagic));
    output.data[4] = gateway_access_store_detail::kSchemaVersion;
    output.data[5] = value.childLogicalDeviceIdSize;
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kRevisionOffset,
        value.storageRevision);
    memcpy(output.data + gateway_access_store_detail::kOperationOffset,
           value.operationId,
           sizeof(value.operationId));
    memcpy(output.data + gateway_access_store_detail::kEnvelopeDigestOffset,
           value.envelopeDigest,
           sizeof(value.envelopeDigest));
    memcpy(output.data + gateway_access_store_detail::kAccessMaterialDigestOffset,
           value.accessMaterialDigest,
           sizeof(value.accessMaterialDigest));
    memcpy(output.data + gateway_access_store_detail::kChildInstanceOffset,
           value.childDeviceInstanceId,
           sizeof(value.childDeviceInstanceId));
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kAccessEpochOffset,
        value.accessEpoch);
    memcpy(output.data + gateway_access_store_detail::kControllerOffset,
           value.controllerId,
           sizeof(value.controllerId));
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kCredentialVersionOffset,
        value.credentialVersion);
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kPermissionsOffset,
        value.permissions);
    gateway_access_store_detail::writeU64(
        output.data + gateway_access_store_detail::kExpiresAtOffset,
        value.expiresAtUnixSeconds);
    memcpy(output.data + gateway_access_store_detail::kSecretOffset,
           value.secret, sizeof(value.secret));
    gateway_access_store_detail::writeU32(
        output.data +
            gateway_access_store_detail::kActivePresenceVersionOffset,
        value.activePresence.version);
    memcpy(
        output.data + gateway_access_store_detail::kActivePresenceKeyOffset,
        value.activePresence.key,
        sizeof(value.activePresence.key));
    gateway_access_store_detail::writeU32(
        output.data +
            gateway_access_store_detail::kPendingPresenceVersionOffset,
        value.pendingPresence.version);
    memcpy(
        output.data + gateway_access_store_detail::kPendingPresenceKeyOffset,
        value.pendingPresence.key,
        sizeof(value.pendingPresence.key));
    memcpy(output.data + gateway_access_store_detail::kChildLogicalIdOffset,
           value.childLogicalDeviceId,
           value.childLogicalDeviceIdSize);
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kDeliveryRevisionOffset,
        value.deliveryRevision);
    gateway_access_store_detail::writeU32(
        output.data + gateway_access_store_detail::kCrcOffset,
        computeCrc32(ByteView(
            output.data, gateway_access_store_detail::kCrcOffset)));
    return Result::success();
}

Result GatewayAccessStore::decode(
    ByteView input,
    GatewayAccessRecord& output) {
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, gateway_access_store_detail::kMagic,
               sizeof(gateway_access_store_detail::kMagic)) != 0 ||
        input.data[4] != gateway_access_store_detail::kSchemaVersion ||
        input.data[6] != 0U ||
        input.data[7] != 0U ||
        gateway_access_store_detail::readU32(
            input.data + gateway_access_store_detail::kCrcOffset) !=
            computeCrc32(ByteView(
                input.data, gateway_access_store_detail::kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const size_t idSize = input.data[5];
    if (idSize == 0U || idSize > gateway::kMaximumLogicalDeviceIdSize) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    for (size_t index = idSize;
         index < gateway::kMaximumLogicalDeviceIdSize; ++index) {
        if (input.data[
                gateway_access_store_detail::kChildLogicalIdOffset + index] !=
            0U) {
            return Result::failure(ErrorCode::NonCanonicalEncoding);
        }
    }

    GatewayAccessRecord decoded;
    decoded.storageRevision = gateway_access_store_detail::readU32(
        input.data + gateway_access_store_detail::kRevisionOffset);
    decoded.deliveryRevision = gateway_access_store_detail::readU32(
        input.data + gateway_access_store_detail::kDeliveryRevisionOffset);
    memcpy(decoded.operationId,
           input.data + gateway_access_store_detail::kOperationOffset,
           sizeof(decoded.operationId));
    memcpy(decoded.envelopeDigest,
           input.data + gateway_access_store_detail::kEnvelopeDigestOffset,
           sizeof(decoded.envelopeDigest));
    memcpy(decoded.accessMaterialDigest,
           input.data + gateway_access_store_detail::kAccessMaterialDigestOffset,
           sizeof(decoded.accessMaterialDigest));
    decoded.childLogicalDeviceIdSize = static_cast<uint8_t>(idSize);
    memcpy(decoded.childLogicalDeviceId,
           input.data + gateway_access_store_detail::kChildLogicalIdOffset,
           idSize);
    memcpy(decoded.childDeviceInstanceId,
           input.data + gateway_access_store_detail::kChildInstanceOffset,
           sizeof(decoded.childDeviceInstanceId));
    decoded.accessEpoch = gateway_access_store_detail::readU32(
        input.data + gateway_access_store_detail::kAccessEpochOffset);
    memcpy(decoded.controllerId,
           input.data + gateway_access_store_detail::kControllerOffset,
           sizeof(decoded.controllerId));
    decoded.credentialVersion = gateway_access_store_detail::readU32(
        input.data +
        gateway_access_store_detail::kCredentialVersionOffset);
    decoded.permissions = gateway_access_store_detail::readU32(
        input.data + gateway_access_store_detail::kPermissionsOffset);
    decoded.expiresAtUnixSeconds = gateway_access_store_detail::readU64(
        input.data + gateway_access_store_detail::kExpiresAtOffset);
    memcpy(decoded.secret,
           input.data + gateway_access_store_detail::kSecretOffset,
           sizeof(decoded.secret));
    decoded.activePresence.version = gateway_access_store_detail::readU32(
        input.data +
        gateway_access_store_detail::kActivePresenceVersionOffset);
    memcpy(
        decoded.activePresence.key,
        input.data + gateway_access_store_detail::kActivePresenceKeyOffset,
        sizeof(decoded.activePresence.key));
    decoded.pendingPresence.version = gateway_access_store_detail::readU32(
        input.data +
        gateway_access_store_detail::kPendingPresenceVersionOffset);
    memcpy(
        decoded.pendingPresence.key,
        input.data + gateway_access_store_detail::kPendingPresenceKeyOffset,
        sizeof(decoded.pendingPresence.key));
    if (!gateway_access_store_detail::validRecord(decoded, true)) {
        clearGatewayAccessRecord(decoded);
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    output = decoded;
    clearGatewayAccessRecord(decoded);
    return Result::success();
}

Result GatewayAccessStore::load(GatewayAccessRecord& output) {
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    GatewayAccessRecord decoded;
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    gateway_access_store_detail::StoredKind kind =
        gateway_access_store_detail::StoredKind::Active;
    if (result) {
        result = gateway_access_store_detail::storedKind(
            ByteView(encoded, written), kind);
    }
    if (result && kind ==
                      gateway_access_store_detail::StoredKind::Revocation) {
        GatewayRevocationTombstone tombstone;
        result = gateway_access_store_detail::decodeRevocation(
            ByteView(encoded, written), tombstone);
        clearGatewayRevocationTombstone(tombstone);
        if (result) result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) result = decode(ByteView(encoded, written), decoded);
    if (result) output = decoded;
    clearGatewayAccessRecord(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result GatewayAccessStore::commit(
    const GatewayAccessRecord& candidate,
    GatewayAccessCommitOutcome& outcome) {
    outcome = GatewayAccessCommitOutcome();
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!gateway_access_store_detail::validRecord(candidate, false)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    GatewayAccessRecord current;
    Result result = load(current);
    if (!result && result.code() != ErrorCode::NotFound) {
        clearGatewayAccessRecord(current);
        return result;
    }

    GatewayAccessRecord next = candidate;
    if (result) {
        const bool sameOperation = gateway_access_store_detail::equalBytes(
            current.operationId, candidate.operationId,
            sizeof(current.operationId));
        if (sameOperation) {
            if (!gateway_access_store_detail::sameControllerCredential(
                    current, candidate) ||
                !gateway_access_store_detail::deliveredPresenceMatches(
                    current, candidate) ||
                !gateway_access_store_detail::equalBytes(
                    current.accessMaterialDigest,
                    candidate.accessMaterialDigest,
                    sizeof(current.accessMaterialDigest))) {
                clearGatewayAccessRecord(current);
                clearGatewayAccessRecord(next);
                return Result::failure(ErrorCode::SequenceConflict);
            }
            outcome.storageRevision = current.deliveryRevision;
            if (gateway_access_store_detail::equalBytes(
                    current.envelopeDigest, candidate.envelopeDigest,
                    sizeof(current.envelopeDigest))) {
                outcome.kind = GatewayAccessCommitKind::ExactReplay;
                clearGatewayAccessRecord(current);
                clearGatewayAccessRecord(next);
                return Result::success();
            }
            next = current;
            memcpy(
                next.envelopeDigest,
                candidate.envelopeDigest,
                sizeof(next.envelopeDigest));
            outcome.kind = GatewayAccessCommitKind::RewrappedReplay;
        } else {
            if (!gateway_access_store_detail::sameChild(
                    current, candidate)) {
                clearGatewayAccessRecord(current);
                clearGatewayAccessRecord(next);
                return Result::failure(ErrorCode::CapacityExceeded);
            }
            if (candidate.accessEpoch < current.accessEpoch) {
                clearGatewayAccessRecord(current);
                clearGatewayAccessRecord(next);
                return Result::failure(ErrorCode::SequenceConflict);
            }
            if (current.storageRevision == UINT32_MAX) {
                clearGatewayAccessRecord(current);
                clearGatewayAccessRecord(next);
                return Result::failure(ErrorCode::CapacityExceeded);
            }
            if (candidate.accessEpoch == current.accessEpoch) {
                if (!gateway_access_store_detail::sameControllerCredential(
                        current, candidate) ||
                    current.pendingPresence.version != 0U ||
                    candidate.activePresence.version <=
                        current.activePresence.version) {
                    clearGatewayAccessRecord(current);
                    clearGatewayAccessRecord(next);
                    return Result::failure(ErrorCode::SequenceConflict);
                }
                next = current;
                memcpy(next.operationId, candidate.operationId,
                       sizeof(next.operationId));
                memcpy(next.envelopeDigest, candidate.envelopeDigest,
                       sizeof(next.envelopeDigest));
                memcpy(
                    next.accessMaterialDigest,
                    candidate.accessMaterialDigest,
                    sizeof(next.accessMaterialDigest));
                next.pendingPresence = candidate.activePresence;
            }
            next.storageRevision = current.storageRevision + 1U;
            next.deliveryRevision = next.storageRevision;
            outcome.kind = GatewayAccessCommitKind::Stored;
        }
    } else {
        next.storageRevision = 1U;
        next.deliveryRevision = 1U;
        outcome.kind = GatewayAccessCommitKind::Stored;
    }

    result = gateway_access_store_detail::replaceVerified(storage_, next);
    if (result) outcome.storageRevision = next.deliveryRevision;
    clearGatewayAccessRecord(current);
    clearGatewayAccessRecord(next);
    return result;
}

Result GatewayAccessStore::activatePendingPresence(
    uint32_t expectedVersion,
    uint32_t& storageRevision) {
    storageRevision = 0U;
    GatewayAccessRecord current;
    Result result = load(current);
    if (result &&
        (expectedVersion == 0U ||
         current.pendingPresence.version != expectedVersion)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        current.activePresence = current.pendingPresence;
        current.pendingPresence = GatewayPresenceLocator();
        ++current.storageRevision;
        result = gateway_access_store_detail::replaceVerified(
            storage_, current);
    }
    if (result) storageRevision = current.storageRevision;
    clearGatewayAccessRecord(current);
    return result;
}

Result GatewayAccessStore::discardPendingPresence(
    uint32_t expectedVersion,
    uint32_t& storageRevision) {
    storageRevision = 0U;
    GatewayAccessRecord current;
    Result result = load(current);
    if (result &&
        (expectedVersion == 0U ||
         current.pendingPresence.version != expectedVersion)) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    if (result) {
        current.pendingPresence = GatewayPresenceLocator();
        ++current.storageRevision;
        result = gateway_access_store_detail::replaceVerified(
            storage_, current);
    }
    if (result) storageRevision = current.storageRevision;
    clearGatewayAccessRecord(current);
    return result;
}

Result GatewayAccessStore::promoteCredentialRenewal(
    const GatewayCredentialRenewalRecord& pending,
    uint32_t& storageRevision) {
    storageRevision = 0U;
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    Result result = GatewayCredentialRenewalStore::validateDurable(pending);
    if (!result ||
        (pending.state != GatewayCredentialRenewalState::ReceiptStored &&
         pending.state != GatewayCredentialRenewalState::Promoted)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    GatewayAccessRecord current;
    result = load(current);
    const bool identityMatches = result &&
        gateway_access_store_detail::equalBytes(
            current.operationId, pending.topologyOperationId,
            sizeof(current.operationId)) &&
        gateway_access_store_detail::equalBytes(
            current.childDeviceInstanceId,
            pending.childDeviceInstanceId,
            sizeof(current.childDeviceInstanceId)) &&
        current.accessEpoch == pending.accessEpoch &&
        gateway_access_store_detail::equalBytes(
            current.controllerId, pending.controllerId,
            sizeof(current.controllerId)) &&
        current.activePresence.version == pending.presenceKeyVersion &&
        current.pendingPresence.version == 0U &&
        gateway_access_store_detail::zero(ByteView(
            current.pendingPresence.key,
            sizeof(current.pendingPresence.key)));
    if (!identityMatches) {
        clearGatewayAccessRecord(current);
        return result ? Result::failure(ErrorCode::SequenceConflict) : result;
    }

    if (current.credentialVersion == pending.credentialVersion) {
        const bool exactReplay =
            current.expiresAtUnixSeconds == pending.expiresAtUnixSeconds &&
            gateway_access_store_detail::equalBytes(
                current.secret, pending.secret, sizeof(current.secret)) &&
            gateway_access_store_detail::equalBytes(
                current.accessMaterialDigest,
                pending.accessMaterialDigest,
                sizeof(current.accessMaterialDigest));
        if (!exactReplay) {
            clearGatewayAccessRecord(current);
            return Result::failure(ErrorCode::SequenceConflict);
        }
        storageRevision = current.deliveryRevision;
        clearGatewayAccessRecord(current);
        return Result::success();
    }

    if (current.credentialVersion != pending.expectedCredentialVersion) {
        clearGatewayAccessRecord(current);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    if (current.storageRevision == UINT32_MAX) {
        clearGatewayAccessRecord(current);
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    ++current.storageRevision;
    current.deliveryRevision = current.storageRevision;
    current.credentialVersion = pending.credentialVersion;
    current.expiresAtUnixSeconds = pending.expiresAtUnixSeconds;
    memcpy(current.secret, pending.secret, sizeof(current.secret));
    memcpy(current.accessMaterialDigest, pending.accessMaterialDigest,
           sizeof(current.accessMaterialDigest));
    secureZero(MutableByteSpan(
        current.envelopeDigest, sizeof(current.envelopeDigest)));
    result = gateway_access_store_detail::replaceVerified(storage_, current);
    if (result) storageRevision = current.deliveryRevision;
    clearGatewayAccessRecord(current);
    return result;
}

Result GatewayAccessStore::loadRevocation(
    GatewayRevocationTombstone& output) {
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    gateway_access_store_detail::StoredKind kind =
        gateway_access_store_detail::StoredKind::Active;
    if (result) {
        result = gateway_access_store_detail::storedKind(
            ByteView(encoded, written), kind);
    }
    if (result && kind ==
                      gateway_access_store_detail::StoredKind::Active) {
        GatewayAccessRecord active;
        result = decode(ByteView(encoded, written), active);
        clearGatewayAccessRecord(active);
        if (result) result = Result::failure(ErrorCode::NotFound);
    }
    GatewayRevocationTombstone decoded;
    if (result) {
        result = gateway_access_store_detail::decodeRevocation(
            ByteView(encoded, written), decoded);
    }
    if (result) output = decoded;
    clearGatewayRevocationTombstone(decoded);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result GatewayAccessStore::commitRevocation(
    uint32_t expectedDeliveryRevision,
    const GatewayRevocationTombstone& candidate,
    uint32_t& storageRevision) {
    storageRevision = 0U;
    if (storage_.protection() == StorageProtection::PlainFlash) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (expectedDeliveryRevision == 0U ||
        !gateway_access_store_detail::validRevocation(candidate, false)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (!result) {
        secureZero(MutableByteSpan(encoded, sizeof(encoded)));
        return result;
    }
    if (written != sizeof(encoded)) {
        secureZero(MutableByteSpan(encoded, sizeof(encoded)));
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    gateway_access_store_detail::StoredKind kind =
        gateway_access_store_detail::StoredKind::Active;
    result = gateway_access_store_detail::storedKind(
        ByteView(encoded, written), kind);

    GatewayRevocationTombstone currentTombstone;
    if (result && kind ==
                      gateway_access_store_detail::StoredKind::Revocation) {
        result = gateway_access_store_detail::decodeRevocation(
            ByteView(encoded, written), currentTombstone);
        GatewayRevocationTombstone replay = candidate;
        replay.storageRevision = currentTombstone.storageRevision;
        if (result && !gateway_access_store_detail::sameRevocation(
                          replay, currentTombstone)) {
            result = Result::failure(ErrorCode::SequenceConflict);
        }
        if (result) storageRevision = currentTombstone.storageRevision;
        clearGatewayRevocationTombstone(replay);
        clearGatewayRevocationTombstone(currentTombstone);
        secureZero(MutableByteSpan(encoded, sizeof(encoded)));
        return result;
    }

    GatewayAccessRecord current;
    if (result) result = decode(ByteView(encoded, written), current);
    const bool matches = result &&
        current.deliveryRevision == expectedDeliveryRevision &&
        gateway_access_store_detail::equalBytes(
            current.operationId, candidate.operationId,
            sizeof(current.operationId));
    if (result && !matches) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }
    if (result && current.storageRevision == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    GatewayRevocationTombstone next = candidate;
    if (result) {
        next.storageRevision = current.storageRevision + 1U;
        result = gateway_access_store_detail::replaceRevocationVerified(
            storage_, next);
    }
    if (result) storageRevision = next.storageRevision;
    clearGatewayAccessRecord(current);
    clearGatewayRevocationTombstone(next);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result GatewayAccessStore::clearRevocation(
    ByteView operationId,
    uint32_t topologyVersion,
    ByteView receiptDigest) {
    if (operationId.data == nullptr ||
        operationId.size != gateway::kOperationIdSize ||
        topologyVersion == 0U || receiptDigest.data == nullptr ||
        receiptDigest.size != gateway::kAccessMaterialDigestSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    GatewayRevocationTombstone current;
    Result result = loadRevocation(current);
    if (!result && result.code() == ErrorCode::NotFound) {
        // An active record is not an idempotent completion; loadRevocation()
        // uses NotFound for both active and empty, so disambiguate by load().
        GatewayAccessRecord active;
        const Result activeResult = load(active);
        clearGatewayAccessRecord(active);
        if (activeResult) return Result::failure(ErrorCode::StateConflict);
        if (activeResult.code() == ErrorCode::NotFound) {
            return Result::success();
        }
        return activeResult;
    }
    if (!result) {
        clearGatewayRevocationTombstone(current);
        return result;
    }
    const bool matches = current.topologyVersion == topologyVersion &&
        gateway_access_store_detail::equalBytes(
            current.operationId, operationId.data,
            sizeof(current.operationId)) &&
        gateway_access_store_detail::equalBytes(
            current.receiptDigest, receiptDigest.data,
            sizeof(current.receiptDigest));
    clearGatewayRevocationTombstone(current);
    if (!matches) return Result::failure(ErrorCode::SequenceConflict);
    result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

Result GatewayAccessStore::finalizeRevocation(
    ByteView operationId,
    uint32_t topologyVersion,
    ByteView receiptDigest,
    ByteView childDeviceInstanceId,
    uint32_t accessEpoch,
    ByteView controllerId,
    uint32_t credentialVersion,
    uint32_t deliveryRevision) {
    if (operationId.data == nullptr ||
        operationId.size != gateway::kOperationIdSize ||
        topologyVersion == 0U || receiptDigest.data == nullptr ||
        receiptDigest.size != gateway::kAccessMaterialDigestSize ||
        childDeviceInstanceId.data == nullptr ||
        childDeviceInstanceId.size != gateway::kDeviceInstanceIdSize ||
        accessEpoch == 0U || controllerId.data == nullptr ||
        controllerId.size != gateway::kControllerIdSize ||
        credentialVersion == 0U || deliveryRevision == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    GatewayRevocationTombstone tombstone;
    const Result tombstoneResult = loadRevocation(tombstone);
    clearGatewayRevocationTombstone(tombstone);
    if (tombstoneResult) {
        return clearRevocation(
            operationId, topologyVersion, receiptDigest);
    }
    if (tombstoneResult.code() != ErrorCode::NotFound) {
        return tombstoneResult;
    }

    GatewayAccessRecord active;
    Result result = load(active);
    if (!result) {
        clearGatewayAccessRecord(active);
        return result.code() == ErrorCode::NotFound
                   ? Result::success()
                   : result;
    }
    const bool matches =
        active.deliveryRevision == deliveryRevision &&
        active.accessEpoch == accessEpoch &&
        active.credentialVersion == credentialVersion &&
        gateway_access_store_detail::equalBytes(
            active.operationId, operationId.data,
            sizeof(active.operationId)) &&
        gateway_access_store_detail::equalBytes(
            active.childDeviceInstanceId, childDeviceInstanceId.data,
            sizeof(active.childDeviceInstanceId)) &&
        gateway_access_store_detail::equalBytes(
            active.controllerId, controllerId.data,
            sizeof(active.controllerId));
    clearGatewayAccessRecord(active);
    if (!matches) return Result::failure(ErrorCode::SequenceConflict);
    result = storage_.clear();
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : result;
}

Result GatewayAccessStore::clear() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound ? Result::success() : result;
}

} // namespace blinker
