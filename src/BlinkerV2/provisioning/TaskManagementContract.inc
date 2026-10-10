#include "TaskManagementContract.h"
#include <string.h>
#include "../core/HmacSha256.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {
namespace task_contract_detail {
bool nonzero(ByteView bytes, size_t size) {
    if (bytes.data == nullptr || bytes.size != size) return false;
    uint8_t bits = 0U;
    for (size_t i = 0U; i < size; ++i) bits |= bytes.data[i];
    return bits != 0U;
}
cbor::Limits limits() {
    cbor::Limits value;
    value.maxTextLength = kMaxEndpointKeyLength; value.maxByteStringLength = 32U;
    value.maxContainerItems = 10U; value.maxDepth = 5U;
    return value;
}
Result validateInput(ByteView input, size_t maximum) {
    if (input.data == nullptr || input.empty()) return Result::failure(ErrorCode::InvalidArgument);
    if (input.size > maximum) return Result::failure(ErrorCode::CapacityExceeded);
    return cbor::validate(input, limits());
}
Result array(cbor::Reader& reader, size_t expected) {
    size_t count = 0U;
    const Result result = reader.readArraySize(count);
    if (!result) return result;
    return count == expected ? Result::success() : Result::failure(ErrorCode::InvalidEncoding);
}
template<typename T> Result number(cbor::Reader& reader, T& value, uint64_t maximum) {
    uint64_t decoded = 0U;
    const Result result = reader.readUnsigned(decoded);
    if (!result) return result;
    if (decoded > maximum) return Result::failure(ErrorCode::ValueOutOfRange);
    value = static_cast<T>(decoded);
    return Result::success();
}
Result version(cbor::Reader& reader) {
    uint64_t value = 0U;
    const Result result = reader.readUnsigned(value);
    if (!result) return result;
    return value == 1U ? Result::success() : Result::failure(ErrorCode::UnsupportedVersion);
}
Result id(cbor::Reader& reader, uint8_t* output, size_t size) {
    ByteView bytes;
    const Result result = reader.readBytes(bytes);
    if (!result) return result;
    if (!nonzero(bytes, size)) return Result::failure(ErrorCode::InvalidEncoding);
    memcpy(output, bytes.data, size);
    return Result::success();
}
Result definition(cbor::Writer& writer, const TaskSnapshotRecord& value) {
    Result result = writer.beginArray(10U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.timing.kind));
    if (result) result = writer.writeUnsigned(value.timing.utcMillis);
    if (result) result = writer.writeUnsigned(value.timing.durationMillis);
    if (result) result = writer.writeUnsigned(value.timing.minuteOfDay);
    if (result) result = writer.writeInt(value.timing.utcOffsetMinutes);
    if (result) result = writer.writeUnsigned(value.timing.weekdays);
    if (result) result = writer.writeBool(value.enabled);
    if (result) result = writer.writeText(StringView(reinterpret_cast<const char*>(value.key), value.keyLength));
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.valueType));
    if (result) result = writer.writeEncodedValue(ByteView(value.value, value.valueLength), limits());
    return result;
}
Result definition(cbor::Reader& reader, TaskSnapshotRecord& value) {
    Result result = array(reader, 10U);
    uint8_t kind = 0U, type = 0U;
    if (result) result = number(reader, kind, static_cast<uint8_t>(TaskTimingKind::Weekly));
    value.timing.kind = static_cast<TaskTimingKind>(kind);
    if (result) result = reader.readUnsigned(value.timing.utcMillis);
    if (result) result = number(reader, value.timing.durationMillis, UINT32_MAX);
    if (result) result = number(reader, value.timing.minuteOfDay, UINT16_MAX);
    int64_t offset = 0;
    if (result) result = reader.readInt(offset);
    if (result && (offset < INT16_MIN || offset > INT16_MAX)) result = Result::failure(ErrorCode::ValueOutOfRange);
    if (result) value.timing.utcOffsetMinutes = static_cast<int16_t>(offset);
    if (result) result = number(reader, value.timing.weekdays, UINT8_MAX);
    if (result) result = reader.readBool(value.enabled);
    StringView key; ByteView scalar;
    if (result) result = reader.readText(key);
    if (result) result = number(reader, type, static_cast<uint8_t>(ValueType::Float64));
    if (result) result = reader.captureValue(scalar);
    if (!result) return result;
    if (key.size == 0U || key.size > sizeof(value.key) || scalar.size > sizeof(value.value))
        return Result::failure(ErrorCode::InvalidEncoding);
    memcpy(value.key, key.data, key.size); value.keyLength = static_cast<uint8_t>(key.size);
    memcpy(value.value, scalar.data, scalar.size); value.valueLength = static_cast<uint8_t>(scalar.size);
    value.valueType = static_cast<ValueType>(type);
    return Result::success(); // Caller reuses Engine/Store semantic validation.
}
Result mutation(cbor::Writer& writer, const TaskMutation& request) {
    Result result = writer.beginArray(7U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(ByteView(request.requestId, 16U));
    if (result) result = writer.writeUnsigned(request.expectedRevision);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(request.kind));
    if (result) result = writer.writeUnsigned(request.taskId);
    if (result) result = writer.writeBool(request.enabled);
    if (result) result = request.kind == TaskMutationKind::Create || request.kind == TaskMutationKind::Replace
        ? definition(writer, request.definition) : writer.writeNull();
    return result;
}
bool validRead(const TaskReadPrepare& request) {
    if (!nonzero(ByteView(request.requestId, 16U), 16U) ||
        (request.index == UINT32_MAX && request.expectedRevision != 0U)) return false;
    if (!request.catalog) return true;
    return request.expectedRevision == 0U && (request.index == UINT32_MAX ||
        (request.index < kTaskMaximumTargets && nonzero(ByteView(request.catalogFingerprint, 32U), 32U)));
}
Result read(cbor::Writer& writer, const TaskReadPrepare& request) {
    Result result = writer.beginArray(request.catalog ? 5U : 4U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(ByteView(request.requestId, 16U));
    if (result) result = writer.writeUnsigned(request.expectedRevision);
    if (result) result = writer.writeUnsigned(request.index);
    if (result && request.catalog) result = writer.writeBytes(ByteView(request.catalogFingerprint, request.index == UINT32_MAX ? 0U : 32U));
    return result;
}
Result sink(void* context, ByteView bytes) { return static_cast<Sha256*>(context)->update(bytes); }
Result startDigest(cbor::Writer& writer, StringView domain, ByteView root, MutableByteSpan digest) {
    if (digest.data == nullptr || digest.size != 32U) return Result::failure(ErrorCode::InvalidArgument);
    secureZero(digest);
    if (!nonzero(root, 32U)) return Result::failure(ErrorCode::InvalidArgument);
    Result result = writer.beginArray(3U);
    if (result) result = writer.writeText(domain);
    if (result) result = writer.writeBytes(root);
    return result;
}
bool validChallenge(const TaskManagementChallenge& value) {
    return value.authority.valid() && nonzero(ByteView(value.nonce, 16U), 16U) &&
        nonzero(ByteView(value.requestId, 16U), 16U) && nonzero(ByteView(value.operationDigest, 32U), 32U);
}
Result challenge(cbor::Writer& writer, const TaskManagementChallenge& value) {
    Result result = writer.beginArray(7U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(ByteView(value.authority.deviceInstanceId, 16U));
    if (result) result = writer.writeBytes(ByteView(value.authority.taskScope, 16U));
    if (result) result = writer.writeBytes(ByteView(value.authority.rootDigest, 32U));
    if (result) result = writer.writeBytes(ByteView(value.nonce, 16U));
    if (result) result = writer.writeBytes(ByteView(value.requestId, 16U));
    if (result) result = writer.writeBytes(ByteView(value.operationDigest, 32U));
    return result;
}
bool validResult(const TaskManagementResult& value) {
    if (value.kind > TaskManagementResultKind::Rejected || value.count > TaskSnapshotCodec::maxTasks ||
        (value.tableRevision == 0U && value.count != 0U) ||
        value.hasRecord != (value.kind == TaskManagementResultKind::Record)) return false;
    if (value.kind == TaskManagementResultKind::Rejected)
        return value.index == UINT32_MAX && (value.rejection == TaskMutationRejection::WriteBudget ||
            value.rejection == TaskMutationRejection::StartCooldown);
    if (value.rejection != TaskMutationRejection::None) return false;
    if (value.kind == TaskManagementResultKind::Record) {
        if (value.index >= value.count) return false;
        TaskSnapshotHeader header;
        header.tableRevision = value.tableRevision; header.nextTaskId = UINT32_MAX;
        return TaskSnapshotCodec::validateRecord(value.record, header).ok();
    }
    if (value.kind == TaskManagementResultKind::Catalog || value.kind == TaskManagementResultKind::Target) {
        const TaskTargetCatalogPage& c = value.catalog;
        if (!nonzero(ByteView(c.fingerprint, 32U), 32U) || c.capacity == 0U || c.capacity > TaskSnapshotCodec::maxTasks ||
            value.count > c.capacity || c.total > kTaskMaximumTargets) return false;
        if (value.kind == TaskManagementResultKind::Catalog) return value.index == UINT32_MAX && c.keyLength == 0U;
        return value.index < c.total && c.keyLength > 0U && c.keyLength <= kMaxEndpointKeyLength &&
            memchr(c.key, 0, c.keyLength) == nullptr && c.valueType <= ValueType::Float64;
    }
    if (value.index != UINT32_MAX) return false;
    if (value.kind == TaskManagementResultKind::Retired)
        return value.count == 0U && nonzero(ByteView(value.retirementId, 16U), 16U);
    if (value.kind == TaskManagementResultKind::Mutation)
        return value.tableRevision != 0U && value.mutation.tableRevision == value.tableRevision &&
            value.mutation.taskId != 0U && value.mutation.taskId < UINT32_MAX;
    return true;
}
bool validRole(TaskResultKey role) { return role == TaskResultKey::DeviceKey || role == TaskResultKey::Admin; }
Result resultFields(cbor::Writer& writer, const TaskManagementChallenge& context,
    const TaskManagementResult& value, TaskResultKey role) {
    Result result = writer.writeUnsigned(1U);
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(role));
    if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.kind));
    if (result) result = writer.writeBytes(ByteView(context.requestId, 16U));
    if (result) result = writer.writeUnsigned(value.tableRevision);
    if (result) result = writer.writeUnsigned(value.count);
    if (value.kind == TaskManagementResultKind::Mutation) {
        if (result) result = writer.beginArray(2U);
        if (result) result = writer.writeUnsigned(value.mutation.taskId);
        if (result) result = writer.writeBool(value.mutation.replayed);
    } else if (value.kind == TaskManagementResultKind::Record) {
        if (result) result = writer.beginArray(2U);
        if (result) result = writer.writeUnsigned(value.index);
        if (result) result = writer.beginArray(6U);
        if (result) result = writer.writeUnsigned(value.record.taskId);
        if (result) result = writer.writeUnsigned(value.record.definitionRevision);
        if (result) result = writer.writeUnsigned(value.record.consumedUtcMillis);
        if (result) result = writer.writeUnsigned(value.record.startSequence);
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.record.progress));
        if (result) result = definition(writer, value.record);
    } else if (value.kind == TaskManagementResultKind::Catalog || value.kind == TaskManagementResultKind::Target) {
        const TaskTargetCatalogPage& c = value.catalog;
        if (result) result = writer.beginArray(value.kind == TaskManagementResultKind::Target ? 6U : 3U);
        if (result) result = writer.writeUnsigned(c.capacity);
        if (result) result = writer.writeUnsigned(c.total);
        if (result) result = writer.writeBytes(ByteView(c.fingerprint, 32U));
        if (value.kind == TaskManagementResultKind::Target) {
            if (result) result = writer.writeUnsigned(value.index);
            if (result) result = writer.writeText(StringView(reinterpret_cast<const char*>(c.key), c.keyLength));
            if (result) result = writer.writeUnsigned(static_cast<uint8_t>(c.valueType));
        }
    } else if (value.kind == TaskManagementResultKind::Rejected) {
        if (result) result = writer.writeUnsigned(static_cast<uint8_t>(value.rejection));
    } else if (value.kind == TaskManagementResultKind::Retired) {
        if (result) result = writer.writeBytes(ByteView(value.retirementId, 16U));
    } else if (result) result = writer.writeNull();
    return result;
}
Result macSink(void* context, ByteView bytes) { return static_cast<HmacSha256*>(context)->update(bytes); }
Result proof(const TaskManagementChallenge& context, const TaskManagementResult& value,
    TaskResultKey role, ByteView secret, MutableByteSpan output) {
    if (!validChallenge(context) || !validResult(value) || !validRole(role) || !nonzero(secret, 32U))
        return Result::failure(ErrorCode::InvalidArgument);
    HmacSha256 mac;
    Result result = mac.begin(secret);
    cbor::Writer writer(macSink, &mac);
    if (result) result = writer.beginArray(3U);
    if (result) result = writer.writeText(StringView("blinker.task-result.v1"));
    if (result) result = challenge(writer, context);
    if (result) result = writer.beginArray(7U);
    if (result) result = resultFields(writer, context, value, role);
    return result ? mac.finish(output) : result;
}
Result retirement(cbor::Writer& writer, const TaskRetirementPrepare& value) {
    if (!value.authority.valid() || !nonzero(ByteView(value.requestId, 16U), 16U))
        return Result::failure(ErrorCode::InvalidArgument);
    Result result = writer.beginArray(5U);
    if (result) result = writer.writeUnsigned(1U);
    if (result) result = writer.writeBytes(ByteView(value.requestId, 16U));
    if (result) result = writer.writeBytes(ByteView(value.authority.deviceInstanceId, 16U));
    if (result) result = writer.writeBytes(ByteView(value.authority.taskScope, 16U));
    if (result) result = writer.writeBytes(ByteView(value.authority.rootDigest, 32U));
    return result;
}
} // namespace task_contract_detail

