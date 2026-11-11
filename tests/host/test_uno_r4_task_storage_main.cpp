#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

#include "UnoR4TaskStorage.h"
#include <BlinkerV2/core/Crc32.h>
#include "../internal/arduino/UnoR4TaskStorageProbe/TaskSlotShape.h"

namespace { size_t taskWorkspaceHeapAllocations = 0U; }

void* operator new(size_t size) {
    ++taskWorkspaceHeapAllocations;
    void* allocation = std::malloc(size == 0U ? 1U : size);
    if (allocation == nullptr) throw std::bad_alloc();
    return allocation;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }

namespace {

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                     \
        std::cerr << "R4 task storage check failed at " << __LINE__          \
                  << ": " #condition << std::endl; std::exit(1);            \
    }                                                                       \
} while (false)

typedef blinker::RenesasUnoTaskSnapshotBlobStore<4U> TaskBlob;
static_assert(TaskBlob::maximumSnapshotSize == 644U, "Four task snapshot bytes");
static_assert(TaskBlob::journalScratchSize == 664U, "Journal does not truncate tasks");
static_assert(sizeof(TaskBlob) >= 2U * TaskBlob::journalScratchSize,
              "Optional task view owns its two buffers, not heap/global/stack scratch");

TaskBlob* rpcReentryTask = nullptr;
bool rpcReentryDenied = false;
void tryRpcReentry() {
    uint8_t output[8U] = {}; size_t written = 99U;
    rpcReentryDenied = rpcReentryTask->load(blinker::MutableByteSpan(output, sizeof(output)), written).code()
        == blinker::ErrorCode::WouldBlock && written == 0U;
}

void fill(uint8_t* value, size_t size, uint8_t seed) {
    for (size_t index = 0U; index < size; ++index)
        value[index] = static_cast<uint8_t>(seed + index * 7U);
}

