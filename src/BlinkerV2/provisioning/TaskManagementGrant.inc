#include "TaskManagementGrant.h"

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {
namespace task_grant_detail {

bool exact(ByteView value, size_t size) {
    return value.data != nullptr && value.size == size;
}

bool nonZeroId(ByteView value) {
    if (!exact(value, kTaskGrantIdSize)) return false;
    uint8_t bits = 0U;
    for (size_t i = 0U; i < value.size; ++i) bits |= value.data[i];
    return bits != 0U;
}

bool validBinding(const TaskGrantBinding& value) {
    return nonZeroId(value.deviceInstanceId) && nonZeroId(value.taskScope) &&
           nonZeroId(value.nonce) && nonZeroId(value.requestId) &&
           exact(value.operationDigest, kTaskGrantDigestSize);
}

bool validUnsigned(const TaskManagementGrant& value) {
    return validBinding(value.binding) && value.serverKeyId != 0U &&
           (value.signatureAlgorithm == ServerSignatureAlgorithm::Ed25519 ||
            value.signatureAlgorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw);
}

Result writeUnsignedFields(cbor::Writer& writer, const TaskManagementGrant& value) {
    Result result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(value.binding.deviceInstanceId);
    if (result) result = writer.writeBytes(value.binding.taskScope);
    if (result) result = writer.writeBytes(value.binding.nonce);
    if (result) result = writer.writeBytes(value.binding.requestId);
    if (result) result = writer.writeBytes(value.binding.operationDigest);
    if (result) result = writer.writeUnsigned(value.serverKeyId);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.signatureAlgorithm));
    return result;
}

Result hashSink(void* context, ByteView bytes) {
    return static_cast<Sha256*>(context)->update(bytes);
}

bool matches(const TaskGrantBinding& value, const TaskGrantBinding& expected) {
    return constantTimeEqual(value.deviceInstanceId, expected.deviceInstanceId) &&
           constantTimeEqual(value.taskScope, expected.taskScope) &&
           constantTimeEqual(value.nonce, expected.nonce) &&
           constantTimeEqual(value.requestId, expected.requestId) &&
           constantTimeEqual(value.operationDigest, expected.operationDigest);
}

} // namespace task_grant_detail

Result encodeTaskManagementGrant(
    const TaskManagementGrant& grant, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (!task_grant_detail::validUnsigned(grant) ||
        !task_grant_detail::exact(grant.signature, kTaskGrantSignatureSize) ||
        output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    Result result = writer.beginArray(9U);
    if (result) result = task_grant_detail::writeUnsignedFields(writer, grant);
    if (result) result = writer.writeBytes(grant.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeTaskManagementGrant(ByteView encoded, TaskManagementGrant& grant) {
    grant = TaskManagementGrant();
    if (encoded.data == nullptr || encoded.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (encoded.size > kTaskManagementGrantMaxSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Limits limits;
    limits.maxTextLength = 0U;
    limits.maxByteStringLength = kTaskGrantSignatureSize;
    limits.maxContainerItems = 9U;
    limits.maxDepth = 1U;
    Result result = cbor::validate(encoded, limits);
    if (!result) return result;
    cbor::Reader reader(encoded, limits);
    size_t count = 0U;
    result = reader.readArraySize(count);
    if (!result) return result;
    if (count != 9U) return Result::failure(ErrorCode::InvalidEncoding);
    uint64_t version = 0U;
    result = reader.readUnsigned(version);
    if (!result) return result;
    if (version != 1U) return Result::failure(ErrorCode::UnsupportedVersion);
    TaskManagementGrant decoded;
    result = reader.readBytes(decoded.binding.deviceInstanceId);
    if (result) result = reader.readBytes(decoded.binding.taskScope);
    if (result) result = reader.readBytes(decoded.binding.nonce);
    if (result) result = reader.readBytes(decoded.binding.requestId);
    if (result) result = reader.readBytes(decoded.binding.operationDigest);
    uint64_t keyId = 0U, algorithm = 0U;
    if (result) result = reader.readUnsigned(keyId);
    if (result) result = reader.readUnsigned(algorithm);
    if (result) result = reader.readBytes(decoded.signature);
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    if (keyId == 0U || keyId > UINT32_MAX || (algorithm != 1U && algorithm != 2U)) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    decoded.serverKeyId = static_cast<uint32_t>(keyId);
    decoded.signatureAlgorithm = static_cast<ServerSignatureAlgorithm>(algorithm);
    if (!task_grant_detail::validUnsigned(decoded) ||
        !task_grant_detail::exact(decoded.signature, kTaskGrantSignatureSize)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    grant = decoded;
    return Result::success();
}

Result taskManagementGrantDigest(const TaskManagementGrant& grant, MutableByteSpan digest) {
    if (digest.data == nullptr || digest.size != kTaskGrantDigestSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(digest);
    if (!task_grant_detail::validUnsigned(grant)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Sha256 hash;
    cbor::Writer writer(task_grant_detail::hashSink, &hash);
    Result result = writer.beginArray(9U);
    if (result) result = writer.writeText(StringView("blinker.task-grant.v1"));
    if (result) result = task_grant_detail::writeUnsignedFields(writer, grant);
    if (result) result = hash.finish(digest);
    if (!result) secureZero(digest);
    return result;
}

Result verifyTaskManagementGrant(
    ByteView encoded, const TaskGrantBinding& expected,
    IServerSignatureVerifier& verifier, TaskManagementGrant& grant) {
    grant = TaskManagementGrant();
    if (!task_grant_detail::validBinding(expected)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    TaskManagementGrant decoded;
    Result result = decodeTaskManagementGrant(encoded, decoded);
    if (!result) return result;
    if (!task_grant_detail::matches(decoded.binding, expected)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    uint8_t digest[kTaskGrantDigestSize] = {};
    result = taskManagementGrantDigest(decoded, MutableByteSpan(digest, sizeof(digest)));
    if (result) result = verifier.verifyDigest(
        decoded.serverKeyId, decoded.signatureAlgorithm,
        ByteView(digest, sizeof(digest)), decoded.signature);
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    if (result) grant = decoded;
    return result;
}

} // namespace blinker