Result encodeTaskRetirementPrepare(const TaskRetirementPrepare& value, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    const Result result = task_contract_detail::retirement(writer, value);
    if (result) encoded = writer.view();
    return result;
}
Result decodeTaskRetirementPrepare(ByteView encoded, TaskRetirementPrepare& value) {
    value = TaskRetirementPrepare();
    Result result = task_contract_detail::validateInput(encoded, kTaskRetirementPrepareMaxSize);
    if (!result) return result;
    TaskRetirementPrepare decoded;
    cbor::Reader reader(encoded, task_contract_detail::limits());
    result = task_contract_detail::array(reader, 5U);
    if (result) result = task_contract_detail::version(reader);
    if (result) result = task_contract_detail::id(reader, decoded.requestId, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.authority.deviceInstanceId, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.authority.taskScope, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.authority.rootDigest, 32U);
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    value = decoded; return Result::success();
}
Result taskRetirementPrepareDigest(const TaskRetirementPrepare& value, bool query, MutableByteSpan digest) {
    if (digest.data == nullptr || digest.size != 32U) return Result::failure(ErrorCode::InvalidArgument);
    secureZero(digest);
    Sha256 hash; cbor::Writer writer(task_contract_detail::sink, &hash);
    Result result = writer.beginArray(2U);
    if (result) result = writer.writeText(query ? StringView("blinker.task-retirement-read.v1") : StringView("blinker.task-retire.v1"));
    if (result) result = task_contract_detail::retirement(writer, value);
    return result ? hash.finish(digest) : result;
}