void expect(blinker::IAtomicBlobStore& store, const uint8_t* value, size_t size) {
    uint8_t output[644U] = {};
    size_t written = 0U;
    CHECK(store.load(blinker::MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == size && std::memcmp(output, value, size) == 0);
}

void setGeneration(fake_preferences::Entry& entry, uint32_t generation) {
    entry.value[8] = static_cast<uint8_t>(generation >> 24U);
    entry.value[9] = static_cast<uint8_t>(generation >> 16U);
    entry.value[10] = static_cast<uint8_t>(generation >> 8U);
    entry.value[11] = static_cast<uint8_t>(generation);
    const uint32_t crc = blinker::computeCrc32(blinker::ByteView(entry.value, entry.size - 4U));
    entry.value[entry.size-4U] = static_cast<uint8_t>(crc >> 24U);
    entry.value[entry.size-3U] = static_cast<uint8_t>(crc >> 16U);
    entry.value[entry.size-2U] = static_cast<uint8_t>(crc >> 8U);
    entry.value[entry.size-1U] = static_cast<uint8_t>(crc);
}

void testSharedOwnerFullSnapshotAndOldBytes() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    const uint8_t identity[] = {1U, 3U, 5U, 7U};
    CHECK(bank.deviceInstanceBlob().replace(blinker::ByteView(identity, sizeof(identity))));
    fake_preferences::Entry* identityEntry = fake_preferences::find("di1");
    CHECK(identityEntry != nullptr);
    identityEntry->value[5] = 6U; // Existing on-disk legacy identity ID.
    setGeneration(*identityEntry, 1U);
    uint8_t original[300U] = {};
    const size_t originalSize = identityEntry->size;
    std::memcpy(original, identityEntry->value, originalSize);
    uint8_t first[644U], second[644U];
    fill(first, sizeof(first), 2U); fill(second, sizeof(second), 9U);
    const size_t beforeWorkspace = taskWorkspaceHeapAllocations;
    size_t beforeDestruction = 0U;
    {
        TaskBlob task(bank);
        CHECK(taskWorkspaceHeapAllocations == beforeWorkspace);
        CHECK(task.protection() == blinker::StorageProtection::PlainFlash);
        const unsigned beforeGet = fake_preferences::state().getCalls;
        const unsigned beforeMetadata = fake_preferences::state().metadataCalls;
        CHECK(task.replace(blinker::ByteView(first, sizeof(first))));
        CHECK(fake_preferences::state().getCalls == beforeGet + 1U);
        CHECK(fake_preferences::state().metadataCalls == beforeMetadata + 4U); // No duplicate SDK PREFLEN.
        CHECK(fake_preferences::find("ts1")->size == 664U);
        expect(task, first, sizeof(first));
        expect(bank.deviceInstanceBlob(), identity, sizeof(identity));
        const uint8_t selector[] = {11U, 12U};
        CHECK(bank.wifiSelectorBlob().replace(blinker::ByteView(selector, sizeof(selector))));
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))));
        expect(task, second, sizeof(second));
        expect(bank.wifiSelectorBlob(), selector, sizeof(selector));
        expect(bank.deviceInstanceBlob(), identity, sizeof(identity));
        CHECK(identityEntry->size == originalSize && std::memcmp(identityEntry->value, original, originalSize) == 0);
        CHECK(fake_preferences::state().beginCalls == 1U && fake_preferences::state().endCalls == 0U &&
              fake_preferences::state().endRequests == 0U);
        beforeDestruction = taskWorkspaceHeapAllocations;
    }
    // View construction/destruction does not allocate its fixed buffers.
    // Raw SDK GET uses a real 664 B string here; do not pretend RPC is heap-free.
    CHECK(taskWorkspaceHeapAllocations == beforeDestruction);
    CHECK(bank.opened() && fake_preferences::state().endCalls == 0U &&
          fake_preferences::state().endRequests == 0U);
    TaskBlob reconstructed(bank);
    expect(reconstructed, second, sizeof(second));
    bank.end();
    size_t written = 99U;
    CHECK(reconstructed.load(blinker::MutableByteSpan(first, sizeof(first)), written).code() ==
          blinker::ErrorCode::NotConfigured && written == 0U);
    CHECK(reconstructed.replace(blinker::ByteView(first, sizeof(first))).code() == blinker::ErrorCode::NotConfigured);
    CHECK(reconstructed.clear().code() == blinker::ErrorCode::NotConfigured);
    CHECK(fake_preferences::state().endCalls == 1U);
}

void testFailuresRecoveryAndTombstone() {
    fake_preferences::reset();
    uint8_t first[644U], second[644U];
    fill(first, sizeof(first), 1U); fill(second, sizeof(second), 5U);
    {
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        TaskBlob task(bank);
        CHECK(task.replace(blinker::ByteView(first, sizeof(first))));
        fake_preferences::failNext(fake_preferences::RejectNextPut);
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))).code() == blinker::ErrorCode::WouldBlock);
        CHECK(blinker::renesasUnoModemOwner().quarantined());
        CHECK(fake_preferences::state().getCalls == fake_preferences::state().getCallsAtPut);
        CHECK(fake_preferences::state().metadataCalls == fake_preferences::state().metadataCallsAtPut);
        bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
        expect(task, first, sizeof(first));
        fake_preferences::failNext(fake_preferences::CorruptNextPut);
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))).code() == blinker::ErrorCode::InternalError);
        bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
        expect(task, first, sizeof(first));
        fake_preferences::failNext(fake_preferences::ApplyButReportFailure);
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))).code() == blinker::ErrorCode::WouldBlock);
        CHECK(blinker::renesasUnoModemOwner().quarantined());
        CHECK(fake_preferences::state().getCalls == fake_preferences::state().getCallsAtPut);
        CHECK(fake_preferences::state().metadataCalls == fake_preferences::state().metadataCallsAtPut);
        bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
        expect(task, second, sizeof(second));
        // A torn newer record retains one complete older journal slot.
        fake_preferences::find("ts0")->size = 13U;
        expect(task, first, sizeof(first));
    }
    {
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        TaskBlob task(bank);
        expect(task, first, sizeof(first));
        CHECK(task.clear());
        CHECK(fake_preferences::find("ts0")->size == 20U);
        CHECK(fake_preferences::find("ts0")->value[6] == 2U);
        size_t written = 99U;
        CHECK(task.load(blinker::MutableByteSpan(first, sizeof(first)), written).code() ==
              blinker::ErrorCode::NotFound && written == 0U);
        CHECK(task.clear().code() == blinker::ErrorCode::NotFound);
    }
    {
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        TaskBlob task(bank);
        size_t written = 99U;
        CHECK(task.load(blinker::MutableByteSpan(first, sizeof(first)), written).code() ==
              blinker::ErrorCode::NotFound && written == 0U);
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))));
        expect(task, second, sizeof(second));
    }
}

