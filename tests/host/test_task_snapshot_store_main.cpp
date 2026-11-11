#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/core/Crc32.h"
#include "BlinkerV2/core/TaskCalendarCursor.h"
#include "BlinkerV2/runtime/TaskSnapshotStore.h"
#include "BlinkerV2/protocol/cbor/Cbor.h"

#include "support/TaskAuthorityFixture.h"

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << "line " << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
static_assert(sizeof(TaskSnapshotRecord) <= 128U, "Bound decoded task record");
static_assert(sizeof(TaskSnapshotHeader) == 124U, "Bound decoded task header including retirement state");

namespace {
const uint64_t base = 1790035200000ULL;
const size_t maximum = TaskSnapshotCodec::headerSize + TaskSnapshotCodec::maxTasks *
    TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize;

TaskSnapshotHeader header(uint32_t nextId = 11U) {
    TaskSnapshotHeader result;
    result.authority = testAuthority();
    result.tableRevision = 1U;
    result.nextTaskId = nextId;
    result.requestId[0] = 1U;
    result.requestDigest[0] = 2U;
    return result;
}

TaskSnapshotRecord record(uint32_t id = 1U) {
    TaskSnapshotRecord result;
    result.taskId = id;
    result.definitionRevision = 1U;
    result.timing.kind = TaskTimingKind::UtcOnce;
    result.timing.utcMillis = base;
    result.enabled = true;
    result.keyLength = 5U;
    memcpy(result.key, "power", 5U);
    result.valueLength = 1U;
    result.value[0] = 0xF5U;
    return result;
}

void checksum(uint8_t* bytes, size_t size) {
    const uint32_t crc = computeCrc32(ByteView(bytes, size - 4U));
    for (size_t i = 0U; i < 4U; ++i)
        bytes[size - 4U + i] = static_cast<uint8_t>(crc >> ((3U - i) * 8U));
}

void exactGolden() {
    // Independent Node big-endian Buffer + bitwise CRC32 oracle, not encoder output.
    const char hex[] =
        "42545331020000010102030405060708090a0b0c0d0e0f102122232425262728"
        "292a2b2c2d2e2f304142434445464748494a4b4c4d4e4f505152535455565758"
        "595a5b5c5d5e5f60000000010000000b01000000000000000000000000000000"
        "0200000000000000000000000000000000000000000000000000000000000000"
        "000000010000000100000000000000000000000001010000000001a0c6698800"
        "000000000000000000050100706f776572000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000f500000000000000000000000000000000000000"
        "de6dd278";
    uint8_t bytes[260];
    auto task = record();
    size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    CHECK(written == sizeof(bytes) && strlen(hex) == 2U * written);
    const char digits[] = "0123456789abcdef";
    for (size_t i = 0U; i < written; ++i) {
        CHECK(hex[i * 2U] == digits[bytes[i] >> 4U]);
        CHECK(hex[i * 2U + 1U] == digits[bytes[i] & 15U]);
    }
}

void terminalMarker() {
    // Independently assembled with Python struct(big-endian) + zlib.crc32.
    const char hex[] =
        "42545331030000000102030405060708090a0b0c0d0e0f102122232425262728"
        "292a2b2c2d2e2f304142434445464748494a4b4c4d4e4f505152535455565758"
        "595a5b5c5d5e5f60000000010000000b01000000000000000000000000000000"
        "0200000000000000000000000000000000000000000000000000000000000000"
        "7773f34b";
    auto meta = header(); meta.retired = true;
    uint8_t bytes[maximum]; size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(meta, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    CHECK(written == 132U && strlen(hex) == written * 2U);
    const char digits[] = "0123456789abcdef";
    for (size_t i = 0U; i < written; ++i) {
        CHECK(hex[2U*i] == digits[bytes[i] >> 4U] && hex[2U*i+1U] == digits[bytes[i] & 15U]);
    }
    TaskSnapshotHeader restored; size_t count = 9U;
    CHECK(TaskSnapshotCodec::decode(ByteView(bytes, written), restored, nullptr, 0U, count));
    CHECK(restored.retired && count == 0U && restored.authority == meta.authority);
    CHECK(restored.tableRevision == meta.tableRevision && restored.nextTaskId == meta.nextTaskId);
    for (size_t size = 0U; size < written; ++size)
        CHECK(!TaskSnapshotCodec::decode(ByteView(bytes, size), restored, nullptr, 0U, count));
    for (size_t i = 0U; i < written; ++i) {
        bytes[i] ^= 1U;
        CHECK(!TaskSnapshotCodec::decode(ByteView(bytes, written), restored, nullptr, 0U, count));
        bytes[i] ^= 1U;
    }
    auto task = record();
    CHECK(!TaskSnapshotCodec::encode(meta, &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    meta.tableRevision = 0U; meta.nextTaskId = 1U; // Initialized, never installed tasks.
    CHECK(TaskSnapshotCodec::encode(meta, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    memset(meta.requestId, 0, sizeof(meta.requestId));
    CHECK(!TaskSnapshotCodec::encode(meta, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    meta = header(); meta.retired = true; memset(meta.requestDigest, 0, sizeof(meta.requestDigest));
    CHECK(!TaskSnapshotCodec::encode(meta, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    bytes[4] = 3U; checksum(bytes, written); // Schema 3 can NEVER carry runnable records.
    CHECK(!TaskSnapshotCodec::decode(ByteView(bytes, written), restored, &task, 1U, count));
    CHECK(count == 0U);
}

void roundTripAndCapacity() {
    for (size_t entries : {0U, 4U, 8U, 10U}) {
        TaskSnapshotRecord input[10];
        for (size_t i = 0U; i < entries; ++i) input[i] = record(static_cast<uint32_t>(i + 1U));
        // A full 64-byte key has no terminator in this length-delimited format.
        if (entries != 0U) {
            input[entries - 1U].keyLength = 64U;
            memset(input[entries - 1U].key, 'k', 64U);
        }
        uint8_t bytes[maximum + 1U];
        memset(bytes, 0xAA, sizeof(bytes));
        size_t written = 99U;
        CHECK(TaskSnapshotCodec::encode(header(), input, entries, MutableByteSpan(bytes, maximum), written));
        CHECK(written == 132U + 128U * entries && bytes[written] == 0xAAU);
        TaskSnapshotHeader restored;
        TaskSnapshotRecord output[10];
        size_t count = 99U;
        CHECK(TaskSnapshotCodec::decode(ByteView(bytes, written), restored, output, 10U, count));
        CHECK(count == entries && restored.authority == testAuthority() && restored.tableRevision == 1U);
        CHECK(restored.nextTaskId == 11U && restored.requestId[0] == 1U && restored.requestDigest[0] == 2U);
        for (size_t i = 0U; i < entries; ++i) {
            CHECK(output[i].taskId == i + 1U && output[i].timing.utcMillis == base);
            CHECK(output[i].keyLength == input[i].keyLength);
            CHECK(memcmp(output[i].key, input[i].key, output[i].keyLength) == 0);
            CHECK(output[i].value[0] == 0xF5U && output[i].enabled);
        }
        uint8_t again[maximum];
        size_t againSize = 0U;
        CHECK(TaskSnapshotCodec::encode(restored, output, count, MutableByteSpan(again, sizeof(again)), againSize));
        CHECK(againSize == written && memcmp(bytes, again, written) == 0);
        CHECK(!TaskSnapshotCodec::encode(header(), input, entries, MutableByteSpan(again, written - 1U), againSize));
        CHECK(againSize == 0U);
        std::cout << entries << " tasks: encoded=" << written << ", decoded="
                  << sizeof(TaskSnapshotHeader) + entries * sizeof(TaskSnapshotRecord) << '\n';
    }
    CHECK(TaskSnapshotCodec::encodedSize(11U) == 0U);
    CHECK(TaskSnapshotCodec::encodedSize(SIZE_MAX) == 0U);
    uint8_t bytes[maximum];
    size_t written = 9U;
    CHECK(!TaskSnapshotCodec::encode(header(), nullptr, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    CHECK(written == 0U);
    CHECK(!TaskSnapshotCodec::encode(header(), nullptr, 11U, MutableByteSpan(bytes, sizeof(bytes)), written));
}

void rejectWithoutPublication(ByteView bytes, size_t capacity = 2U) {
    TaskSnapshotHeader restored;
    restored.authority = otherAuthority();
    TaskSnapshotRecord output[2];
    output[0].taskId = 555U;
    output[1].taskId = 666U;
    size_t count = 99U;
    CHECK(!TaskSnapshotCodec::decode(bytes, restored, output, capacity, count));
    CHECK(count == 0U && restored.authority == otherAuthority());
    CHECK(output[0].taskId == 555U && output[1].taskId == 666U);
}

void corruption() {
    uint8_t bytes[maximum];
    TaskSnapshotRecord records[2] = {record(1U), record(2U)};
    size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(header(), records, 2U, MutableByteSpan(bytes, sizeof(bytes)), written));
    for (size_t length = 0U; length < written; ++length) rejectWithoutPublication(ByteView(bytes, length));
    rejectWithoutPublication(ByteView(bytes, written + 1U));
    rejectWithoutPublication(ByteView(bytes, written), 1U);
    rejectWithoutPublication(ByteView());
    for (size_t byte = 0U; byte < written; ++byte) {
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            bytes[byte] ^= static_cast<uint8_t>(1U << bit);
            rejectWithoutPublication(ByteView(bytes, written));
            bytes[byte] ^= static_cast<uint8_t>(1U << bit);
        }
    }
    // Corruption with a recomputed CRC exercises semantics, not only checksum.
    const size_t badOffsets[] = {5U, 128U + 21U, 128U + 43U, 128U + 117U,
                                 128U + 44U + 5U, 128U + 108U + 1U, 256U + 20U};
    for (size_t offset : badOffsets) {
        const uint8_t saved = bytes[offset];
        bytes[offset] = 0xFFU;
        checksum(bytes, written);
        rejectWithoutPublication(ByteView(bytes, written));
        bytes[offset] = saved;
    }
    checksum(bytes, written);
    // Duplicate task IDs, including an invalid second record, cannot publish the first.
    bytes[259] = 1U;
    checksum(bytes, written);
    rejectWithoutPublication(ByteView(bytes, written));
}

void versionAndBinding() {
    // A valid EMPTY schema-1 snapshot must be rejected, never mapped to a u128 scope.
    uint8_t legacy[76] = {};
    memcpy(legacy, "BTS1", 4U); legacy[4] = 1U; legacy[11] = 7U; legacy[19] = 1U;
    checksum(legacy, sizeof(legacy));
    TaskSnapshotHeader restored; TaskSnapshotRecord recordOut; size_t count = 0U;
    CHECK(TaskSnapshotCodec::decode(ByteView(legacy, sizeof(legacy)), restored, &recordOut, 1U, count).code()
          == ErrorCode::UnsupportedVersion);
    for (unsigned field = 0U; field < 3U; ++field) {
        auto meta = header(); const auto task = record(); uint8_t bytes[maximum]; size_t written = 0U;
        if (field == 0U) memset(meta.authority.deviceInstanceId, 0, 16U);
        else if (field == 1U) memset(meta.authority.taskScope, 0, 16U);
        else memset(meta.authority.rootDigest, 0, 32U);
        CHECK(!TaskSnapshotCodec::encode(meta, &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
        CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
        const size_t offset = field == 0U ? 8U : (field == 1U ? 24U : 40U);
        memset(bytes + offset, 0, field == 2U ? 32U : 16U); checksum(bytes, written);
        rejectWithoutPublication(ByteView(bytes, written));
    }
}

void validation() {
    uint8_t bytes[maximum];
    size_t written = 0U;
    for (unsigned fault = 0U; fault < 15U; ++fault) {
        auto meta = header();
        auto task = record();
        switch (fault) {
        case 0: meta.authority = TaskAuthorityBinding(); break;
        case 1: meta.tableRevision = 0U; break;
        case 2: meta.nextTaskId = 1U; break;
        case 3: meta.requestId[0] = 0U; break;
        case 4: meta.requestDigest[0] = 0U; break;
        case 5: task.definitionRevision = 2U; break;
        case 6: task.keyLength = 65U; break;
        case 7: task.key[2] = 0U; break;
        case 8: task.key[0] = 0xFFU; break;
        case 9: task.valueLength = 10U; break;
        case 10: task.valueType = ValueType::Text; break;
        case 11: task.value[0] = 0U; break; // Integer where boolean expected.
        case 12: task.progress = TaskProgress::Claimed; break; // Missing occurrence.
        case 13: task.startSequence = 1U; break; // Calendar cannot have a countdown start.
        default: task.timing.durationMillis = 1U; break;
        }
        CHECK(!TaskSnapshotCodec::encode(meta, &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
        CHECK(written == 0U);
    }
    // All five supported endpoint scalar types use the existing CBOR/value validator.
    for (unsigned valueType = 0U; valueType <= 4U; ++valueType) {
        auto task = record();
        task.valueType = static_cast<ValueType>(valueType);
        cbor::Writer writer(MutableByteSpan(task.value, sizeof(task.value)));
        switch (task.valueType) {
        case ValueType::Boolean: CHECK(writer.writeBool(false)); break;
        case ValueType::SignedInteger: CHECK(writer.writeInt(-9007199254740991LL)); break;
        case ValueType::UnsignedInteger: CHECK(writer.writeUnsigned(9007199254740991ULL)); break;
        case ValueType::Float32: CHECK(writer.writeFloat32(1.25F)); break;
        default: CHECK(writer.writeFloat64(-1.25)); break;
        }
        task.valueLength = static_cast<uint8_t>(writer.size());
        CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    }
    auto task = record();
    const uint8_t invalidValues[][9] = {
        {0xFA, 0x7F, 0x80, 0, 0}, // infinity
        {0xFA, 0x7F, 0xC0, 0, 0}, // NaN
        {0x1B, 0, 0x20, 0, 0, 0, 0, 0, 0}, // 2^53
        {0x18, 1}, // non-minimal integer
        {0xF5, 0xF5} // two values
    };
    const uint8_t lengths[] = {5U, 5U, 9U, 2U, 2U};
    for (size_t i = 0U; i < 5U; ++i) {
        memcpy(task.value, invalidValues[i], sizeof(task.value));
        task.valueLength = lengths[i];
        task.valueType = i < 2U ? ValueType::Float32 : (i == 4U ? ValueType::Boolean : ValueType::UnsignedInteger);
        CHECK(!TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    }
}

void progressAndDeletion() {
    uint8_t bytes[maximum];
    size_t written = 0U;
    const int16_t offsets[] = {-720, -1, 0, 345, 840};
    for (int16_t offset : offsets) {
        auto task = record();
        task.timing = TaskTiming();
        task.timing.kind = TaskTimingKind::Weekly;
        task.timing.minuteOfDay = 480U;
        task.timing.weekdays = 0x7FU;
        task.timing.utcOffsetMinutes = offset;
        task.progress = TaskProgress::Completed;
        CHECK(nextTaskUtc(task.timing, base, task.consumedUtcMillis));
        CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
        TaskSnapshotHeader restored;
        TaskSnapshotRecord output;
        size_t count = 0U;
        CHECK(TaskSnapshotCodec::decode(ByteView(bytes, written), restored, &output, 1U, count));
        CHECK(output.timing.utcOffsetMinutes == offset && output.consumedUtcMillis == task.consumedUtcMillis);
    }
    for (TaskProgress progress : {TaskProgress::Claimed, TaskProgress::Completed, TaskProgress::Failed, TaskProgress::Skipped}) {
        auto task = record();
        task.progress = progress;
        task.consumedUtcMillis = base;
        CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
        task.consumedUtcMillis = base + 1U;
        CHECK(!TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    }
    for (TaskProgress progress : {TaskProgress::CountdownArmed, TaskProgress::Claimed, TaskProgress::Completed,
                                  TaskProgress::Failed, TaskProgress::Skipped, TaskProgress::Interrupted}) {
        auto task = record();
        task.timing = TaskTiming();
        task.timing.durationMillis = 1000U;
        task.startSequence = 9U;
        task.progress = progress;
        CHECK(TaskSnapshotCodec::encode(header(), &task, 1U, MutableByteSpan(bytes, sizeof(bytes)), written));
    }
    auto initial = header();
    initial.tableRevision = 0U;
    initial.nextTaskId = 1U;
    memset(initial.requestId, 0, sizeof(initial.requestId));
    memset(initial.requestDigest, 0, sizeof(initial.requestDigest));
    CHECK(TaskSnapshotCodec::encode(initial, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    auto deleted = header(100U);
    deleted.tableRevision = 5U;
    CHECK(TaskSnapshotCodec::encode(deleted, nullptr, 0U, MutableByteSpan(bytes, sizeof(bytes)), written));
    TaskSnapshotHeader output;
    size_t count = 99U;
    CHECK(TaskSnapshotCodec::decode(ByteView(bytes, written), output, nullptr, 0U, count));
    CHECK(count == 0U && output.nextTaskId == 100U && output.tableRevision == 5U && output.requestId[0] == 1U);
}

class FaultStore : public IAtomicBlobStore {
public:
    enum class Fault { None, BeforeCommit, AfterCommit, DirtyHandle };
    uint8_t durable[maximum] = {};
    uint8_t visible[maximum] = {};
    size_t durableSize = 0U;
    size_t visibleSize = 0U;
    unsigned loads = 0U;
    unsigned writes = 0U;
    Fault fault = Fault::None;
    bool overreport = false;

    Result load(MutableByteSpan output, size_t& written) override {
        ++loads;
        written = 0U;
        if (visibleSize == 0U) return Result::failure(ErrorCode::NotFound);
        if (!output.data || output.size < visibleSize) return Result::failure(ErrorCode::BufferTooSmall);
        memcpy(output.data, visible, visibleSize);
        written = overreport ? output.size + 1U : visibleSize;
        return Result::success();
    }
    Result replace(ByteView value) override {
        ++writes;
        CHECK(value.size <= maximum);
        if (fault != Fault::BeforeCommit) {
            memcpy(visible, value.data, value.size);
            visibleSize = value.size;
        }
        if (fault == Fault::None || fault == Fault::AfterCommit) {
            memcpy(durable, value.data, value.size);
            durableSize = value.size;
        }
        return fault == Fault::None ? Result::success() : Result::failure(ErrorCode::InternalError);
    }
    Result clear() override { CHECK(false); return Result::failure(ErrorCode::InternalError); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
    void powerCycle() {
        memcpy(visible, durable, durableSize);
        visibleSize = durableSize;
        fault = Fault::None;
    }
};

void faultBoundary() {
    for (auto fault : {FaultStore::Fault::BeforeCommit, FaultStore::Fault::AfterCommit, FaultStore::Fault::DirtyHandle}) {
        FaultStore flash;
        uint8_t workspace[maximum];
        TaskSnapshotStore store(flash, MutableByteSpan(workspace, sizeof(workspace)));
        auto meta = header();
        auto task = record();
        size_t count = 9U;
        CHECK(store.load(meta, &task, 1U, count).code() == ErrorCode::NotFound && !store.failed());
        CHECK(count == 0U);
        CHECK(store.replace(meta, &task, 1U));
        task.progress = TaskProgress::Claimed;
        task.consumedUtcMillis = base;
        flash.fault = fault;
        CHECK(!store.replace(meta, &task, 1U) && store.failed());
        const unsigned reads = flash.loads;
        const unsigned writes = flash.writes;
        CHECK(store.load(meta, &task, 1U, count).code() == ErrorCode::StateConflict);
        CHECK(store.replace(meta, &task, 1U).code() == ErrorCode::StateConflict);
        CHECK(flash.loads == reads && flash.writes == writes); // No dirty read-back or retry.
        flash.powerCycle();
        TaskSnapshotStore recovered(flash, MutableByteSpan(workspace, sizeof(workspace)));
        CHECK(recovered.load(meta, &task, 1U, count) && count == 1U);
        CHECK(task.progress == (fault == FaultStore::Fault::AfterCommit ? TaskProgress::Claimed : TaskProgress::None));
        if (task.progress == TaskProgress::Claimed) {
            TaskCalendarCursor cursor;
            CHECK(cursor.reset(task.consumedUtcMillis));
            WallClockSnapshot time;
            time.utcMillis = base - 1U;
            time.sampleRevision = 1U;
            time.source = WallTimeSource::Server;
            time.state = WallClockState::Synchronized;
            TaskCalendarDecision decision;
            CHECK(cursor.poll(task.timing, time, 0U, decision));
            CHECK(decision.state == CalendarPoll::Exhausted); // No replay of persisted Claim.
            task.progress = TaskProgress::Completed;
            flash.fault = FaultStore::Fault::BeforeCommit;
            CHECK(!recovered.replace(meta, &task, 1U));
            flash.powerCycle();
            TaskSnapshotStore again(flash, MutableByteSpan(workspace, sizeof(workspace)));
            CHECK(again.load(meta, &task, 1U, count));
            CHECK(task.progress == TaskProgress::Claimed); // Result lost -> Unknown, not retry.
        }
    }
}

void storageErrors() {
    FaultStore flash;
    uint8_t workspace[maximum];
    TaskSnapshotStore store(flash, MutableByteSpan(workspace, sizeof(workspace)));
    auto meta = header();
    auto task = record();
    CHECK(store.clear().code() == ErrorCode::NotConfigured && !store.failed());
    task.keyLength = 0U;
    CHECK(!store.replace(meta, &task, 1U) && !store.failed() && flash.writes == 0U);
    task = record();
    CHECK(store.replace(meta, &task, 1U));
    size_t count = 99U;
    flash.visible[72] ^= 1U;
    CHECK(!store.load(meta, &task, 1U, count) && store.failed() && count == 0U);
    CHECK(store.clear().code() == ErrorCode::StateConflict); // Corrupt data cannot be auto-formatted.
    CHECK(task.taskId == 1U);
    flash.powerCycle();
    TaskSnapshotStore recovered(flash, MutableByteSpan(workspace, sizeof(workspace)));
    flash.overreport = true;
    CHECK(!recovered.load(meta, &task, 1U, count) && recovered.failed());
    TaskSnapshotStore tooSmall(flash, MutableByteSpan(workspace, 1U));
    CHECK(!tooSmall.replace(meta, &task, 1U) && !tooSmall.failed());
    CHECK(!tooSmall.load(meta, &task, 1U, count) && !tooSmall.failed());
}
} // namespace

int main() {
    exactGolden(); terminalMarker(); roundTripAndCapacity(); corruption(); versionAndBinding(); validation(); progressAndDeletion();
    faultBoundary(); storageErrors();
    std::cout << "task snapshot: codec/version/binding/store groups passed; header=" << sizeof(TaskSnapshotHeader)
              << ", record=" << sizeof(TaskSnapshotRecord) << ", store=" << sizeof(TaskSnapshotStore) << '\n';
}