Result encodeTaskMutationPrepare(const TaskMutation& request, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    Result result = validateTaskMutation(request);
    if (!result) return result;
    if (output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    result = task_contract_detail::mutation(writer, request);
    if (result) encoded = writer.view();
    return result;
}

Result decodeTaskMutationPrepare(ByteView encoded, TaskMutation& request) {
    request = TaskMutation();
    Result result = task_contract_detail::validateInput(encoded, kTaskMutationPrepareMaxSize);
    if (!result) return result;
    cbor::Reader reader(encoded, task_contract_detail::limits());
    TaskMutation decoded;
    result = task_contract_detail::array(reader, 7U);
    if (result) result = task_contract_detail::version(reader);
    if (result) result = task_contract_detail::id(reader, decoded.requestId, 16U);
    if (result) result = task_contract_detail::number(reader, decoded.expectedRevision, UINT32_MAX);
    uint8_t operation = 0U;
    if (result) result = task_contract_detail::number(reader, operation, static_cast<uint8_t>(TaskMutationKind::Start));
    decoded.kind = static_cast<TaskMutationKind>(operation);
    if (result) result = task_contract_detail::number(reader, decoded.taskId, UINT32_MAX);
    if (result) result = reader.readBool(decoded.enabled);
    if (result) result = decoded.kind == TaskMutationKind::Create || decoded.kind == TaskMutationKind::Replace
        ? task_contract_detail::definition(reader, decoded.definition) : reader.readNull();
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    result = validateTaskMutation(decoded);
    if (result) request = decoded;
    return result;
}

Result taskMutationPrepareDigest(const TaskMutation& request, ByteView currentRoot, MutableByteSpan digest) {
    Sha256 hash;
    cbor::Writer writer(task_contract_detail::sink, &hash);
    Result result = task_contract_detail::startDigest(writer, StringView("blinker.task-mutation.v1"), currentRoot, digest);
    if (result) result = validateTaskMutation(request);
    if (result) result = task_contract_detail::mutation(writer, request);
    return result ? hash.finish(digest) : result;
}

Result encodeTaskReadPrepare(const TaskReadPrepare& request, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr || !task_contract_detail::validRead(request))
        return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    const Result result = task_contract_detail::read(writer, request);
    if (result) encoded = writer.view();
    return result;
}