void testCapacityConflictAndGeneration() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    TaskBlob task(bank);
    uint8_t value[645U] = {};
    CHECK(task.replace(blinker::ByteView(value, sizeof(value))).code() == blinker::ErrorCode::CapacityExceeded);
    CHECK(task.replace(blinker::ByteView()).code() == blinker::ErrorCode::InvalidArgument);
    CHECK(task.clear().code() == blinker::ErrorCode::NotFound);
    CHECK(task.replace(blinker::ByteView(value, 644U)));
    size_t written = 0U;
    CHECK(task.load(blinker::MutableByteSpan(value, 643U), written).code() ==
          blinker::ErrorCode::BufferTooSmall && written == 644U);
    fake_preferences::Entry* active = fake_preferences::find("ts1");
    CHECK(active != nullptr);
    setGeneration(*active, UINT32_MAX);
    CHECK(task.replace(blinker::ByteView(value, 644U)).code() == blinker::ErrorCode::SequenceConflict);
    CHECK(task.clear().code() == blinker::ErrorCode::SequenceConflict);
    CHECK(fake_preferences::find("ts0") == nullptr);
    setGeneration(*active, 1U);
    fake_preferences::Entry* collision = fake_preferences::allocate("ts0");
    std::memcpy(collision->value, active->value, active->size); collision->size = active->size;
    CHECK(task.load(blinker::MutableByteSpan(value, sizeof(value)), written).code() == blinker::ErrorCode::SequenceConflict);
}

void testForeignKeyConflict() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    TaskBlob task(bank);
    uint8_t value[644U] = {};
    size_t written = 0U;
    // An unrelated existing key is not silently formatted/overwritten.
    fake_preferences::Entry* foreign = fake_preferences::allocate("ts0");
    foreign->size = 3U; foreign->value[0] = 42U;
    CHECK(task.load(blinker::MutableByteSpan(value, sizeof(value)), written).code() == blinker::ErrorCode::InvalidEncoding);
    CHECK(task.replace(blinker::ByteView(value, 644U)).code() == blinker::ErrorCode::InvalidEncoding);
    CHECK(task.clear().code() == blinker::ErrorCode::InvalidEncoding);
    CHECK(foreign->size == 3U && foreign->value[0] == 42U && fake_preferences::find("ts1") == nullptr);
}

