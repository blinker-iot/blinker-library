#include "LocalSetupSagaStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'S', 'G', '2'};
const uint8_t kSchemaVersion = 1U;
const size_t kTicketIdOffset = 20U;
const size_t kControllerIdOffset = 36U;
const size_t kControllerSecretDigestOffset = 52U;
const size_t kClaimRequestIdOffset = 84U;
const size_t kClaimNonceOffset = 100U;
const size_t kClaimCommitAckDigestOffset = 116U;
const size_t kControllerInstallDigestOffset = 148U;
const size_t kNetworkRequestDigestOffset = 180U;
const size_t kCloudCommitAckDigestOffset = 212U;
const size_t kCrcOffset = LocalSetupSagaStore::serializedSize - 4U;

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

} // namespace

Result LocalSetupSagaStore::encode(const LocalSetupSagaRecord& record,
                                   MutableByteSpan output) {
    Result result = validateLocalSetupSagaRecord(record);
    if (!result) return result;
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(record.state);
    writeU32(output.data + 8U, record.ownershipGeneration);
    writeU32(output.data + 12U, record.controllerPermissions);
    writeU32(output.data + 16U, record.authorizedOperations);
    memcpy(output.data + kTicketIdOffset, record.ticketId, sizeof(record.ticketId));
    memcpy(output.data + kControllerIdOffset, record.controllerId, sizeof(record.controllerId));
    memcpy(output.data + kControllerSecretDigestOffset,
           record.controllerSecretDigest,
           sizeof(record.controllerSecretDigest));
    memcpy(output.data + kClaimRequestIdOffset,
           record.claimRequestId,
           sizeof(record.claimRequestId));
    memcpy(output.data + kClaimNonceOffset, record.claimNonce, sizeof(record.claimNonce));
    memcpy(output.data + kClaimCommitAckDigestOffset,
           record.claimCommitAckDigest,
           sizeof(record.claimCommitAckDigest));
    memcpy(output.data + kControllerInstallDigestOffset,
           record.controllerInstallDigest,
           sizeof(record.controllerInstallDigest));
    memcpy(output.data + kNetworkRequestDigestOffset,
           record.networkRequestDigest,
           sizeof(record.networkRequestDigest));
    memcpy(output.data + kCloudCommitAckDigestOffset,
           record.cloudCommitAckDigest,
           sizeof(record.cloudCommitAckDigest));
    writeU32(output.data + kCrcOffset,
             computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result LocalSetupSagaStore::decode(ByteView input,
                                   LocalSetupSagaRecord& record) {
    record = LocalSetupSagaRecord();
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion || input.data[6] != 0U ||
        input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    LocalSetupSagaRecord decoded;
    decoded.state = static_cast<LocalSetupSagaState>(input.data[5]);
    decoded.ownershipGeneration = readU32(input.data + 8U);
    decoded.controllerPermissions = readU32(input.data + 12U);
    decoded.authorizedOperations = readU32(input.data + 16U);
    memcpy(decoded.ticketId, input.data + kTicketIdOffset, sizeof(decoded.ticketId));
    memcpy(decoded.controllerId, input.data + kControllerIdOffset, sizeof(decoded.controllerId));
    memcpy(decoded.controllerSecretDigest,
           input.data + kControllerSecretDigestOffset,
           sizeof(decoded.controllerSecretDigest));
    memcpy(decoded.claimRequestId,
           input.data + kClaimRequestIdOffset,
           sizeof(decoded.claimRequestId));
    memcpy(decoded.claimNonce, input.data + kClaimNonceOffset, sizeof(decoded.claimNonce));
    memcpy(decoded.claimCommitAckDigest,
           input.data + kClaimCommitAckDigestOffset,
           sizeof(decoded.claimCommitAckDigest));
    memcpy(decoded.controllerInstallDigest,
           input.data + kControllerInstallDigestOffset,
           sizeof(decoded.controllerInstallDigest));
    memcpy(decoded.networkRequestDigest,
           input.data + kNetworkRequestDigestOffset,
           sizeof(decoded.networkRequestDigest));
    memcpy(decoded.cloudCommitAckDigest,
           input.data + kCloudCommitAckDigestOffset,
           sizeof(decoded.cloudCommitAckDigest));
    Result result = validateLocalSetupSagaRecord(decoded);
    if (result) record = decoded;
    clearLocalSetupSagaRecord(decoded);
    return result;
}

Result LocalSetupSagaStore::load(LocalSetupSagaRecord& output) {
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = decode(ByteView(encoded, written), output);
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result LocalSetupSagaStore::save(const LocalSetupSagaRecord& record) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(record, MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;
    LocalSetupSagaRecord verified;
    result = load(verified);
    const bool same = result && sameLocalSetupSagaRecord(record, verified);
    clearLocalSetupSagaRecord(verified);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result LocalSetupSagaStore::clear() {
    Result result = storage_.clear();
    return result.code() == ErrorCode::NotFound ? Result::success() : result;
}

} // namespace blinker