Result decodeTaskReadPrepare(ByteView encoded, TaskReadPrepare& request) {
    request = TaskReadPrepare();
    Result result = task_contract_detail::validateInput(encoded, kTaskReadPrepareMaxSize);
    if (!result) return result;
    TaskReadPrepare decoded;
    cbor::Reader reader(encoded, task_contract_detail::limits());
    size_t items = 0U;
    result = reader.readArraySize(items);
    if (result && items != 4U && items != 5U) result = Result::failure(ErrorCode::InvalidEncoding);
    decoded.catalog = items == 5U;
    if (result) result = task_contract_detail::version(reader);
    if (result) result = task_contract_detail::id(reader, decoded.requestId, 16U);
    if (result) result = task_contract_detail::number(reader, decoded.expectedRevision, UINT32_MAX);
    if (result) result = task_contract_detail::number(reader, decoded.index, UINT32_MAX);
    if (result && decoded.catalog) {
        ByteView fingerprint;
        result = reader.readBytes(fingerprint);
        if (result && fingerprint.size != (decoded.index == UINT32_MAX ? 0U : 32U))
            result = Result::failure(ErrorCode::InvalidEncoding);
        if (result && !fingerprint.empty()) memcpy(decoded.catalogFingerprint, fingerprint.data, 32U);
    }
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    if (!task_contract_detail::validRead(decoded))
        return Result::failure(ErrorCode::InvalidArgument);
    request = decoded;
    return Result::success();
}