void testCheckedReadsAndFaultLatch() {
    uint8_t first[644U], second[644U], output[644U];
    fill(first, sizeof(first), 3U); fill(second, sizeof(second), 7U);
    for (unsigned fault = 0U; fault < 4U; ++fault) {
        fake_preferences::reset();
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        TaskBlob task(bank);
        CHECK(task.replace(blinker::ByteView(first, sizeof(first))));
        CHECK(task.replace(blinker::ByteView(second, sizeof(second))));
        const unsigned beforePuts = fake_preferences::state().putCalls;
        if (fault == 3U) fake_preferences::failRead("ts0");
        else fake_preferences::failMetadata(fault == 1U ? fake_preferences::MetadataLength : fake_preferences::MetadataType,
                                             "ts0", fault == 2U);
        uint8_t workspace[644U] = {};
        blinker::TaskSnapshotStore managed(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
        blinker::TaskSnapshotHeader header;
        blinker::TaskSnapshotRecord records[4U];
        size_t count = 99U;
        const blinker::Result result = managed.load(header, records, 4U, count);
        CHECK(!result && result.code() != blinker::ErrorCode::NotFound);
        CHECK(managed.failed() && count == 0U && blinker::renesasUnoModemOwner().quarantined());
        const unsigned stoppedCalls = fake_preferences::state().rpcCalls;
        CHECK(managed.replace(header, nullptr, 0U).code() == blinker::ErrorCode::StateConflict);
        CHECK(managed.clear().code() == blinker::ErrorCode::StateConflict);
        size_t written = 99U;
        CHECK(!task.load(blinker::MutableByteSpan(output, sizeof(output)), written) && written == 0U);
        CHECK(!task.replace(blinker::ByteView(first, sizeof(first))));
        bank.end(); CHECK(!bank.begin());
        CHECK(fake_preferences::state().putCalls == beforePuts && fake_preferences::state().rpcCalls == stoppedCalls);
        // Only a modeled whole MCU/connectivity-module restart clears the
        // protocol quarantine; end()/begin() cannot establish that boundary.
        fake_preferences::reboot(); CHECK(bank.begin());
        TaskBlob recovered(bank);
        expect(recovered, second, sizeof(second));
    }
}

void testActualNamespaceIsolation() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    TaskBlob task(bank);
    const uint8_t first[] = {7U, 9U, 11U};
    CHECK(task.replace(blinker::ByteView(first, sizeof(first))));
    bank.end();
    {
        blinker::RenesasUnoStorageConfig config;
        config.namespaceName = "other_store";
        blinker::RenesasUnoPreferencesBlobBank other(config);
        CHECK(other.begin());
        TaskBlob isolated(other);
        uint8_t output[8U]; size_t written = 0U;
        CHECK(isolated.load(blinker::MutableByteSpan(output, sizeof(output)), written).code() == blinker::ErrorCode::NotFound);
        const uint8_t different[] = {2U, 4U};
        CHECK(isolated.replace(blinker::ByteView(different, sizeof(different))));
        expect(isolated, different, sizeof(different));
        other.end();
    }
    CHECK(bank.begin());
    expect(task, first, sizeof(first));
}

