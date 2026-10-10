#include "TaskSnapshotStore.h"

#include <string.h>
#include "../core/Crc32.h"
#include "../core/Utf8.h"
#include "../model/EndpointValue.h"

namespace blinker {
namespace {

const uint8_t taskSnapshotMagic[4] = {'B', 'T', 'S', '1'};

void taskSnapshotWrite(uint8_t* output, uint64_t value, size_t width) {
    for (size_t i = width; i != 0U; --i) {
        output[i - 1U] = static_cast<uint8_t>(value);
        value >>= 8U;
    }
}

uint64_t taskSnapshotRead(const uint8_t* input, size_t width) {
    uint64_t value = 0U;
    for (size_t i = 0U; i < width; ++i) value = (value << 8U) | input[i];
    return value;
}

bool taskSnapshotZero(const uint8_t* input, size_t size) {
    for (size_t i = 0U; i < size; ++i) if (input[i] != 0U) return false;
    return true;
}

bool validTaskSnapshotHeader(const TaskSnapshotHeader& header, size_t count) {
    if (!header.authority.valid() || header.nextTaskId == 0U) return false;
    const bool noId = taskSnapshotZero(header.requestId, sizeof(header.requestId));
    const bool noDigest = taskSnapshotZero(header.requestDigest, sizeof(header.requestDigest));
    if (header.retired) return count == 0U && !noId && !noDigest;
    if (header.tableRevision == 0U)
        return count == 0U && header.nextTaskId == 1U && noId && noDigest;
    return !noId && !noDigest;
}

bool validTaskSnapshotRecord(const TaskSnapshotRecord& record,
                             const TaskSnapshotHeader& header, uint32_t previousId) {
    if (record.taskId <= previousId || record.taskId >= header.nextTaskId ||
        record.definitionRevision == 0U || record.definitionRevision > header.tableRevision ||
        !validateTaskTiming(record.timing) || record.keyLength == 0U ||
        record.keyLength > sizeof(record.key) || record.valueLength == 0U ||
        record.valueLength > sizeof(record.value) || record.valueType > ValueType::Float64 ||
        record.progress > TaskProgress::Interrupted) return false;
    const StringView key(reinterpret_cast<const char*>(record.key), record.keyLength);
    if (!isValidUtf8(key) || memchr(record.key, 0, record.keyLength) != nullptr) return false;
    const ByteView encoded(record.value, record.valueLength);
    cbor::Reader reader(encoded);
    cbor::Type type;
    const PropertyConstraints scalarLimits; // Finite numbers and JS-safe integers.
    EndpointDescriptor endpoint;
    endpoint.key = key;
    endpoint.type = record.valueType;
    endpoint.access = AccessWrite;
    endpoint.constraints = &scalarLimits;
    if (!reader.peekType(type) || !cbor::validate(encoded) ||
        !validateEndpointValue(endpoint, encoded, type, AccessWrite)) return false;

    if (record.timing.kind == TaskTimingKind::Countdown) {
        if (record.consumedUtcMillis != 0U) return false;
        if ((record.progress == TaskProgress::None) != (record.startSequence == 0U)) return false;
        return record.progress != TaskProgress::CountdownArmed || record.enabled;
    }
    if (record.startSequence != 0U || record.progress == TaskProgress::CountdownArmed ||
        record.progress == TaskProgress::Interrupted) return false;
    if (record.progress == TaskProgress::None) return record.consumedUtcMillis == 0U;
    if (record.consumedUtcMillis < kWallTimeMinimumMillis ||
        record.consumedUtcMillis >= kWallTimeEndMillis) return false;
    uint64_t occurrence = 0U;
    return nextTaskUtc(record.timing, record.consumedUtcMillis - 1U, occurrence) &&
        occurrence == record.consumedUtcMillis;
}

void encodeTaskSnapshotRecord(const TaskSnapshotRecord& record, uint8_t* bytes) {
    taskSnapshotWrite(bytes, record.taskId, 4U);
    taskSnapshotWrite(bytes + 4U, record.definitionRevision, 4U);
    taskSnapshotWrite(bytes + 8U, record.consumedUtcMillis, 8U);
    taskSnapshotWrite(bytes + 16U, record.startSequence, 4U);
    bytes[20] = static_cast<uint8_t>(record.timing.kind);
    bytes[21] = record.enabled ? 1U : 0U;
    bytes[22] = static_cast<uint8_t>(record.progress);
    bytes[23] = static_cast<uint8_t>(record.valueType);
    taskSnapshotWrite(bytes + 24U, record.timing.utcMillis, 8U);
    taskSnapshotWrite(bytes + 32U, record.timing.durationMillis, 4U);
    taskSnapshotWrite(bytes + 36U, record.timing.minuteOfDay, 2U);
    taskSnapshotWrite(bytes + 38U, static_cast<uint16_t>(record.timing.utcOffsetMinutes), 2U);
    bytes[40] = record.timing.weekdays;
    bytes[41] = record.keyLength;
    bytes[42] = record.valueLength;
    memcpy(bytes + 44U, record.key, record.keyLength);
    memcpy(bytes + 108U, record.value, record.valueLength);
}

bool decodeTaskSnapshotRecord(const uint8_t* bytes, TaskSnapshotRecord& record) {
    record = TaskSnapshotRecord();
    record.taskId = static_cast<uint32_t>(taskSnapshotRead(bytes, 4U));
    record.definitionRevision = static_cast<uint32_t>(taskSnapshotRead(bytes + 4U, 4U));
    record.consumedUtcMillis = taskSnapshotRead(bytes + 8U, 8U);
    record.startSequence = static_cast<uint32_t>(taskSnapshotRead(bytes + 16U, 4U));
    record.timing.kind = static_cast<TaskTimingKind>(bytes[20]);
    record.enabled = bytes[21] == 1U;
    record.progress = static_cast<TaskProgress>(bytes[22]);
    record.valueType = static_cast<ValueType>(bytes[23]);
    record.timing.utcMillis = taskSnapshotRead(bytes + 24U, 8U);
    record.timing.durationMillis = static_cast<uint32_t>(taskSnapshotRead(bytes + 32U, 4U));
    record.timing.minuteOfDay = static_cast<uint16_t>(taskSnapshotRead(bytes + 36U, 2U));
    const int32_t offset = static_cast<int32_t>(taskSnapshotRead(bytes + 38U, 2U));
    record.timing.utcOffsetMinutes = static_cast<int16_t>(offset < 32768 ? offset : offset - 65536);
    record.timing.weekdays = bytes[40];
    record.keyLength = bytes[41];
    record.valueLength = bytes[42];
    if (bytes[21] > 1U || bytes[43] != 0U || record.keyLength > sizeof(record.key) ||
        record.valueLength > sizeof(record.value) || !taskSnapshotZero(bytes + 117U, 11U) ||
        !taskSnapshotZero(bytes + 44U + record.keyLength, 64U - record.keyLength) ||
        !taskSnapshotZero(bytes + 108U + record.valueLength, 9U - record.valueLength)) return false;
    memcpy(record.key, bytes + 44U, record.keyLength);
    memcpy(record.value, bytes + 108U, record.valueLength);
    return true;
}

} // namespace

size_t TaskSnapshotCodec::encodedSize(size_t count) {
    return count <= maxTasks ? headerSize + count * recordSize + trailerSize : 0U;
}

Result TaskSnapshotCodec::validateRecord(const TaskSnapshotRecord& record,
                                         const TaskSnapshotHeader& header, uint32_t previousId) {
    return validTaskSnapshotRecord(record, header, previousId)
        ? Result::success() : Result::failure(ErrorCode::InvalidArgument);
}

Result TaskSnapshotCodec::encode(const TaskSnapshotHeader& header,
                                 const TaskSnapshotRecord* records, size_t count,
                                 MutableByteSpan output, size_t& written) {
    written = 0U;
    const size_t size = encodedSize(count);
    if (size == 0U) return Result::failure(ErrorCode::CapacityExceeded);
    if ((count != 0U && records == nullptr) || !validTaskSnapshotHeader(header, count))
        return Result::failure(ErrorCode::InvalidArgument);
    uint32_t previousId = 0U;
    for (size_t i = 0U; i < count; ++i) {
        if (!validTaskSnapshotRecord(records[i], header, previousId))
            return Result::failure(ErrorCode::InvalidArgument);
        previousId = records[i].taskId;
    }
    if (output.data == nullptr || output.size < size)
        return Result::failure(ErrorCode::BufferTooSmall);
    memset(output.data, 0, size);
    memcpy(output.data, taskSnapshotMagic, 4U);
    output.data[4] = header.retired ? 3U : 2U;
    taskSnapshotWrite(output.data + 6U, count, 2U);
    memcpy(output.data + 8U, header.authority.deviceInstanceId, 16U);
    memcpy(output.data + 24U, header.authority.taskScope, 16U);
    memcpy(output.data + 40U, header.authority.rootDigest, 32U);
    taskSnapshotWrite(output.data + 72U, header.tableRevision, 4U);
    taskSnapshotWrite(output.data + 76U, header.nextTaskId, 4U);
    memcpy(output.data + 80U, header.requestId, sizeof(header.requestId));
    memcpy(output.data + 96U, header.requestDigest, sizeof(header.requestDigest));
    for (size_t i = 0U; i < count; ++i)
        encodeTaskSnapshotRecord(records[i], output.data + headerSize + i * recordSize);
    taskSnapshotWrite(output.data + size - trailerSize,
                      computeCrc32(ByteView(output.data, size - trailerSize)), trailerSize);
    written = size;
    return Result::success();
}

Result TaskSnapshotCodec::decode(ByteView input, TaskSnapshotHeader& header,
                                 TaskSnapshotRecord* records, size_t capacity, size_t& count) {
    count = 0U;
    if (input.data == nullptr || input.size < 5U ||
        memcmp(input.data, taskSnapshotMagic, 4U) != 0)
        return Result::failure(ErrorCode::InvalidEncoding);
    if (input.data[4] != 2U && input.data[4] != 3U) return Result::failure(ErrorCode::UnsupportedVersion);
    if (input.size < encodedSize(0U)) return Result::failure(ErrorCode::InvalidEncoding);
    const size_t entries = static_cast<size_t>(taskSnapshotRead(input.data + 6U, 2U));
    if (entries > maxTasks || input.size != encodedSize(entries) || input.data[5] != 0U ||
        taskSnapshotRead(input.data + input.size - trailerSize, trailerSize) !=
            computeCrc32(ByteView(input.data, input.size - trailerSize)))
        return Result::failure(ErrorCode::InvalidEncoding);
    if (entries > capacity || (entries != 0U && records == nullptr))
        return Result::failure(ErrorCode::BufferTooSmall);
    TaskSnapshotHeader decoded;
    decoded.retired = input.data[4] == 3U;
    memcpy(decoded.authority.deviceInstanceId, input.data + 8U, 16U);
    memcpy(decoded.authority.taskScope, input.data + 24U, 16U);
    memcpy(decoded.authority.rootDigest, input.data + 40U, 32U);
    decoded.tableRevision = static_cast<uint32_t>(taskSnapshotRead(input.data + 72U, 4U));
    decoded.nextTaskId = static_cast<uint32_t>(taskSnapshotRead(input.data + 76U, 4U));
    memcpy(decoded.requestId, input.data + 80U, sizeof(decoded.requestId));
    memcpy(decoded.requestDigest, input.data + 96U, sizeof(decoded.requestDigest));
    if (!validTaskSnapshotHeader(decoded, entries)) return Result::failure(ErrorCode::InvalidEncoding);
    uint32_t previousId = 0U;
    for (size_t i = 0U; i < entries; ++i) {
        TaskSnapshotRecord record;
        if (!decodeTaskSnapshotRecord(input.data + headerSize + i * recordSize, record) ||
            !validTaskSnapshotRecord(record, decoded, previousId))
            return Result::failure(ErrorCode::InvalidEncoding);
        previousId = record.taskId;
    }
    // All validation finished. Do not publish a prefix of a corrupt table.
    for (size_t i = 0U; i < entries; ++i)
        decodeTaskSnapshotRecord(input.data + headerSize + i * recordSize, records[i]);
    header = decoded;
    count = entries;
    return Result::success();
}

Result TaskSnapshotStore::load(TaskSnapshotHeader& header, TaskSnapshotRecord* records,
                               size_t capacity, size_t& count) {
    count = 0U;
    if (failed()) return Result::failure(ErrorCode::StateConflict);
    if (workspace_.data == nullptr || workspace_.size < TaskSnapshotCodec::encodedSize(0U))
        return Result::failure(ErrorCode::BufferTooSmall);
    size_t written = 0U;
    Result result = storage_.load(workspace_, written);
    if (result && written > workspace_.size) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = TaskSnapshotCodec::decode(ByteView(workspace_.data, written), header, records, capacity, count);
    state_ = result || result.code() == ErrorCode::NotFound ? State::Ready : State::Failed;
    return result;
}

Result TaskSnapshotStore::replace(const TaskSnapshotHeader& header,
                                  const TaskSnapshotRecord* records, size_t count) {
    if (failed()) return Result::failure(ErrorCode::StateConflict);
    size_t written = 0U;
    Result result = TaskSnapshotCodec::encode(header, records, count, workspace_, written);
    if (!result) return result; // Invalid candidate did not touch storage.
    result = storage_.replace(ByteView(workspace_.data, written));
    state_ = result ? State::Ready : State::Failed; // No read-back on a possibly dirty handle.
    return result;
}

Result TaskSnapshotStore::clear() {
    if (failed()) return Result::failure(ErrorCode::StateConflict);
    if (state_ != State::Ready) return Result::failure(ErrorCode::NotConfigured);
    const Result result = storage_.clear();
    if (result.code() == ErrorCode::NotFound) return Result::success();
    if (!result) state_ = State::Failed;
    return result;
}

} // namespace blinker