Result taskReadPrepareDigest(ByteView requestId, uint32_t revision, uint32_t index,
    ByteView currentRoot, MutableByteSpan digest) {
    if (!task_contract_detail::nonzero(requestId, 16U)) return Result::failure(ErrorCode::InvalidArgument);
    TaskReadPrepare request; memcpy(request.requestId, requestId.data, 16U);
    request.expectedRevision = revision; request.index = index;
    return taskReadPrepareDigest(request, currentRoot, digest);
}
Result taskReadPrepareDigest(const TaskReadPrepare& request, ByteView currentRoot, MutableByteSpan digest) {
    Sha256 hash;
    cbor::Writer writer(task_contract_detail::sink, &hash);
    Result result = task_contract_detail::startDigest(writer, StringView("blinker.task-read.v1"), currentRoot, digest);
    if (!task_contract_detail::validRead(request)) return Result::failure(ErrorCode::InvalidArgument);
    if (result) result = task_contract_detail::read(writer, request);
    return result ? hash.finish(digest) : result;
}

Result encodeTaskManagementChallenge(const TaskManagementChallenge& value, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr || !task_contract_detail::validChallenge(value)) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Writer writer(output);
    const Result result = task_contract_detail::challenge(writer, value);
    if (result) encoded = writer.view();
    return result;
}

Result decodeTaskManagementChallenge(ByteView encoded, TaskManagementChallenge& value) {
    value = TaskManagementChallenge();
    Result result = task_contract_detail::validateInput(encoded, kTaskManagementChallengeMaxSize);
    if (!result) return result;
    cbor::Reader reader(encoded, task_contract_detail::limits());
    TaskManagementChallenge decoded;
    result = task_contract_detail::array(reader, 7U);
    if (result) result = task_contract_detail::version(reader);
    if (result) result = task_contract_detail::id(reader, decoded.authority.deviceInstanceId, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.authority.taskScope, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.authority.rootDigest, 32U);
    if (result) result = task_contract_detail::id(reader, decoded.nonce, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.requestId, 16U);
    if (result) result = task_contract_detail::id(reader, decoded.operationDigest, 32U);
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    value = decoded;
    return Result::success();
}