void testAbsentRpcAndUncertainWriteLatch() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    TaskBlob task(bank);
    uint8_t workspace[644U] = {};
    size_t written = 99U;
    // Even an actually empty table is absent only after a successful RPC.
    fake_preferences::failMetadata(fake_preferences::MetadataType, "ts0");
    CHECK(task.load(blinker::MutableByteSpan(workspace, sizeof(workspace)), written).code() ==
          blinker::ErrorCode::WouldBlock && written == 0U);
    CHECK(fake_preferences::state().putCalls == 0U);
    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());

    blinker::TaskSnapshotHeader header;
    header.authority.deviceInstanceId[0] = 1U;
    header.authority.taskScope[0] = 2U;
    header.authority.rootDigest[0] = 3U;
    blinker::TaskSnapshotHeader loaded;
    size_t count = 99U;
    blinker::TaskSnapshotStore managed(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
    CHECK(managed.load(loaded, nullptr, 0U, count).code() == blinker::ErrorCode::NotFound && count == 0U);
    fake_preferences::failNext(fake_preferences::ApplyButFailMetadata);
    CHECK(managed.replace(header, nullptr, 0U).code() == blinker::ErrorCode::WouldBlock);
    CHECK(managed.failed());
    const unsigned uncertainWrites = fake_preferences::state().putCalls;
    CHECK(managed.replace(header, nullptr, 0U).code() == blinker::ErrorCode::StateConflict);
    CHECK(managed.clear().code() == blinker::ErrorCode::StateConflict);
    CHECK(fake_preferences::state().putCalls == uncertainWrites);

    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
    blinker::TaskSnapshotStore recovered(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
    CHECK(recovered.load(loaded, nullptr, 0U, count));
    CHECK(loaded.authority == header.authority && count == 0U);
    fake_preferences::failNext(fake_preferences::ApplyButFailReadback);
    CHECK(recovered.clear().code() == blinker::ErrorCode::WouldBlock && recovered.failed());
    const unsigned uncertainClear = fake_preferences::state().putCalls;
    CHECK(recovered.clear().code() == blinker::ErrorCode::StateConflict);
    CHECK(recovered.replace(header, nullptr, 0U).code() == blinker::ErrorCode::StateConflict);
    CHECK(fake_preferences::state().putCalls == uncertainClear);
}

void testOperationBudgetAndOwnership() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    blinker::RenesasUnoModemOwner& owner = blinker::renesasUnoModemOwner();
    uint8_t value[644U] = {}, output[644U] = {};
    TaskBlob task(bank, 10U);
    {
        blinker::RenesasUnoModemOwner::Operation active(owner, &bank, 10U);
        CHECK(active.result() && !owner.networkAllowed());
        const unsigned calls = fake_preferences::state().rpcCalls;
        CHECK(task.replace(blinker::ByteView(value, sizeof(value))).code() == blinker::ErrorCode::WouldBlock);
        CHECK(fake_preferences::state().rpcCalls == calls && fake_preferences::state().putCalls == 0U);
        CHECK(owner.prepare());
        fake_preferences::state().nowMillis += 10U;
        CHECK(owner.prepare().code() == blinker::ErrorCode::WouldBlock);
        CHECK(owner.outcome() == blinker::RenesasUnoModemOwner::DeadlineBeforeWrite && !owner.quarantined());
    }
    CHECK(owner.networkAllowed() && fake_preferences::state().timeoutMillis == MODEM_TIMEOUT);

    // Native calls in one journal operation share the same decreasing budget,
    // not a fresh timeout for each metadata/GET/PUT invocation.
    fake_preferences::state().rpcMillis = 1U;
    const unsigned firstTimeout = fake_preferences::state().timeoutCalls;
    rpcReentryTask = &task; rpcReentryDenied = false;
    fake_preferences::state().rpcHook = &tryRpcReentry;
    CHECK(task.replace(blinker::ByteView(value, sizeof(value))));
    CHECK(rpcReentryDenied); rpcReentryTask = nullptr;
    const unsigned lastTimeout = fake_preferences::state().timeoutCalls;
    CHECK(lastTimeout > firstTimeout + 2U);
    uint32_t previous = 10U;
    for (unsigned index = firstTimeout; index + 1U < lastTimeout; ++index) {
        CHECK(fake_preferences::state().timeoutHistory[index] <= previous);
        previous = fake_preferences::state().timeoutHistory[index];
    }
    CHECK(fake_preferences::state().timeoutMillis == MODEM_TIMEOUT && !owner.quarantined());
    CHECK(previous < fake_preferences::state().timeoutHistory[firstTimeout]);

    // A late, complete read is still excluded from the current operation.
    const unsigned puts = fake_preferences::state().putCalls;
    fake_preferences::state().nextRpcMillis = 10U;
    size_t written = 99U;
    CHECK(task.load(blinker::MutableByteSpan(output, sizeof(output)), written).code() == blinker::ErrorCode::WouldBlock);
    CHECK(written == 0U && owner.quarantined() && owner.outcome() == blinker::RenesasUnoModemOwner::ReadUncertain);
    CHECK(fake_preferences::state().putCalls == puts && fake_preferences::state().timeoutMillis == MODEM_TIMEOUT);
    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
    expect(task, value, sizeof(value));

    // Header transmission already promised the payload: finish it even when
    // transmission consumes the window, with zero fresh acknowledgement wait.
    fake_preferences::state().writeHeaderMillis = 10U;
    value[0] = 9U;
    const unsigned beforeHeaderPut = fake_preferences::state().putCalls;
    CHECK(task.replace(blinker::ByteView(value, sizeof(value))).code() == blinker::ErrorCode::WouldBlock);
    CHECK(fake_preferences::state().putCalls == beforeHeaderPut + 1U && fake_preferences::state().timeoutAtPassthrough == 0U);
    CHECK(owner.quarantined() && owner.outcome() == blinker::RenesasUnoModemOwner::WriteUncertain);
    CHECK(fake_preferences::state().getCalls == fake_preferences::state().getCallsAtPut);
    CHECK(fake_preferences::state().metadataCalls == fake_preferences::state().metadataCallsAtPut);
    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
    expect(task, value, sizeof(value));

    // PUT can be durably applied while its acknowledgement crosses deadline.
    // No verification GET or replacement follows on the quarantined protocol.
    fake_preferences::state().passthroughMillis = 10U;
    value[0] = 7U;
    CHECK(task.replace(blinker::ByteView(value, sizeof(value))).code() == blinker::ErrorCode::WouldBlock);
    CHECK(owner.quarantined() && owner.outcome() == blinker::RenesasUnoModemOwner::WriteUncertain);
    CHECK(fake_preferences::state().getCalls == fake_preferences::state().getCallsAtPut);
    CHECK(fake_preferences::state().metadataCalls == fake_preferences::state().metadataCallsAtPut);
    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
    expect(task, value, sizeof(value));

    // uint32 rollover still computes elapsed time and remaining timeout.
    fake_preferences::state().nowMillis = UINT32_MAX - 2U;
    fake_preferences::state().rpcMillis = 1U;
    CHECK(task.load(blinker::MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == sizeof(value) && fake_preferences::state().nowMillis < 10U && !owner.quarantined());
}

blinker::TaskSnapshotHeader emptyTaskHeader() {
    blinker::TaskSnapshotHeader header;
    header.authority.deviceInstanceId[0] = 1U;
    header.authority.taskScope[0] = 2U;
    header.authority.rootDigest[0] = 3U;
    return header;
}

void testJournalWriteAdmissionDeadline() {
    for (bool clearing : {false, true}) {
        fake_preferences::reset();
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        // Empty REPLACE inspects two TYPE replies (four clock observations).
        // CLEAR inspects TYPE/TYPE/LEN/GET (eight clock observations). The
        // NEXT prepare at the write boundary must deny, not start a PUT.
        TaskBlob task(bank, clearing ? 9U : 5U);
        uint8_t workspace[644U] = {};
        blinker::TaskSnapshotStore managed(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
        blinker::TaskSnapshotHeader loaded;
        size_t count = 99U;
        CHECK(managed.load(loaded, nullptr, 0U, count).code() == blinker::ErrorCode::NotFound && count == 0U);
        const auto header = emptyTaskHeader();
        if (clearing) CHECK(managed.replace(header, nullptr, 0U));
        const unsigned puts = fake_preferences::state().putCalls;
        fake_preferences::state().nowMillis = 0U;
        fake_preferences::state().tickMillis = 1U;
        const blinker::Result result = clearing ? managed.clear() : managed.replace(header, nullptr, 0U);
        CHECK(result.code() == blinker::ErrorCode::WouldBlock && managed.failed());
        CHECK(blinker::renesasUnoModemOwner().outcome() == blinker::RenesasUnoModemOwner::DeadlineBeforeWrite);
        CHECK(!blinker::renesasUnoModemOwner().quarantined() && blinker::renesasUnoModemOwner().networkAllowed());
        CHECK(fake_preferences::state().putCalls == puts && fake_preferences::state().timeoutMillis == MODEM_TIMEOUT);
        const unsigned calls = fake_preferences::state().rpcCalls;
        CHECK(managed.replace(header, nullptr, 0U).code() == blinker::ErrorCode::StateConflict);
        CHECK(managed.clear().code() == blinker::ErrorCode::StateConflict);
        CHECK(fake_preferences::state().rpcCalls == calls && fake_preferences::state().putCalls == puts);
        // The owner is healthy but the Store retains its independent failure
        // latch: absence/old media is not permission to replay this request.
        fake_preferences::state().tickMillis = 0U;
        CHECK(fake_preferences::find("ts0") == nullptr);
        CHECK((fake_preferences::find("ts1") != nullptr) == clearing);
    }
}

void testLateClearIsUnknownAndNeverRetried() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    TaskBlob task(bank, 10U);
    uint8_t workspace[644U] = {};
    blinker::TaskSnapshotStore managed(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
    blinker::TaskSnapshotHeader loaded;
    size_t count = 99U;
    CHECK(managed.load(loaded, nullptr, 0U, count).code() == blinker::ErrorCode::NotFound);
    CHECK(managed.replace(emptyTaskHeader(), nullptr, 0U));
    fake_preferences::state().passthroughMillis = 10U;
    CHECK(managed.clear().code() == blinker::ErrorCode::WouldBlock && managed.failed());
    CHECK(blinker::renesasUnoModemOwner().quarantined());
    CHECK(blinker::renesasUnoModemOwner().outcome() == blinker::RenesasUnoModemOwner::WriteUncertain);
    CHECK(fake_preferences::state().getCalls == fake_preferences::state().getCallsAtPut);
    CHECK(fake_preferences::state().metadataCalls == fake_preferences::state().metadataCallsAtPut);
    const unsigned calls = fake_preferences::state().rpcCalls;
    const unsigned puts = fake_preferences::state().putCalls;
    CHECK(managed.clear().code() == blinker::ErrorCode::StateConflict);
    CHECK(fake_preferences::state().rpcCalls == calls && fake_preferences::state().putCalls == puts);
    bank.end(); fake_preferences::reboot(); CHECK(bank.begin());
    blinker::TaskSnapshotStore recovered(task, blinker::MutableByteSpan(workspace, sizeof(workspace)));
    CHECK(recovered.load(loaded, nullptr, 0U, count).code() == blinker::ErrorCode::NotFound && count == 0U);
    CHECK(!recovered.failed() && fake_preferences::state().putCalls == puts);
}

void makeProbeSnapshot(uint8_t phase, uint8_t (&output)[644U]) {
    blinker::TaskSnapshotHeader header;
    header.authority.deviceInstanceId[0] = 1U;
    header.authority.taskScope[0] = 2U;
    header.authority.rootDigest[0] = 3U;
    header.tableRevision = phase; header.nextTaskId = 5U;
    header.requestId[0] = phase; header.requestDigest[0] = phase;
    blinker::TaskSnapshotRecord records[4U];
    for (size_t i = 0U; i < 4U; ++i) {
        records[i].taskId = static_cast<uint32_t>(i + 1U);
        records[i].definitionRevision = phase;
        records[i].timing.durationMillis = 1000U + static_cast<uint32_t>(i);
        records[i].key[0] = 'p'; records[i].keyLength = 1U;
        records[i].value[0] = phase == 1U ? 0xF4U : 0xF5U; records[i].valueLength = 1U;
    }
    size_t written = 0U;
    CHECK(blinker::TaskSnapshotCodec::encode(header, records, 4U,
          blinker::MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == sizeof(output));
}

void expectProbeSlotShape(uint8_t phase) {
    const fake_preferences::Entry* entries[2] = {
        fake_preferences::find("ts0"), fake_preferences::find("ts1")};
    const bool present[2] = {entries[0] != nullptr, entries[1] != nullptr};
    const uint32_t sizes[2] = {entries[0] == nullptr ? 0U : static_cast<uint32_t>(entries[0]->size),
                             entries[1] == nullptr ? 0U : static_cast<uint32_t>(entries[1]->size)};
    CHECK(task_probe::matchesTaskSlotShape(phase, present, sizes, TaskBlob::journalScratchSize));
    // Admission is invariant under slot order, unlike the faulty ts0-first
    // check that rejected a successful real-board first write to ts1.
    const bool reversedPresent[2] = {present[1], present[0]};
    const uint32_t reversedSizes[2] = {sizes[1], sizes[0]};
    CHECK(task_probe::matchesTaskSlotShape(phase, reversedPresent, reversedSizes, TaskBlob::journalScratchSize));
    for (uint8_t other = 0U; other <= 3U; ++other)
        if (other != phase) CHECK(!task_probe::matchesTaskSlotShape(other, present, sizes, TaskBlob::journalScratchSize));
}

void expectProbePhase(TaskBlob& store, const uint8_t (&expected)[644U], uint8_t phase) {
    uint8_t output[644U] = {};
    size_t written = 0U, count = 0U;
    CHECK(store.load(blinker::MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == sizeof(output) && std::memcmp(output, expected, written) == 0);
    blinker::TaskSnapshotHeader header;
    blinker::TaskSnapshotRecord records[4U];
    CHECK(blinker::TaskSnapshotCodec::decode(blinker::ByteView(output, written), header, records, 4U, count));
    CHECK(count == 4U && header.tableRevision == phase);
    for (size_t i = 0U; i < count; ++i)
        CHECK(!records[i].enabled && records[i].progress == blinker::TaskProgress::None && records[i].startSequence == 0U);
}

void testProbeSlotOrderAgainstActualStore() {
    for (unsigned reverse = 0U; reverse < 2U; ++reverse) {
        fake_preferences::reset();
        blinker::RenesasUnoPreferencesBlobBank bank;
        CHECK(bank.begin());
        TaskBlob firstBoot(bank);
        uint8_t first[644U] = {}, second[644U] = {};
        makeProbeSnapshot(1U, first); makeProbeSnapshot(2U, second);
        expectProbeSlotShape(0U);
        CHECK(firstBoot.replace(blinker::ByteView(first, sizeof(first))));
        expectProbeSlotShape(1U);
        expectProbePhase(firstBoot, first, 1U);
        CHECK(fake_preferences::state().putCalls == 1U);
        if (reverse != 0U) {
            // Only the host media label changes; keep the actual journal bytes,
            // generation and CRC untouched to exercise a valid opposite slot.
            fake_preferences::Entry* active = fake_preferences::find("ts0");
            if (active != nullptr) fake_preferences::copyLabel(active->key, sizeof(active->key), "ts1");
            else {
                active = fake_preferences::find("ts1"); CHECK(active != nullptr);
                fake_preferences::copyLabel(active->key, sizeof(active->key), "ts0");
            }
        }
        // Real-board continuation is READ after reopening, then REPLACE2;
        // never repeat the already successful first write.
        bank.end(); CHECK(bank.begin());
        TaskBlob recovered(bank);
        expectProbeSlotShape(1U);
        expectProbePhase(recovered, first, 1U);
        CHECK(recovered.replace(blinker::ByteView(second, sizeof(second))));
        expectProbeSlotShape(2U);
        expectProbePhase(recovered, second, 2U);
        CHECK(fake_preferences::state().putCalls == 2U);
        CHECK(recovered.clear());
        expectProbeSlotShape(3U);
        CHECK(fake_preferences::state().putCalls == 3U);
        size_t written = 99U;
        CHECK(recovered.load(blinker::MutableByteSpan(first, sizeof(first)), written).code() ==
              blinker::ErrorCode::NotFound && written == 0U);
        bank.end(); CHECK(bank.begin());
        TaskBlob afterClear(bank);
        expectProbeSlotShape(3U);
        CHECK(afterClear.load(blinker::MutableByteSpan(first, sizeof(first)), written).code() == blinker::ErrorCode::NotFound);
    }
    const bool present[2] = {true, false};
    const uint32_t wrongSizes[2] = {663U, 0U};
    CHECK(!task_probe::matchesTaskSlotShape(1U, present, wrongSizes, TaskBlob::journalScratchSize));
}

} // namespace

int main() {
    testSharedOwnerFullSnapshotAndOldBytes();
    testFailuresRecoveryAndTombstone();
    testCapacityConflictAndGeneration();
    testForeignKeyConflict();
    testCheckedReadsAndFaultLatch();
    testActualNamespaceIsolation();
    testAbsentRpcAndUncertainWriteLatch();
    testOperationBudgetAndOwnership();
    testJournalWriteAdmissionDeadline();
    testLateClearIsUnknownAndNeverRetried();
    testProbeSlotOrderAgainstActualStore();
    std::cout << "UNO R4 shared Preferences task snapshot tests passed\n";
    return 0;
}
