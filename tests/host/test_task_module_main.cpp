#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#include <type_traits>
#include "BlinkerV2/arduino/platform/TaskModule.h"
#include "support/TaskAuthorityFixture.h"

using namespace blinker;
using blinker::integration::official_detail::TaskModule;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

namespace {
size_t heapAllocations = 0U;
}

void* operator new(size_t size) {
    ++heapAllocations;
    void* allocation = std::malloc(size == 0U ? 1U : size);
    if (allocation == nullptr) throw std::bad_alloc();
    return allocation;
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }

namespace {
struct Environment : ITaskEnvironment {
    uint32_t now = 0U;
    uint64_t utc = 1790035200000ULL;
    void advance(uint32_t elapsed) { now += elapsed; utc += elapsed; }
    void observe(TaskObservation& output) override {
        output = TaskObservation();
        output.nowMillis = now;
        output.authority.binding = testAuthority();
        output.authority.state = TaskAuthorityState::Current;
        output.time.utcMillis = utc;
        output.time.sampleRevision = 1U;
        output.time.source = WallTimeSource::Server;
        output.time.state = WallClockState::Synchronized;
    }
};

template <size_t Capacity>
struct Flash : IAtomicBlobStore {
    enum : size_t {
        maximumSize = TaskSnapshotCodec::headerSize + Capacity * TaskSnapshotCodec::recordSize +
                      TaskSnapshotCodec::trailerSize
    };
    uint8_t bytes[maximumSize] = {};
    size_t size = 0U;
    size_t loadCapacity = 0U;
    unsigned opens = 0U, loads = 0U, writes = 0U, clears = 0U;
    bool opened = false;
    ErrorCode loadError = ErrorCode::Ok;
    void open() { ++opens; opened = true; } // The platform owner, not TaskModule.
    Result load(MutableByteSpan output, size_t& written) override {
        ++loads;
        loadCapacity = output.size;
        written = 0U;
        if (!opened) return Result::failure(ErrorCode::NotConfigured);
        if (loadError != ErrorCode::Ok) return Result::failure(loadError);
        if (size == 0U) return Result::failure(ErrorCode::NotFound);
        if (output.size < size) return Result::failure(ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, size);
        written = size;
        return Result::success();
    }
    Result replace(ByteView input) override {
        ++writes;
        CHECK(opened && input.size <= maximumSize);
        std::memcpy(bytes, input.data, input.size);
        size = input.size;
        return Result::success();
    }
    Result clear() override {
        ++clears;
        CHECK(opened);
        size = 0U;
        return Result::success();
    }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};

struct Target : ITaskTarget {
    unsigned calls = 0U;
    Result validate(const TaskSnapshotRecord&) override { return Result::success(); }
    Result apply(const TaskSnapshotRecord&) override { ++calls; return Result::success(); }
};

TaskWritePolicy policy() {
    TaskWritePolicy value;
    value.windowMillis = 100U;
    value.writesPerWindow = 100U;
    value.minimumStartMillis = 10U;
    value.maximumCommitMillis = 100U;
    return value; // Fault-free host fixture only, not production admission.
}

TaskMutation create(uint8_t requestId, uint32_t revision, uint64_t due) {
    TaskMutation request;
    request.requestId[0] = requestId;
    request.expectedRevision = revision;
    request.definition.enabled = true;
    request.definition.timing.kind = TaskTimingKind::UtcOnce;
    request.definition.timing.utcMillis = due;
    request.definition.key[0] = 'p';
    request.definition.keyLength = 1U;
    request.definition.value[0] = 0xF5U;
    request.definition.valueLength = 1U;
    return request;
}

template <size_t Capacity>
void openedStoreAndRecovery() {
    const size_t before = heapAllocations;
    Flash<Capacity> flash;
    Environment environment;
    Target target;
    flash.open();
    {
        TaskModule<Capacity> module(flash, environment, target, policy());
        CHECK(flash.loads == 0U && flash.writes == 0U && flash.opens == 1U);
        CHECK(module.engine().capacity() == Capacity);
        CHECK(module.begin().code() == ErrorCode::NotFound);
        CHECK(module.engine().state() == TaskEngineState::Empty);
        CHECK(flash.loads == 1U && flash.loadCapacity == Flash<Capacity>::maximumSize);
        CHECK(flash.opens == 1U && flash.writes == 0U && flash.clears == 0U);
        CHECK(module.begin().code() == ErrorCode::AlreadyExists && flash.loads == 1U);
        environment.advance(100U);
        CHECK(module.engine().initializeVerified(testAuthority()));
        TaskMutationReceipt receipt;
        for (size_t i = 0U; i < Capacity; ++i)
            CHECK(module.engine().mutateVerified(testAuthority(),
                create(static_cast<uint8_t>(i + 1U), static_cast<uint32_t>(i), environment.utc + 1000U), receipt));
        CHECK(module.engine().count() == Capacity);
        CHECK(flash.size == TaskSnapshotCodec::encodedSize(Capacity));
        const unsigned writes = flash.writes;
        CHECK(!module.engine().mutateVerified(testAuthority(),
            create(static_cast<uint8_t>(Capacity + 1U), static_cast<uint32_t>(Capacity), environment.utc + 1000U), receipt));
        CHECK(flash.writes == writes && module.engine().count() == Capacity);
    }
    environment.now = 0U; // Fresh runtime, same independently opened durable store.
    {
        TaskModule<Capacity> recovered(flash, environment, target, policy());
        CHECK(recovered.begin() && flash.loads == 2U && flash.opens == 1U);
        CHECK(recovered.engine().count() == Capacity);
        TaskSnapshotRecord record;
        CHECK(recovered.engine().recordAt(Capacity - 1U, record) && record.taskId == Capacity);
        environment.advance(1000U);
        TaskPollReport report;
        for (size_t i = 0U; i < Capacity; ++i)
            CHECK(recovered.engine().poll(report) && report.callbackInvoked && report.progress == TaskProgress::Completed);
        CHECK(target.calls == Capacity && flash.writes == 1U + 3U * Capacity);
        CHECK(recovered.engine().poll(report) && !report.callbackInvoked);
        CHECK(recovered.begin().code() == ErrorCode::AlreadyExists && flash.loads == 2U);
    }
    CHECK(flash.opened && flash.opens == 1U && flash.clears == 0U);
    CHECK(heapAllocations == before);
    std::printf("%zu tasks: portable task module=%zu bytes, workspace=%zu bytes\n",
                Capacity, sizeof(TaskModule<Capacity>), static_cast<size_t>(Flash<Capacity>::maximumSize));
}

void failedBeginDoesNotRetry() {
    const size_t before = heapAllocations;
    Environment environment;
    Target target;
    {
        Flash<1U> flash; // Owner has not opened it: module never opens on its behalf.
        TaskModule<1U> module(flash, environment, target, policy());
        CHECK(module.begin().code() == ErrorCode::NotConfigured);
        CHECK(module.engine().state() == TaskEngineState::Fault && flash.loads == 1U && flash.opens == 0U);
        flash.open();
        CHECK(module.begin().code() == ErrorCode::AlreadyExists && flash.loads == 1U);
        CHECK(flash.writes == 0U && flash.clears == 0U);
    }
    {
        Flash<1U> flash;
        flash.open(); flash.loadError = ErrorCode::InternalError;
        TaskModule<1U> module(flash, environment, target, policy());
        CHECK(module.begin().code() == ErrorCode::InternalError);
        flash.loadError = ErrorCode::Ok;
        CHECK(module.begin().code() == ErrorCode::AlreadyExists && flash.loads == 1U);
        CHECK(module.engine().state() == TaskEngineState::Fault && flash.writes == 0U);
    }
    {
        Flash<1U> flash;
        flash.open(); flash.bytes[0] = 0xA5U; flash.size = 1U; // Decode failure is terminal too.
        TaskModule<1U> module(flash, environment, target, policy());
        CHECK(!module.begin() && module.engine().state() == TaskEngineState::Fault);
        flash.size = 0U;
        CHECK(module.begin().code() == ErrorCode::AlreadyExists && flash.loads == 1U);
    }
    {
        Flash<1U> flash;
        flash.open();
        TaskModule<1U> module(flash, environment, target, TaskWritePolicy());
        CHECK(module.begin().code() == ErrorCode::InvalidArgument);
        CHECK(module.begin().code() == ErrorCode::AlreadyExists && flash.loads == 0U);
        CHECK(module.engine().state() == TaskEngineState::Fault && flash.writes == 0U);
    }
    CHECK(heapAllocations == before && target.calls == 0U);
}

static_assert(!std::is_copy_constructible<TaskModule<4U>>::value, "Task storage must not be copied");
static_assert(!std::is_move_constructible<TaskModule<4U>>::value, "Task storage must not be moved");
static_assert(sizeof(TaskModule<8U>) - sizeof(TaskModule<4U>) ==
              4U * (TaskSnapshotCodec::recordSize + sizeof(TaskSnapshotRecord) + sizeof(TaskRuntimeSlot)),
              "Task module storage must scale with its configured capacity");
} // namespace

int main() {
    openedStoreAndRecovery<1U>();
    openedStoreAndRecovery<4U>();
    openedStoreAndRecovery<8U>();
    openedStoreAndRecovery<10U>();
    failedBeginDoesNotRetry();
    std::puts("portable task module opened-store, recovery, single-begin and no-heap checks passed");
}