Result encodeTaskManagementResult(const TaskManagementChallenge& completed, const TaskManagementResult& value,
    TaskResultKey role, ByteView secret, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (output.data == nullptr) return Result::failure(ErrorCode::InvalidArgument);
    uint8_t tag[32] = {};
    Result result = task_contract_detail::proof(completed, value, role, secret, MutableByteSpan(tag, sizeof(tag)));
    cbor::Writer writer(output);
    if (result) result = writer.beginArray(8U);
    if (result) result = task_contract_detail::resultFields(writer, completed, value, role);
    if (result) result = writer.writeBytes(ByteView(tag, sizeof(tag)));
    secureZero(MutableByteSpan(tag, sizeof(tag)));
    if (result && writer.size() > kTaskManagementResultMaxSize) return Result::failure(ErrorCode::CapacityExceeded);
    if (result) encoded = writer.view();
    return result;
}

Result verifyTaskManagementResult(ByteView encoded, const TaskManagementChallenge& expected,
    TaskResultKey role, ByteView secret, TaskManagementResult& value) {
    value = TaskManagementResult();
    if (!task_contract_detail::validChallenge(expected) || !task_contract_detail::validRole(role) ||
        !task_contract_detail::nonzero(secret, 32U)) return Result::failure(ErrorCode::InvalidArgument);
    Result result = task_contract_detail::validateInput(encoded, kTaskManagementResultMaxSize);
    if (!result) return result;
    TaskManagementResult decoded;
    cbor::Reader reader(encoded, task_contract_detail::limits());
    result = task_contract_detail::array(reader, 8U);
    if (result) result = task_contract_detail::version(reader);
    uint8_t keyRole = 0U, kind = 0U;
    if (result) result = task_contract_detail::number(reader, keyRole, static_cast<uint8_t>(TaskResultKey::Admin));
    if (result && keyRole != static_cast<uint8_t>(role)) result = Result::failure(ErrorCode::AuthenticationRequired);
    if (result) result = task_contract_detail::number(reader, kind, static_cast<uint8_t>(TaskManagementResultKind::Rejected));
    decoded.kind = static_cast<TaskManagementResultKind>(kind);
    ByteView requestId, tag;
    if (result) result = reader.readBytes(requestId);
    if (result && !constantTimeEqual(requestId, ByteView(expected.requestId, 16U)))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    if (result) result = task_contract_detail::number(reader, decoded.tableRevision, UINT32_MAX);
    if (result) result = task_contract_detail::number(reader, decoded.count, TaskSnapshotCodec::maxTasks);
    if (decoded.kind == TaskManagementResultKind::Mutation) {
        if (result) result = task_contract_detail::array(reader, 2U);
        if (result) result = task_contract_detail::number(reader, decoded.mutation.taskId, UINT32_MAX);
        if (result) result = reader.readBool(decoded.mutation.replayed);
        decoded.mutation.tableRevision = decoded.tableRevision;
    } else if (decoded.kind == TaskManagementResultKind::Record) {
        decoded.hasRecord = true;
        if (result) result = task_contract_detail::array(reader, 2U);
        if (result) result = task_contract_detail::number(reader, decoded.index, UINT32_MAX);
        if (result) result = task_contract_detail::array(reader, 6U);
        if (result) result = task_contract_detail::number(reader, decoded.record.taskId, UINT32_MAX);
        if (result) result = task_contract_detail::number(reader, decoded.record.definitionRevision, UINT32_MAX);
        if (result) result = reader.readUnsigned(decoded.record.consumedUtcMillis);
        if (result) result = task_contract_detail::number(reader, decoded.record.startSequence, UINT32_MAX);
        uint8_t progress = 0U;
        if (result) result = task_contract_detail::number(reader, progress, static_cast<uint8_t>(TaskProgress::Interrupted));
        decoded.record.progress = static_cast<TaskProgress>(progress);
        if (result) result = task_contract_detail::definition(reader, decoded.record);
    } else if (decoded.kind == TaskManagementResultKind::Catalog || decoded.kind == TaskManagementResultKind::Target) {
        TaskTargetCatalogPage& c = decoded.catalog;
        if (result) result = task_contract_detail::array(reader, decoded.kind == TaskManagementResultKind::Target ? 6U : 3U);
        if (result) result = task_contract_detail::number(reader, c.capacity, TaskSnapshotCodec::maxTasks);
        if (result) result = task_contract_detail::number(reader, c.total, kTaskMaximumTargets);
        if (result) result = task_contract_detail::id(reader, c.fingerprint, 32U);
        if (decoded.kind == TaskManagementResultKind::Target) {
            StringView key; uint8_t type = 0U;
            if (result) result = task_contract_detail::number(reader, decoded.index, UINT32_MAX);
            if (result) result = reader.readText(key);
            if (result) result = task_contract_detail::number(reader, type, static_cast<uint8_t>(ValueType::Float64));
            if (result && key.size > kMaxEndpointKeyLength) result = Result::failure(ErrorCode::CapacityExceeded);
            if (result) { memcpy(c.key, key.data, key.size); c.keyLength = static_cast<uint8_t>(key.size); c.valueType = static_cast<ValueType>(type); }
        }
    } else if (decoded.kind == TaskManagementResultKind::Rejected) {
        uint8_t reason = 0U;
        if (result) result = task_contract_detail::number(reader, reason, static_cast<uint8_t>(TaskMutationRejection::StartCooldown));
        decoded.rejection = static_cast<TaskMutationRejection>(reason);
    } else if (decoded.kind == TaskManagementResultKind::Retired) {
        if (result) result = task_contract_detail::id(reader, decoded.retirementId, 16U);
    } else if (result) result = reader.readNull();
    if (result) result = reader.readBytes(tag);
    if (!result) return result;
    if (!reader.finished()) return Result::failure(ErrorCode::TrailingData);
    if (tag.size != 32U) return Result::failure(ErrorCode::InvalidEncoding);
    uint8_t computed[32] = {};
    result = task_contract_detail::proof(expected, decoded, role, secret, MutableByteSpan(computed, sizeof(computed)));
    if (result && !constantTimeEqual(ByteView(computed, sizeof(computed)), tag))
        result = Result::failure(ErrorCode::AuthenticationRequired);
    secureZero(MutableByteSpan(computed, sizeof(computed)));
    if (result) value = decoded;
    return result;
}

Result encodeTaskCarrier(TaskWireOperation operation, ByteView body, MutableByteSpan output, ByteView& encoded) {
    encoded = ByteView();
    if (operation > TaskWireOperation::RetirementRead || body.data == nullptr || body.empty() || body.size > kTaskManagementResultMaxSize)
        return Result::failure(ErrorCode::InvalidArgument);
    const size_t prefix = body.size < 24U ? 4U : 5U;
    if (output.data == nullptr || output.size < prefix + body.size) return Result::failure(ErrorCode::BufferTooSmall);
    // Explicit overlap support lets one product-owned buffer hold RX then TX.
    memmove(output.data + prefix, body.data, body.size);
    output.data[0] = 0x83U; output.data[1] = 1U; output.data[2] = static_cast<uint8_t>(operation);
    output.data[3] = body.size < 24U ? static_cast<uint8_t>(0x40U + body.size) : 0x58U;
    if (prefix == 5U) output.data[4] = static_cast<uint8_t>(body.size);
    encoded = ByteView(output.data, prefix + body.size);
    return Result::success();
}

Result decodeTaskCarrier(ByteView encoded, TaskWireOperation& operation, ByteView& body) {
    operation = TaskWireOperation::Failure; body = ByteView();
    if (encoded.data == nullptr || encoded.size > kTaskCarrierMaxSize) return Result::failure(ErrorCode::InvalidArgument);
    cbor::Limits limits; limits.maxTextLength = 0U; limits.maxByteStringLength = kTaskManagementResultMaxSize;
    limits.maxContainerItems = 3U; limits.maxDepth = 1U;
    Result result = cbor::validate(encoded, limits);
    cbor::Reader reader(encoded, limits); size_t count = 0U; uint64_t version = 0U, op = 0U; ByteView inner;
    if (result) result = reader.readArraySize(count);
    if (result) result = reader.readUnsigned(version);
    if (result) result = reader.readUnsigned(op);
    if (result) result = reader.readBytes(inner);
    if (!result) return result;
    if (!reader.finished() || count != 3U || version != 1U || op > 8U || inner.empty())
        return Result::failure(ErrorCode::InvalidEncoding);
    operation = static_cast<TaskWireOperation>(op); body = inner;
    return Result::success();
}

} // namespace blinker
