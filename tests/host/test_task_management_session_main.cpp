#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>
#include "BlinkerV2/provisioning/TaskManagementSession.h"
#include "BlinkerV2/api/TaskProductLifecycle.h"
#include "BlinkerV2/api/Product.h"
#include "BlinkerV2/protocol/cbor/Cbor.h"
#include "BlinkerV2/runtime/TaskCloudTransport.h"
#include "BlinkerV2/identity/ConfiguredDeviceKeySource.h"

using namespace blinker;
template class blinker::TaskCloudTransport<>; // Compile the real ManagedMqtt composition as well.
#if UINTPTR_MAX <= UINT32_MAX
static_assert(sizeof(TaskCloudTransport<>) <= 96U, "Task carrier exceeds its 32-bit object budget");
#endif
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)
namespace {
using Bytes = std::vector<uint8_t>;
const uint64_t channel = 17U;
struct Clock : IClock {
    uint32_t now = 100U;
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t&) const override { return false; }
};
struct KeySource : IDeviceKeySource {
    unsigned loads = 0U;
    uint8_t version = 1U;
    bool fail = false;
    std::function<void()> hook;
    Result load(DeviceKey& output) override {
        ++loads;
        memset(output.bytes, version, sizeof(output.bytes));
        if (hook) hook();
        return fail ? Result::failure(ErrorCode::InternalError) : Result::success();
    }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};
struct Random : IRandom {
    uint8_t counter = 0U;
    bool zero = false, fail = false;
    std::function<void()> hook;
    Result fill(MutableByteSpan output) override {
        memset(output.data, zero ? 0U : ++counter, output.size);
        if (hook) hook();
        return fail ? Result::failure(ErrorCode::NotConfigured) : Result::success();
    }
};
// NOT crypto: exact signed-digest boundary double. Grant/operation real crypto
// is independently covered by the Node golden oracle, not this fake provider.
struct Verifier : IServerSignatureVerifier {
    uint8_t expected[32] = {};
    unsigned calls = 0U;
    bool fail = false;
    std::function<void()> hook;
    Result verifyDigest(uint32_t key, ServerSignatureAlgorithm algorithm, ByteView digest, ByteView signature) override {
        ++calls;
        const bool match = key == 1U && algorithm == ServerSignatureAlgorithm::Ed25519 && signature.size == 64U &&
            constantTimeEqual(digest, ByteView(expected, sizeof(expected)));
        if (hook) hook();
        return match && !fail ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
    }
    Bytes sign(const TaskManagementChallenge& challenge) {
        TaskManagementGrant grant;
        grant.binding.deviceInstanceId = ByteView(challenge.authority.deviceInstanceId, 16U);
        grant.binding.taskScope = ByteView(challenge.authority.taskScope, 16U);
        grant.binding.nonce = ByteView(challenge.nonce, 16U);
        grant.binding.requestId = ByteView(challenge.requestId, 16U);
        grant.binding.operationDigest = ByteView(challenge.operationDigest, 32U);
        grant.serverKeyId = 1U;
        const uint8_t placeholder[64] = {1U}; grant.signature = ByteView(placeholder, 64U);
        CHECK(taskManagementGrantDigest(grant, MutableByteSpan(expected, sizeof(expected))));
        uint8_t buffer[kTaskManagementGrantMaxSize]; ByteView encoded;
        CHECK(encodeTaskManagementGrant(grant, MutableByteSpan(buffer, sizeof(buffer)), encoded));
        return Bytes(encoded.data, encoded.data + encoded.size);
    }
};
struct Flash : IAtomicBlobStore {
    Bytes bytes;
    unsigned writes = 0U, loads = 0U;
    bool fail = false;
    std::function<void()> hook;
    Result load(MutableByteSpan output, size_t& written) override {
        ++loads; written = 0U;
        if (bytes.empty()) return Result::failure(ErrorCode::NotFound);
        CHECK(output.size >= bytes.size());
        memcpy(output.data, bytes.data(), bytes.size()); written = bytes.size(); return Result::success();
    }
    Result replace(ByteView value) override {
        ++writes; bytes.assign(value.data, value.data + value.size);
        if (hook) hook();
        return fail ? Result::failure(ErrorCode::InternalError) : Result::success();
    }
    Result clear() override { bytes.clear(); return Result::success(); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
};
struct Environment : ITaskEnvironment {
    TaskAuthorityCache& cache;
    Clock& clock;
    Environment(TaskAuthorityCache& a, Clock& c) : cache(a), clock(c) {}
    void observe(TaskObservation& output) override {
        output = TaskObservation(); output.nowMillis = clock.now;
        TaskAuthorityCache::read(&cache, output.authority);
    }
};
struct Target : ITaskTarget {
    unsigned applied = 0U;
    bool changed = false, blockValidation = false;
    Result describe(size_t index, TaskTargetView& output, size_t& total) override {
        total = 2U; output = TaskTargetView();
        if (index == UINT32_MAX) return Result::success();
        if (index >= total) return Result::failure(ErrorCode::NotFound);
        output.key = StringView(index == 0U ? "power" : "level", 5U);
        output.valueType = index == 0U ? ValueType::Boolean : changed ? ValueType::SignedInteger : ValueType::UnsignedInteger;
        return Result::success();
    }
    Result validate(const TaskSnapshotRecord&) override { return blockValidation ? Result::failure(ErrorCode::WouldBlock) : Result::success(); }
    Result apply(const TaskSnapshotRecord&) override { ++applied; return Result::success(); }
};
TaskWritePolicy policy() {
    TaskWritePolicy value; value.windowMillis = 100U; value.writesPerWindow = 100U;
    value.minimumStartMillis = 100U; value.maximumCommitMillis = 100U; return value;
}
struct Fixture {
    Clock clock;
    KeySource key;
    TaskRootSources sources;
    DeviceInstanceId instance;
    TaskAuthorityCache cache;
    Environment environment;
    Target target;
    Flash flash;
    uint8_t workspace[TaskSnapshotCodec::headerSize + 2U * TaskSnapshotCodec::recordSize + TaskSnapshotCodec::trailerSize] = {};
    TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
    TaskSnapshotStore store;
    TaskEngine engine;
    Random random; Verifier verifier;
    TaskManagementSession session;
    uint8_t scope[16] = {9U};
    uint8_t requestId[16] = {3U};
    explicit Fixture(const TaskWritePolicy& limits = policy()) : environment(cache, clock), store(flash, MutableByteSpan(workspace, sizeof(workspace))),
        engine(store, environment, target, records, slots, 2U, limits),
        session(engine, cache, random, verifier, clock, 1000U, 10U) {
        memset(instance.bytes, 7U, sizeof(instance.bytes)); sources.deviceKey = &key;
        CHECK(cache.refresh(instance, sources));
        CHECK(engine.begin().code() == ErrorCode::NotFound);
        clock.now += 100U; // Refill the Engine's initially empty write budget.
    }
    TaskAuthorityChange change(TaskAuthorityBinding& root, ByteView previous = ByteView()) {
        CHECK(cache.root(root));
        TaskAuthorityChange value;
        value.deviceInstanceId = ByteView(instance.bytes, 16U); value.taskScope = ByteView(scope, 16U);
        value.requestId = ByteView(requestId, 16U); value.currentRoot = ByteView(root.rootDigest, 32U);
        value.previousRoot = previous; return value;
    }
    Result commit(const TaskManagementChallenge& challenge, TaskManagementResult& out) {
        const Bytes bytes = verifier.sign(challenge);
        TaskManagementChallenge completed;
        const Result result = session.commit(channel, ByteView(bytes.data(), bytes.size()), out, &completed);
        if (!result) { CHECK(!completed.authority.valid()); return result; }
        CHECK(completed.authority == challenge.authority);
        CHECK(constantTimeEqual(ByteView(completed.nonce, 16U), ByteView(challenge.nonce, 16U)));
        CHECK(constantTimeEqual(ByteView(completed.requestId, 16U), ByteView(challenge.requestId, 16U)));
        CHECK(constantTimeEqual(ByteView(completed.operationDigest, 32U), ByteView(challenge.operationDigest, 32U)));
        uint8_t secret[32]; memset(secret, key.version, sizeof(secret)); // Synthetic key source only.
        uint8_t wire[kTaskManagementResultMaxSize]; ByteView encoded;
        CHECK(encodeTaskManagementResult(completed, out, TaskResultKey::DeviceKey, ByteView(secret, sizeof(secret)),
            MutableByteSpan(wire, sizeof(wire)), encoded));
        TaskManagementResult verified;
        CHECK(verifyTaskManagementResult(encoded, challenge, TaskResultKey::DeviceKey, ByteView(secret, sizeof(secret)), verified));
        CHECK(verified.tableRevision == out.tableRevision && verified.count == out.count && verified.kind == out.kind);
        CHECK(verified.hasRecord == out.hasRecord && verified.index == out.index);
        CHECK(verified.rejection == out.rejection);
        if (out.hasRecord) CHECK(verified.record.taskId == out.record.taskId && verified.record.progress == out.record.progress);
        return result;
    }
    void initialize() {
        TaskAuthorityBinding root; TaskManagementChallenge challenge; TaskManagementResult out;
        CHECK(session.prepareAuthority(channel, change(root), challenge));
        CHECK(commit(challenge, out)); CHECK(out.tableRevision == 0U && out.count == 0U);
        CHECK(engine.state() == TaskEngineState::Ready && flash.writes == 1U);
        clock.now += 10U;
    }
    TaskMutation create() {
        TaskMutation value;
        memcpy(value.requestId, requestId, 16U); ++value.requestId[0];
        value.definition.enabled = true; value.definition.timing.durationMillis = 1000U;
        value.definition.key[0] = 'p'; value.definition.keyLength = 1U;
        value.definition.value[0] = 0xF5U; value.definition.valueLength = 1U;
        return value;
    }
};

void testAuthenticatedRejection() {
    TaskWritePolicy limited = policy(); limited.writesPerWindow = 2U; limited.windowMillis = 200U;
    Fixture f(limited); f.initialize();
    TaskManagementChallenge challenge; TaskManagementResult out;
    const auto create = f.create();
    CHECK(f.session.prepareMutation(channel, create, challenge));
    CHECK(f.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Rejected && out.rejection == TaskMutationRejection::WriteBudget);
    CHECK(f.flash.writes == 1U && f.engine.tableRevision() == 0U && f.engine.count() == 0U);
    CHECK(!f.commit(challenge, out)); // Rejection consumed the nonce.
    f.clock.now += 100U;
    CHECK(f.session.prepareMutation(channel, create, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Mutation && f.flash.writes == 2U);
    // Persisted replay wins even with an empty bucket; do not mislabel success.
    f.clock.now += 10U;
    CHECK(f.session.prepareMutation(channel, create, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Mutation && out.mutation.replayed && f.flash.writes == 2U);

    Fixture start; start.initialize(); auto value = start.create(); value.definition.timing.durationMillis = 1U;
    CHECK(start.session.prepareMutation(channel, value, challenge)); CHECK(start.commit(challenge, out));
    TaskMutation request; request.kind = TaskMutationKind::Start; request.taskId = 1U;
    request.expectedRevision = 1U; request.requestId[0] = 33U; start.clock.now += 10U;
    CHECK(start.session.prepareMutation(channel, request, challenge)); CHECK(start.commit(challenge, out));
    start.clock.now += 10U; TaskPollReport report; CHECK(start.engine.poll(report) && report.callbackInvoked);
    request.expectedRevision = 2U; request.requestId[0]++;
    const auto writes = start.flash.writes;
    CHECK(start.session.prepareMutation(channel, request, challenge)); CHECK(start.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Rejected && out.rejection == TaskMutationRejection::StartCooldown);
    CHECK(out.tableRevision == 2U && start.flash.writes == writes && start.target.applied == 1U);
    start.clock.now += 100U;
    CHECK(start.session.prepareMutation(channel, request, challenge)); CHECK(start.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Mutation && out.tableRevision == 3U);

    // Grant failure, a root change, or a possible committed write never gets a
    // signed pre-write refusal, even if the operation also lacks write credits.
    for (unsigned fault = 0U; fault < 4U; ++fault) {
        Fixture bad(fault >= 2U ? policy() : limited); bad.initialize();
        CHECK(bad.session.prepareMutation(channel, bad.create(), challenge));
        if (fault == 0U) bad.verifier.fail = true;
        else if (fault == 1U) bad.verifier.hook = [&]() { bad.cache.invalidate(); };
        else if (fault == 2U) bad.flash.fail = true; // replace persists bytes before reporting failure.
        else bad.target.blockValidation = true; // Generic WouldBlock is not write-budget proof.
        CHECK(!bad.commit(challenge, out));
        CHECK(out.kind != TaskManagementResultKind::Rejected && out.rejection == TaskMutationRejection::None);
        CHECK(bad.flash.writes == (fault == 2U ? 2U : 1U));
    }
}

void testTargetCatalog() {
    Fixture f; TaskReadPrepare read; read.requestId[0] = 55U; read.catalog = true;
    TaskManagementChallenge challenge; TaskManagementResult out;
    CHECK(!f.session.prepareRead(channel, read, challenge)); // No current authority.
    f.initialize(); const unsigned writes = f.flash.writes;
    CHECK(f.session.prepareRead(channel, read, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.kind == TaskManagementResultKind::Catalog && out.catalog.capacity == 2U && out.catalog.total == 2U);
    const auto metadata = out.catalog;
    // Independent Node oracle: capacity=2, ordered [(power,bool),(level,uint)].
    const uint8_t golden[32] = {0x67U,0x3bU,0xd7U,0x65U,0xfdU,0x77U,0xe8U,0xe2U,0x2cU,0xefU,0x78U,0x2eU,0x36U,0x66U,0xb8U,0xa4U,0x9fU,0x35U,0xc7U,0xe3U,0xe6U,0xffU,0x4aU,0x4eU,0x75U,0x2fU,0x69U,0x82U,0x8cU,0x0bU,0xb4U,0x3dU};
    CHECK(memcmp(metadata.fingerprint, golden, 32U) == 0);
    for (uint32_t i = 0U; i < 2U; ++i) {
        f.clock.now += 10U; read.index = i; memcpy(read.catalogFingerprint, metadata.fingerprint, 32U);
        CHECK(f.session.prepareRead(channel, read, challenge));
        memset(read.catalogFingerprint, 0, 32U); // Pending owns the original hash.
        CHECK(f.commit(challenge, out));
        CHECK(out.kind == TaskManagementResultKind::Target && out.index == i && out.catalog.keyLength == 5U);
        CHECK(memcmp(out.catalog.key, i == 0U ? "power" : "level", 5U) == 0);
        CHECK(memcmp(out.catalog.fingerprint, metadata.fingerprint, 32U) == 0);
    }
    f.clock.now += 10U; memcpy(read.catalogFingerprint, metadata.fingerprint, 32U);
    CHECK(f.session.prepareRead(channel, read, challenge)); f.target.changed = true;
    CHECK(f.commit(challenge, out).code() == ErrorCode::SequenceConflict);
    f.clock.now += 10U; read.index = 2U;
    CHECK(f.session.prepareRead(channel, read, challenge)); CHECK(!f.commit(challenge, out));
    CHECK(f.flash.writes == writes && f.target.applied == 0U);
    // A fresh metadata read observes the new configuration; old pages cannot
    // be silently combined, even if the task table revision is still zero.
    f.clock.now += 10U; read.index = UINT32_MAX; memset(read.catalogFingerprint, 0, 32U);
    CHECK(f.session.prepareRead(channel, read, challenge)); CHECK(f.commit(challenge, out));
    CHECK(memcmp(out.catalog.fingerprint, metadata.fingerprint, 32U) != 0);
}

void testLifecycle() {
    Fixture f;
    TaskAuthorityObservation observation; f.cache.observe(observation);
    CHECK(observation.state == TaskAuthorityState::Pending && !observation.binding.valid());
    CHECK(f.cache.restore(f.engine).code() == ErrorCode::NotFound);
    f.initialize();
    const unsigned reads = f.key.loads, loads = f.flash.loads;
    for (unsigned i = 0U; i < 50U; ++i) {
        TaskPollReport report; CHECK(f.engine.poll(report));
        CHECK(f.cache.restore(f.engine));
    }
    CHECK(f.key.loads == reads && f.flash.loads == loads);
    // A reboot can restore scope without a network grant, but only after a
    // fresh independent root load plus validated persisted table load.
    TaskAuthorityCache rebootCache;
    Environment env(rebootCache, f.clock);
    uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
    TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
    TaskEngine engine(store, env, f.target, records, slots, 2U, policy());
    CHECK(engine.begin().code() == ErrorCode::WouldBlock);
    CHECK(!rebootCache.restore(engine));
    CHECK(rebootCache.refresh(f.instance, f.sources)); CHECK(rebootCache.restore(engine));
    TaskPollReport report; CHECK(engine.poll(report)); CHECK(engine.state() == TaskEngineState::Ready);
    ++f.key.version; CHECK(rebootCache.refresh(f.instance, f.sources));
    CHECK(rebootCache.restore(engine).code() == ErrorCode::AuthenticationRequired);
    CHECK(!engine.poll(report)); CHECK(engine.state() == TaskEngineState::AwaitingAuthority);
    rebootCache.retire(); CHECK(!rebootCache.refresh(f.instance, f.sources));
    CHECK(!engine.poll(report)); CHECK(engine.state() == TaskEngineState::Retired);
    f.key.hook = [&]() { f.cache.invalidate(); };
    CHECK(!f.cache.refresh(f.instance, f.sources));
    TaskAuthorityBinding root; CHECK(!f.cache.root(root));
    f.key.hook = nullptr; f.key.fail = true; CHECK(!f.cache.refresh(f.instance, f.sources));
    CHECK(!f.cache.root(root));
}

void testManualKeyContinuity() {
    Fixture f;
    ConfiguredDeviceKeySource key;
    const auto configure = [&](uint8_t byte) {
        DeviceKey raw; memset(raw.bytes, byte, sizeof(raw.bytes));
        char text[44]; StringView encoded;
        CHECK(encodeDeviceKey(raw, MutableCharSpan(text, sizeof(text)), encoded));
        CHECK(key.configure(encoded)); clearDeviceKey(raw);
    };
    configure(1U); f.sources.deviceKey = &key; CHECK(f.cache.refresh(f.instance, f.sources));
    f.initialize();
    const Bytes durable = f.flash.bytes;
    // Same Sketch Key and instance on a fresh runtime: restore only AFTER real
    // root loading. No network/session credentials are used as the task root.
    TaskAuthorityCache cache; Environment environment(cache, f.clock);
    uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
    TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
    TaskEngine engine(store, environment, f.target, records, slots, 2U, policy());
    TaskManagementSession session(engine, cache, f.random, f.verifier, f.clock, 1000U, 10U);
    TaskCommandAdapter adapter(session, cache, engine, f.instance, f.sources);
    CHECK(engine.begin().code() == ErrorCode::WouldBlock); CHECK(adapter.start());
    TaskPollReport report; CHECK(engine.poll(report)); CHECK(engine.state() == TaskEngineState::Ready);
    adapter.stop(); CHECK(adapter.start()); CHECK(engine.poll(report));
    CHECK(f.flash.bytes == durable); // Stop/reconnect is not retirement or erase.
    adapter.invalidateRoots(); configure(2U); // Trusted maintenance boundary only.
    CHECK(adapter.refreshRoots().code() == ErrorCode::AuthenticationRequired);
    CHECK(!engine.poll(report) && engine.state() == TaskEngineState::AwaitingAuthority);
    CHECK(f.flash.bytes == durable); // Requires signed same-owner Rebind, never auto clear/init.
    adapter.invalidateRoots(); key.clear(); CHECK(!adapter.refreshRoots()); CHECK(!engine.poll(report));
    CHECK(f.flash.bytes == durable);
    adapter.retire(); CHECK(!engine.poll(report) && engine.state() == TaskEngineState::Retired);
}

void testOperations() {
    Fixture f; f.initialize();
    TaskManagementChallenge challenge; TaskManagementResult out;
    TaskMutation request = f.create();
    CHECK(f.session.prepareMutation(channel, request, challenge));
    request.definition.value[0] = 0xF4U; // Pending owns all bytes, no borrowed RX storage.
    TaskManagementChallenge rejected;
    CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, 0U, rejected).code() == ErrorCode::WouldBlock);
    CHECK(!rejected.authority.valid());
    const Bytes grant = f.verifier.sign(challenge);
    CHECK(f.session.commit(channel + 1U, ByteView(grant.data(), grant.size()), out).code() == ErrorCode::AuthenticationRequired);
    CHECK(f.session.pending() && f.verifier.calls == 1U); // Only initialize verified so far.
    CHECK(f.session.commit(channel, ByteView(grant.data(), grant.size()), out));
    CHECK(out.tableRevision == 1U && out.count == 1U && out.mutation.taskId == 1U);
    CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), out));
    CHECK(f.flash.writes == 2U && f.target.applied == 0U);
    f.clock.now += 10U;
    CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 1U, 0U, challenge));
    CHECK(f.commit(challenge, out)); CHECK(out.hasRecord && out.record.value[0] == 0xF5U);
    f.clock.now += 10U;
    CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, 0U, challenge));
    CHECK(f.commit(challenge, out).code() == ErrorCode::SequenceConflict);
    CHECK(!out.hasRecord && out.count == 0U && out.record.keyLength == 0U);
    f.clock.now += 10U;
    CHECK(!f.session.prepareRead(channel, ByteView(f.requestId, 16U), 1U, UINT32_MAX, challenge));
    CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, challenge));
    CHECK(f.commit(challenge, out)); CHECK(!out.hasRecord && out.count == 1U && out.tableRevision == 1U);
    // Repeated exact mutation uses the Engine receipt, not another write.
    f.clock.now += 10U; request = f.create();
    CHECK(f.session.prepareMutation(channel, request, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.mutation.replayed && f.flash.writes == 2U);
}

void testWindows() {
    for (unsigned mode = 0U; mode < 8U; ++mode) {
        Fixture f; TaskAuthorityBinding root; TaskManagementChallenge challenge; TaskManagementResult out;
        CHECK(f.session.prepareAuthority(channel, f.change(root), challenge));
        const Bytes grant = f.verifier.sign(challenge);
        if (mode == 0U) f.clock.now += 1000U;
        if (mode == 1U) f.session.disconnect(channel);
        if (mode == 2U) f.cache.invalidate();
        if (mode == 3U) f.verifier.fail = true;
        if (mode == 4U) f.verifier.hook = [&]() { f.cache.invalidate(); CHECK(f.cache.refresh(f.instance, f.sources)); };
        if (mode == 5U) f.verifier.hook = [&]() { f.clock.now += 1000U; };
        if (mode == 6U) f.verifier.hook = [&]() { f.session.disconnect(channel); };
        if (mode == 7U) f.clock.now -= 1U;
        CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), out));
        CHECK(!f.session.pending() && f.flash.writes == 0U && f.engine.state() == TaskEngineState::Empty);
        CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), out));
        CHECK(f.verifier.calls <= 1U);
    }
    Fixture f; TaskAuthorityBinding root; TaskManagementChallenge challenge; TaskManagementResult out;
    f.random.zero = true;
    CHECK(!f.session.prepareAuthority(channel, f.change(root), challenge));
    CHECK(!f.session.pending() && !challenge.authority.valid());
    f.random.zero = false; f.random.fail = true; f.clock.now += 10U;
    CHECK(f.session.prepareAuthority(channel, f.change(root), challenge).code() == ErrorCode::NotConfigured);
    f.random.fail = false; f.clock.now += 10U;
    f.random.hook = [&]() { f.session.cancel(); };
    CHECK(!f.session.prepareAuthority(channel, f.change(root), challenge));
    f.random.hook = nullptr; f.clock.now += 10U;
    CHECK(f.session.prepareAuthority(channel, f.change(root), challenge));
    const Bytes grant = f.verifier.sign(challenge);
    f.session.disconnect(channel + 1U); CHECK(f.session.pending());
    CHECK(!f.session.commit(channel, ByteView(), out)); // Malformed also consumes.
    CHECK(!f.session.pending()); CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), out));
    CHECK(f.session.prepareAuthority(channel, f.change(root), challenge).code() == ErrorCode::WouldBlock);
    f.clock.now += 10U;
    CHECK(f.session.prepareAuthority(channel, f.change(root), challenge));
    f.verifier.hook = [&]() {
        TaskManagementResult nested;
        CHECK(f.session.commit(channel, ByteView(), nested).code() == ErrorCode::WouldBlock);
    };
    CHECK(f.commit(challenge, out));
    // uint32 wrap with a short valid elapsed period is not a timeout/backstep.
    f.clock.now = UINT32_MAX - 20U;
    TaskManagementSession wrapping(f.engine, f.cache, f.random, f.verifier, f.clock, 1000U, 10U);
    CHECK(wrapping.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, challenge));
    const Bytes wrapGrant = f.verifier.sign(challenge); f.verifier.hook = nullptr;
    f.clock.now += 40U;
    CHECK(wrapping.commit(channel, ByteView(wrapGrant.data(), wrapGrant.size()), out));
}

void testRebindAndDurability() {
    Fixture f; f.initialize();
    TaskAuthorityBinding previous; CHECK(f.engine.authorityBinding(previous));
    ++f.key.version; CHECK(f.cache.refresh(f.instance, f.sources));
    CHECK(!f.cache.restore(f.engine));
    TaskAuthorityBinding root; TaskManagementChallenge challenge; TaskManagementResult out;
    TaskAuthorityChange change = f.change(root, ByteView(previous.rootDigest, 32U));
    uint8_t wrongScope[16] = {99U}; change.taskScope = ByteView(wrongScope, 16U);
    CHECK(!f.session.prepareAuthority(channel, change, challenge));
    change.taskScope = ByteView(f.scope, 16U);
    CHECK(f.session.prepareAuthority(channel, change, challenge));
    CHECK(f.commit(challenge, out)); CHECK(f.flash.writes == 2U && out.tableRevision == 0U);
    TaskAuthorityBinding stored; CHECK(f.engine.authorityBinding(stored));
    CHECK(constantTimeEqual(ByteView(stored.rootDigest, 32U), ByteView(root.rootDigest, 32U)));
    f.clock.now += 10U;
    CHECK(f.session.prepareMutation(channel, f.create(), challenge));
    f.flash.hook = [&]() { f.cache.invalidate(); };
    CHECK(!f.commit(challenge, out)); CHECK(f.flash.writes == 3U && !out.mutation.replayed);
    CHECK(f.engine.state() == TaskEngineState::AwaitingAuthority);
    f.flash.hook = nullptr; CHECK(f.cache.refresh(f.instance, f.sources)); CHECK(f.cache.restore(f.engine));
    TaskPollReport report; CHECK(f.engine.poll(report));
    f.clock.now += 10U;
    CHECK(f.session.prepareMutation(channel, f.create(), challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.mutation.replayed && f.flash.writes == 3U);
    // Failure after writing remains faulted; no read on the dirty store, no
    // pending retry, no Current scope left behind for a failed initialization.
    Fixture broken; broken.flash.fail = true;
    CHECK(broken.session.prepareAuthority(channel, broken.change(root), challenge));
    CHECK(!broken.commit(challenge, out)); CHECK(broken.engine.state() == TaskEngineState::Fault);
    TaskAuthorityObservation observed; broken.cache.observe(observed);
    CHECK(observed.state == TaskAuthorityState::Pending && broken.flash.loads == 1U);
}

void testRetiredScopeCannotResume() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        Fixture f; f.initialize();
        TaskAuthorityBinding stored; CHECK(f.engine.authorityBinding(stored));
        TaskManagementChallenge challenge;
        if (mode == 0U)
            CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, challenge));
        if (mode == 1U) CHECK(f.session.prepareMutation(channel, f.create(), challenge));
        if (mode == 2U) {
            ++f.key.version; CHECK(f.cache.refresh(f.instance, f.sources));
            TaskAuthorityBinding current;
            CHECK(f.session.prepareAuthority(channel, f.change(current, ByteView(stored.rootDigest, 32U)), challenge));
        }
        const Bytes grant = f.verifier.sign(challenge);
        const uint8_t retirementId[16] = {81U};
        // Trusted core seam simulates termination by another lifecycle owner
        // while an ordinary management challenge is outstanding.
        CHECK(f.engine.retireVerified(stored, ByteView(retirementId, 16U)));
        const Bytes terminal = f.flash.bytes;
        const unsigned writes = f.flash.writes;
        TaskManagementResult result; TaskManagementChallenge completed;
        CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), result, &completed));
        CHECK(!completed.authority.valid() && !f.session.pending());
        CHECK(f.flash.writes == writes && f.flash.bytes == terminal);
        CHECK(f.engine.state() == TaskEngineState::Retired && f.target.applied == 0U);

        // A fresh runtime and a valid old/new Key cannot turn a marker into an
        // empty table, restore its scope, rebind it or accept ordinary commands.
        TaskAuthorityCache cache; Environment environment(cache, f.clock);
        uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
        TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
        TaskEngine engine(store, environment, f.target, records, slots, 2U, policy());
        TaskManagementSession session(engine, cache, f.random, f.verifier, f.clock, 1000U, 10U);
        CHECK(cache.refresh(f.instance, f.sources));
        CHECK(engine.begin().code() == ErrorCode::AuthenticationRequired);
        CHECK(!cache.restore(engine));
        TaskAuthorityObservation observation; cache.observe(observation);
        CHECK(observation.state == TaskAuthorityState::Pending);
        f.clock.now += 10U;
        TaskAuthorityBinding current;
        CHECK(session.prepareAuthority(channel, f.change(current), challenge).code() == ErrorCode::StateConflict);
        // Supply a genuinely different, independently loaded root so Rebind
        // reaches the retired-state guard, not same-root input validation.
        ++f.key.version;
        CHECK(f.cache.refresh(f.instance, f.sources) && cache.refresh(f.instance, f.sources));
        CHECK(session.prepareAuthority(channel, f.change(current, ByteView(stored.rootDigest, 32U)), challenge).code()
            == ErrorCode::StateConflict);
        CHECK(session.prepareMutation(channel, f.create(), challenge).code() == ErrorCode::AuthenticationRequired);
        CHECK(session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, challenge).code()
            == ErrorCode::AuthenticationRequired);
        TaskSnapshotHeader marker; CHECK(engine.retirementRecord(marker));
        CHECK(marker.retired && marker.authority == stored);
        CHECK(f.flash.writes == writes && f.flash.bytes == terminal && f.target.applied == 0U);
    }
}

void testDigestCoverage() {
    Fixture f;
    TaskAuthorityBinding root; CHECK(f.cache.root(root));
    TaskMutation request = f.create(); uint8_t original[32], changed[32];
    CHECK(taskMutationPrepareDigest(request, ByteView(root.rootDigest, 32U), MutableByteSpan(original, 32U)));
    for (unsigned field = 0U; field < 7U; ++field) {
        TaskMutation other = request; TaskAuthorityBinding otherRoot = root;
        if (field == 0U) ++other.requestId[15];
        if (field == 1U) ++other.expectedRevision;
        if (field == 2U) other.definition.value[0] = 0xF4U;
        if (field == 3U) ++other.definition.timing.durationMillis;
        if (field == 4U) other.definition.enabled = false;
        if (field == 5U) other.definition.key[0] = 'q';
        if (field == 6U) ++otherRoot.rootDigest[31];
        CHECK(taskMutationPrepareDigest(other, ByteView(otherRoot.rootDigest, 32U), MutableByteSpan(changed, 32U)));
        CHECK(memcmp(original, changed, 32U) != 0);
    }
    request.definition.progress = TaskProgress::Completed;
    CHECK(!taskMutationPrepareDigest(request, ByteView(root.rootDigest, 32U), MutableByteSpan(changed, 32U)));
    for (uint8_t byte : changed) CHECK(byte == 0U);
}

void testStartAndLostReceipt() {
    Fixture f; f.initialize();
    TaskManagementChallenge challenge; TaskManagementResult out;
    CHECK(f.session.prepareMutation(channel, f.create(), challenge)); CHECK(f.commit(challenge, out));
    f.clock.now += 100U;
    TaskMutation start;
    memcpy(start.requestId, f.requestId, 16U); start.requestId[0] += 2U;
    start.kind = TaskMutationKind::Start; start.taskId = 1U; start.expectedRevision = 1U;
    CHECK(f.session.prepareMutation(channel, start, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.tableRevision == 2U && f.flash.writes == 3U);
    // Simulate lost response. Same request/new challenge, no second Start.
    f.clock.now += 10U;
    CHECK(f.session.prepareMutation(channel, start, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.mutation.replayed && f.flash.writes == 3U);
    f.session.disconnect(channel); // Leaving page never cancels installed tasks.
    f.clock.now += 990U;
    TaskPollReport report; CHECK(f.engine.poll(report));
    CHECK(report.callbackInvoked && f.target.applied == 1U && f.flash.writes == 5U);
    CHECK(f.session.prepareRead(channel, ByteView(f.requestId, 16U), 2U, 0U, challenge));
    CHECK(f.commit(challenge, out));
    CHECK(out.record.progress == TaskProgress::Completed && out.record.startSequence == 1U);
    CHECK(f.engine.poll(report) && f.target.applied == 1U);
    // TTL ends after a successful, within-budget durable change: hide response,
    // never undo/auto-repeat; next exact retry is answered by Engine replay.
    f.clock.now += 10U;
    TaskMutation disable;
    memcpy(disable.requestId, f.requestId, 16U); disable.requestId[0] += 3U;
    disable.kind = TaskMutationKind::SetEnabled; disable.taskId = 1U; disable.expectedRevision = 2U;
    CHECK(f.session.prepareMutation(channel, disable, challenge));
    f.verifier.hook = [&]() { f.clock.now += 990U; };
    f.flash.hook = [&]() { f.clock.now += 20U; };
    CHECK(!f.commit(challenge, out)); CHECK(!out.hasRecord && out.tableRevision == 0U);
    CHECK(f.engine.tableRevision() == 3U && f.flash.writes == 6U);
    f.verifier.hook = nullptr; f.flash.hook = nullptr;
    CHECK(f.session.prepareMutation(channel, disable, challenge)); CHECK(f.commit(challenge, out));
    CHECK(out.mutation.replayed && f.flash.writes == 6U);
}

void testEncodedManagementFlow() {
    Fixture f; f.initialize();
    uint8_t wire[256]; ByteView encoded;
    CHECK(encodeTaskMutationPrepare(f.create(), MutableByteSpan(wire, sizeof(wire)), encoded));
    TaskMutation received;
    CHECK(decodeTaskMutationPrepare(encoded, received));
    TaskManagementChallenge deviceChallenge, clientChallenge, completed;
    CHECK(f.session.prepareMutation(channel, received, deviceChallenge));
    CHECK(encodeTaskManagementChallenge(deviceChallenge, MutableByteSpan(wire, sizeof(wire)), encoded));
    CHECK(decodeTaskManagementChallenge(encoded, clientChallenge));
    const Bytes grant = f.verifier.sign(clientChallenge);
    TaskManagementResult result;
    CHECK(f.session.commit(channel, ByteView(grant.data(), grant.size()), result, &completed));
    CHECK(result.kind == TaskManagementResultKind::Mutation && f.flash.writes == 2U);
    uint8_t secret[32]; memset(secret, f.key.version, sizeof(secret));
    CHECK(!encodeTaskManagementResult(completed, result, TaskResultKey::DeviceKey, ByteView(secret, 32U),
        MutableByteSpan(wire, 1U), encoded));
    CHECK(encoded.empty() && f.engine.tableRevision() == 1U); // Small TX buffer doesn't undo the commit.
    CHECK(encodeTaskManagementResult(completed, result, TaskResultKey::DeviceKey, ByteView(secret, 32U),
        MutableByteSpan(wire, sizeof(wire)), encoded));
    TaskManagementResult confirmed;
    CHECK(verifyTaskManagementResult(encoded, clientChallenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), confirmed));
    CHECK(confirmed.mutation.taskId == 1U && confirmed.tableRevision == 1U);
    // A late duplicate cannot leak the previous successful completed context.
    CHECK(!f.session.commit(channel, ByteView(grant.data(), grant.size()), result, &completed));
    CHECK(!completed.authority.valid() && result.tableRevision == 0U);
}
// Real codec + adapter + Session + Engine. Only storage/random/signature
// boundary doubles; no network identity or deployed service is simulated.
struct WireFixture : Fixture {
    TaskCommandAdapter adapter;
    TaskCommandSource source;
    uint8_t wire[kTaskManagementResultMaxSize] = {};
    ByteView response;
    WireFixture() : adapter(session, cache, engine, instance, sources) {
        source.generation = 1U; source.session = 7U; source.transport = 2U;
        source.routed = true; source.peerKind = bbp2::RoutePeerKind::Account; source.peerId[0] = 19U;
        CHECK(adapter.start());
    }
    Result call(TaskCommand command, ByteView body) {
        return adapter.receive(source, command, body, MutableByteSpan(wire, sizeof(wire)), response);
    }
    TaskManagementChallenge prepare(TaskCommand command, ByteView body) {
        clock.now += 10U;
        CHECK(call(command, body));
        TaskManagementChallenge context; CHECK(decodeTaskManagementChallenge(response, context));
        return context;
    }
    TaskManagementChallenge prepareRead() {
        uint8_t bytes[32]; ByteView encoded; TaskReadPrepare read;
        read.requestId[0] = 31U;
        CHECK(encodeTaskReadPrepare(read, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        return prepare(TaskCommand::Read, encoded);
    }
    Result finish(const TaskManagementChallenge& context) {
        const Bytes grant = verifier.sign(context);
        return call(TaskCommand::Commit, ByteView(grant.data(), grant.size()));
    }
    void initializeWire() {
        uint8_t bytes[kTaskAuthorityChangeMaxSize]; ByteView encoded; TaskAuthorityBinding root;
        CHECK(encodeTaskAuthorityChange(change(root), MutableByteSpan(bytes, sizeof(bytes)), encoded));
        const auto context = prepare(TaskCommand::Authority, encoded);
        CHECK(finish(context)); CHECK(engine.state() == TaskEngineState::Ready);
        DeviceKey secret; CHECK(key.load(secret)); TaskManagementResult verified;
        CHECK(verifyTaskManagementResult(response, context, TaskResultKey::DeviceKey,
            ByteView(secret.bytes, sizeof(secret.bytes)), verified));
        CHECK(verified.kind == TaskManagementResultKind::Authority);
    }
};

void testRetirementSession() {
    {
        WireFixture f; f.initializeWire();
        TaskRetirementPrepare request; CHECK(f.engine.authorityBinding(request.authority)); request.requestId[0] = 81U;
        uint8_t bytes[kTaskRetirementPrepareMaxSize]; ByteView encoded;
        for (unsigned field = 0U; field < 3U; ++field) {
            auto wrong = request;
            if (field == 0U) wrong.authority.deviceInstanceId[0] ^= 1U;
            if (field == 1U) wrong.authority.taskScope[0] ^= 1U;
            if (field == 2U) wrong.authority.rootDigest[0] ^= 1U;
            CHECK(encodeTaskRetirementPrepare(wrong, MutableByteSpan(bytes, sizeof(bytes)), encoded));
            f.clock.now += 10U;
            CHECK(!f.call(TaskCommand::Retire, encoded) && !f.session.pending());
        }
        CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        auto challenge = f.prepare(TaskCommand::Retire, encoded);
        // All identity/nonce/request fields match, but a Query signature must
        // never authorize the write operation against this still-active table.
        CHECK(taskRetirementPrepareDigest(request, true, MutableByteSpan(challenge.operationDigest, 32U)));
        const Bytes queryGrant = f.verifier.sign(challenge);
        CHECK(!f.call(TaskCommand::Commit, ByteView(queryGrant.data(), queryGrant.size())));
        CHECK(f.engine.state() != TaskEngineState::Retired && f.flash.writes == 1U);
    }
    for (unsigned event = 0U; event < 6U; ++event) {
        WireFixture f; f.initializeWire();
        TaskRetirementPrepare request; CHECK(f.engine.authorityBinding(request.authority)); request.requestId[0] = 81U;
        uint8_t bytes[87]; ByteView encoded;
        CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        f.clock.now += 10U;
        CHECK(!f.call(TaskCommand::RetirementRead, encoded)); // No marker is NOT authenticated proof of absence.
        auto challenge = f.prepare(TaskCommand::Retire, encoded);
        const Bytes grant = f.verifier.sign(challenge);
        if (event == 0U) f.clock.now += 1000U;
        if (event == 1U) f.adapter.disconnect(f.source);
        if (event == 2U) f.cache.invalidate();
        if (event == 3U) f.verifier.fail = true;
        if (event == 4U) f.verifier.hook = [&]() { f.cache.invalidate(); };
        if (event == 5U) f.flash.fail = true; // Commit happened, but provider returned failure.
        CHECK(!f.call(TaskCommand::Commit, ByteView(grant.data(), grant.size())) && f.response.empty());
        CHECK(f.flash.writes == (event == 5U ? 2U : 1U) && !f.session.pending());
        if (event != 5U) CHECK(f.engine.state() != TaskEngineState::Retired);
        else {
            CHECK(f.engine.state() == TaskEngineState::Retired);
            f.clock.now += 10U;
            CHECK(!f.call(TaskCommand::RetirementRead, encoded)); // No dirty-handle read-back proof.
            CHECK(f.flash.writes == 2U);
        }
    }
    WireFixture f; f.initializeWire();
    TaskRetirementPrepare request; CHECK(f.engine.authorityBinding(request.authority)); request.requestId[0] = 81U;
    uint8_t bytes[87]; ByteView encoded;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    auto challenge = f.prepare(TaskCommand::Retire, encoded);
    CHECK(f.finish(challenge) && f.engine.state() == TaskEngineState::Retired && f.flash.writes == 2U);
    uint8_t secret[32]; memset(secret, f.key.version, 32U);
    TaskManagementResult result;
    CHECK(verifyTaskManagementResult(f.response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    CHECK(result.kind == TaskManagementResultKind::Retired && result.retirementId[0] == 81U && result.count == 0U);
    challenge = f.prepare(TaskCommand::Retire, encoded); CHECK(f.finish(challenge) && f.flash.writes == 2U);
    // Query has a separate request ID, reports the ORIGINAL retirement ID and
    // must remain read-only, including after reboot and a lost result.
    request.requestId[0] = 82U;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.clock.now += 10U; CHECK(!f.call(TaskCommand::Retire, encoded)); // Cannot replace terminal intent.
    challenge = f.prepare(TaskCommand::RetirementRead, encoded); CHECK(f.finish(challenge));
    CHECK(verifyTaskManagementResult(f.response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    CHECK(result.retirementId[0] == 81U && f.flash.writes == 2U);
    // A query ticket cannot be used to retire an active table: separate digest.
    uint8_t retireDigest[32]; CHECK(taskRetirementPrepareDigest(request, false, MutableByteSpan(retireDigest, 32U)));
    CHECK(!constantTimeEqual(ByteView(retireDigest, 32U), ByteView(challenge.operationDigest, 32U)));

    TaskAuthorityCache cache; Environment environment(cache, f.clock);
    uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
    TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
    TaskEngine engine(store, environment, f.target, records, slots, 2U, policy());
    TaskManagementSession session(engine, cache, f.random, f.verifier, f.clock, 1000U, 10U);
    TaskCommandAdapter adapter(session, cache, engine, f.instance, f.sources);
    CHECK(engine.begin().code() == ErrorCode::AuthenticationRequired);
    CHECK(!adapter.start()); // Executable scope cannot restore; root is independently available for query.
    ByteView response; uint8_t output[197];
    const auto receive = [&](TaskCommand command, ByteView body) {
        return adapter.receive(f.source, command, body, MutableByteSpan(output, sizeof(output)), response);
    };
    CHECK(receive(TaskCommand::RetirementRead, encoded)); CHECK(decodeTaskManagementChallenge(response, challenge));
    const Bytes grant = f.verifier.sign(challenge);
    CHECK(receive(TaskCommand::Commit, ByteView(grant.data(), grant.size())));
    CHECK(verifyTaskManagementResult(response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    CHECK(result.retirementId[0] == 81U && f.flash.writes == 2U && f.target.applied == 0U);
    ++f.key.version; CHECK(!adapter.refreshRoots());
    CHECK(!receive(TaskCommand::RetirementRead, encoded)); // Old-root request remains invalid under a new Key.
}

void testCrossRootRetirement() {
    WireFixture f; f.initializeWire();
    uint8_t bytes[kTaskManagementResultMaxSize]; ByteView encoded;
    CHECK(encodeTaskMutationPrepare(f.create(), MutableByteSpan(bytes, sizeof(bytes)), encoded));
    CHECK(f.finish(f.prepare(TaskCommand::Mutation, encoded)));
    TaskAuthorityBinding first; CHECK(f.engine.authorityBinding(first));
    // Device installs B, but the Service may lose that receipt. Current Key C
    // must terminate the same scope without pretending A->C/B->C was installed.
    ++f.key.version; CHECK(!f.adapter.refreshRoots());
    TaskAuthorityBinding root;
    CHECK(encodeTaskAuthorityChange(f.change(root, ByteView(first.rootDigest, 32U)),
        MutableByteSpan(bytes, sizeof(bytes)), encoded));
    CHECK(f.finish(f.prepare(TaskCommand::Authority, encoded)));
    TaskAuthorityBinding stored; CHECK(f.engine.authorityBinding(stored)); CHECK(stored != first);
    ++f.key.version; CHECK(!f.adapter.refreshRoots());
    TaskPollReport report; CHECK(!f.engine.poll(report));
    CHECK(f.engine.state() == TaskEngineState::AwaitingAuthority);
    const unsigned writes = f.flash.writes;
    TaskRetirementPrepare request; CHECK(f.cache.root(request.authority));
    memcpy(request.authority.taskScope, stored.taskScope, 16U); request.requestId[0] = 81U;
    CHECK(request.authority != stored && request.authority.sameOwner(stored));
    for (unsigned field = 0U; field < 3U; ++field) {
        auto wrong = request;
        if (field == 0U) wrong.authority.deviceInstanceId[0] ^= 1U;
        if (field == 1U) wrong.authority.taskScope[0] ^= 1U;
        if (field == 2U) memcpy(wrong.authority.rootDigest, stored.rootDigest, 32U);
        CHECK(encodeTaskRetirementPrepare(wrong, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        f.clock.now += 10U;
        CHECK(!f.call(TaskCommand::Retire, encoded) && !f.session.pending());
    }
    TaskManagementChallenge denied;
    CHECK(!f.session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, denied));
    CHECK(!f.session.prepareMutation(channel, f.create(), denied));
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.clock.now += 10U; CHECK(!f.call(TaskCommand::RetirementRead, encoded)); // Still no terminal marker.
    auto challenge = f.prepare(TaskCommand::Retire, encoded);
    CHECK(challenge.authority == request.authority && f.flash.writes == writes);
    CHECK(f.finish(challenge));
    uint8_t secret[32]; memset(secret, f.key.version, 32U); TaskManagementResult result;
    CHECK(verifyTaskManagementResult(f.response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    CHECK(result.kind == TaskManagementResultKind::Retired && result.count == 0U && result.retirementId[0] == 81U);
    memset(secret, f.key.version - 1U, 32U);
    CHECK(!verifyTaskManagementResult(f.response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    TaskSnapshotHeader marker; CHECK(f.engine.retirementRecord(marker));
    CHECK(marker.authority == stored && f.flash.writes == writes + 1U && f.target.applied == 0U);
    const Bytes terminal = f.flash.bytes;
    challenge = f.prepare(TaskCommand::Retire, encoded); CHECK(f.finish(challenge));
    CHECK(f.flash.writes == writes + 1U && f.flash.bytes == terminal); // Same original retirement, no rewrite.

    // Fresh runtime/Key D: stored terminal binding stays B. Only a fresh D
    // Grant may query it; a stale request or Grant cannot reveal a receipt.
    TaskAuthorityCache cache; Environment environment(cache, f.clock);
    uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
    TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
    TaskEngine engine(store, environment, f.target, records, slots, 2U, policy());
    TaskManagementSession session(engine, cache, f.random, f.verifier, f.clock, 1000U, 10U);
    TaskCommandAdapter adapter(session, cache, engine, f.instance, f.sources);
    ++f.key.version; CHECK(engine.begin().code() == ErrorCode::AuthenticationRequired); CHECK(!adapter.start());
    ByteView response; uint8_t output[kTaskManagementResultMaxSize];
    const auto receive = [&](TaskCommand command, ByteView body) {
        return adapter.receive(f.source, command, body, MutableByteSpan(output, sizeof(output)), response);
    };
    CHECK(!receive(TaskCommand::RetirementRead, encoded)); // C request is no longer current.
    CHECK(cache.root(request.authority)); memcpy(request.authority.taskScope, stored.taskScope, 16U);
    request.requestId[0] = 82U;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    CHECK(receive(TaskCommand::RetirementRead, encoded)); CHECK(decodeTaskManagementChallenge(response, challenge));
    const Bytes grant = f.verifier.sign(challenge);
    CHECK(receive(TaskCommand::Commit, ByteView(grant.data(), grant.size())));
    memset(secret, f.key.version, 32U);
    CHECK(verifyTaskManagementResult(response, challenge, TaskResultKey::DeviceKey, ByteView(secret, 32U), result));
    CHECK(result.retirementId[0] == 81U && result.count == 0U);
    CHECK(f.flash.writes == writes + 1U && f.flash.bytes == terminal && f.target.applied == 0U);
    CHECK(!session.prepareMutation(channel, f.create(), denied));
    CHECK(!session.prepareRead(channel, ByteView(f.requestId, 16U), 0U, UINT32_MAX, denied));
}

void testCrossRootRetirementFailures() {
    for (unsigned event = 0U; event < 8U; ++event) {
        WireFixture f; f.initializeWire();
        TaskAuthorityBinding stored; CHECK(f.engine.authorityBinding(stored));
        ++f.key.version; CHECK(!f.adapter.refreshRoots());
        TaskRetirementPrepare request; CHECK(f.cache.root(request.authority));
        memcpy(request.authority.taskScope, stored.taskScope, 16U); request.requestId[0] = 81U;
        uint8_t bytes[kTaskRetirementPrepareMaxSize]; ByteView encoded;
        CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        const auto challenge = f.prepare(TaskCommand::Retire, encoded);
        const Bytes grant = f.verifier.sign(challenge);
        if (event == 0U) f.clock.now += 1000U;
        if (event == 1U) f.adapter.disconnect(f.source);
        if (event == 2U) { ++f.key.version; CHECK(!f.adapter.refreshRoots()); }
        if (event == 3U) f.verifier.fail = true;
        if (event == 4U) f.verifier.hook = [&]() { ++f.key.version; CHECK(f.cache.refresh(f.instance, f.sources)); };
        if (event == 5U) f.flash.fail = true; // Provider commits before returning failure.
        if (event == 6U) f.flash.hook = [&]() { ++f.key.version; CHECK(f.cache.refresh(f.instance, f.sources)); };
        if (event == 7U) {
            // Even with unchanged root generation, an independently authorized
            // replacement of the stored binding must invalidate this old pin.
            TaskManagementSession other(f.engine, f.cache, f.random, f.verifier, f.clock, 1000U, 10U);
            TaskAuthorityBinding current; TaskManagementChallenge changed; TaskManagementResult result;
            CHECK(other.prepareAuthority(channel + 1U, f.change(current, ByteView(stored.rootDigest, 32U)), changed));
            const Bytes signedChange = f.verifier.sign(changed);
            CHECK(other.commit(channel + 1U, ByteView(signedChange.data(), signedChange.size()), result));
            CHECK(f.verifier.sign(challenge) == grant); // Restore boundary double for original Grant.
        }
        CHECK(!f.call(TaskCommand::Commit, ByteView(grant.data(), grant.size())) && f.response.empty());
        CHECK(!f.session.pending() && f.target.applied == 0U);
        CHECK(f.flash.writes == (event >= 5U ? 2U : 1U));
        if (event == 5U) {
            f.clock.now += 10U; CHECK(!f.call(TaskCommand::RetirementRead, encoded));
            // Only a genuinely fresh store handle may authenticate a marker
            // that persisted before the provider reported the failed write.
            TaskAuthorityCache cache; Environment environment(cache, f.clock);
            uint8_t workspace[sizeof(f.workspace)]; TaskSnapshotRecord records[2]; TaskRuntimeSlot slots[2];
            TaskSnapshotStore store(f.flash, MutableByteSpan(workspace, sizeof(workspace)));
            TaskEngine engine(store, environment, f.target, records, slots, 2U, policy());
            TaskManagementSession session(engine, cache, f.random, f.verifier, f.clock, 1000U, 10U);
            TaskCommandAdapter adapter(session, cache, engine, f.instance, f.sources);
            CHECK(engine.begin().code() == ErrorCode::AuthenticationRequired); CHECK(!adapter.start());
            uint8_t output[kTaskManagementResultMaxSize]; ByteView response;
            CHECK(adapter.receive(f.source, TaskCommand::RetirementRead, encoded, MutableByteSpan(output, sizeof(output)), response));
            TaskManagementChallenge fresh; CHECK(decodeTaskManagementChallenge(response, fresh));
            const Bytes signedQuery = f.verifier.sign(fresh);
            CHECK(adapter.receive(f.source, TaskCommand::Commit, ByteView(signedQuery.data(), signedQuery.size()),
                MutableByteSpan(output, sizeof(output)), response));
            uint8_t key[32]; memset(key, f.key.version, 32U); TaskManagementResult result;
            CHECK(verifyTaskManagementResult(response, fresh, TaskResultKey::DeviceKey, ByteView(key, 32U), result));
            CHECK(result.kind == TaskManagementResultKind::Retired && result.retirementId[0] == 81U);
            CHECK(f.flash.writes == 2U && f.target.applied == 0U);
        }
    }
}

void testCommandAdapter() {
    WireFixture f; f.initializeWire();
    uint8_t mutationBytes[kTaskMutationPrepareMaxSize]; ByteView mutationEncoded;
    CHECK(encodeTaskMutationPrepare(f.create(), MutableByteSpan(mutationBytes, sizeof(mutationBytes)), mutationEncoded));
    const auto install = f.prepare(TaskCommand::Mutation, mutationEncoded);
    memset(mutationBytes, 0, sizeof(mutationBytes)); // Pending owns its request.
    CHECK(f.finish(install) && f.engine.count() == 1U && f.engine.tableRevision() == 1U);
    uint8_t secret[32]; memset(secret, f.key.version, sizeof(secret));
    TaskManagementResult installed;
    CHECK(verifyTaskManagementResult(f.response, install, f.source.role, ByteView(secret, 32U), installed));
    CHECK(installed.kind == TaskManagementResultKind::Mutation && installed.mutation.taskId == 1U);
    auto challenge = f.prepareRead();
    const Bytes grant = f.verifier.sign(challenge);
    // Every part of the source binding matters. Wrong peers cannot consume it.
    for (unsigned i = 0U; i < 7U; ++i) {
        TaskCommandSource other = f.source;
        switch (i) {
        case 0U: ++other.generation; break;
        case 1U: ++other.session; break;
        case 2U: ++other.transport; break;
        case 3U: ++other.peerId[0]; break;
        case 4U: other.peerKind = bbp2::RoutePeerKind::CloudService; break;
        case 5U: other.role = TaskResultKey::Admin; break;
        default: other.routed = false; memset(other.peerId, 0, 16U); other.peerKind = bbp2::RoutePeerKind::LogicalDevice; break;
        }
        CHECK(!f.adapter.receive(other, TaskCommand::Commit, ByteView(grant.data(), grant.size()),
            MutableByteSpan(f.wire, sizeof(f.wire)), f.response));
        f.adapter.disconnect(other);
        CHECK(f.response.empty() && f.session.pending());
    }
    CHECK(f.finish(challenge));
    CHECK(!f.finish(challenge) && f.response.empty()); // Consumed, even same bytes.
    // Authorized single-page retrieval returns owned definition and progress.
    TaskReadPrepare page; page.requestId[0] = 32U; page.expectedRevision = 1U; page.index = 0U;
    uint8_t pageBytes[kTaskReadPrepareMaxSize]; ByteView pageEncoded;
    CHECK(encodeTaskReadPrepare(page, MutableByteSpan(pageBytes, sizeof(pageBytes)), pageEncoded));
    const auto pageChallenge = f.prepare(TaskCommand::Read, pageEncoded);
    CHECK(f.finish(pageChallenge));
    CHECK(verifyTaskManagementResult(f.response, pageChallenge, f.source.role, ByteView(secret, 32U), installed));
    CHECK(installed.hasRecord && installed.index == 0U && installed.record.taskId == 1U);
    challenge = f.prepareRead();
    f.adapter.disconnectTransport(f.source.transport, f.source.generation + 1U);
    CHECK(f.session.pending());
    f.adapter.disconnectTransport(f.source.transport, f.source.generation);
    CHECK(!f.finish(challenge));
    challenge = f.prepareRead();
    f.clock.now += 1000U; f.adapter.poll();
    CHECK(!f.finish(challenge));
    challenge = f.prepareRead();
    f.adapter.stop(); CHECK(!f.finish(challenge)); CHECK(f.adapter.start());
    CHECK(!f.finish(challenge)); CHECK(f.engine.state() == TaskEngineState::Ready);

    // Reject undersized TX before committing, without consuming the challenge.
    challenge = f.prepareRead(); const Bytes freshGrant = f.verifier.sign(challenge);
    const unsigned calls = f.verifier.calls, writes = f.flash.writes;
    CHECK(!f.adapter.receive(f.source, TaskCommand::Commit, ByteView(freshGrant.data(), freshGrant.size()),
        MutableByteSpan(f.wire, 1U), f.response));
    CHECK(f.response.empty() && f.session.pending() && f.verifier.calls == calls && f.flash.writes == writes);
    CHECK(f.finish(challenge));
    const uint8_t invalid[] = {0x80U};
    f.clock.now += 10U;
    CHECK(!f.call(TaskCommand::Mutation, ByteView(invalid, sizeof(invalid))) && !f.session.pending());
    challenge = f.prepareRead();
    CHECK(f.call(TaskCommand::Read, ByteView(invalid, sizeof(invalid))).code() == ErrorCode::WouldBlock);
    CHECK(f.finish(challenge));

    // Real result source reloads exactly once; the callback cannot enter again,
    // retire/restart/rotate a root then leak a stale successful receipt.
    for (unsigned mode = 0U; mode < 5U; ++mode) {
        WireFixture t; t.initializeWire(); const auto pending = t.prepareRead();
        const unsigned loads = t.key.loads;
        t.key.hook = [&]() {
            if (mode == 0U) t.adapter.disconnect(t.source);
            else if (mode == 1U) t.adapter.stop();
            else if (mode == 2U) t.adapter.invalidateRoots();
            else if (mode == 3U) t.adapter.retire();
            else {
                ByteView nested;
                CHECK(t.adapter.receive(t.source, TaskCommand::Commit, ByteView(), MutableByteSpan(t.wire, sizeof(t.wire)), nested).code() == ErrorCode::WouldBlock);
            }
        };
        CHECK(t.finish(pending).ok() == (mode == 4U));
        CHECK(t.key.loads == loads + 1U);
        if (mode != 4U) CHECK(t.response.empty());
    }
    for (bool writeFails : {false, true}) {
        WireFixture t; t.initializeWire();
        uint8_t bytes[kTaskMutationPrepareMaxSize]; ByteView encoded;
        CHECK(encodeTaskMutationPrepare(t.create(), MutableByteSpan(bytes, sizeof(bytes)), encoded));
        const auto pending = t.prepare(TaskCommand::Mutation, encoded);
        t.flash.fail = writeFails; t.key.fail = !writeFails;
        CHECK(!t.finish(pending) && t.response.empty());
        CHECK(t.flash.writes == 2U); // No reinitialize/retry to hide uncertain write.
        CHECK(!t.session.pending());
        if (!writeFails) CHECK(t.engine.tableRevision() == 1U); // Receipt failure is not rollback.
    }
    // Changed actual root cannot be used to authenticate the old context even
    // if an integration violated its required invalidate-before-write contract.
    WireFixture stale; stale.initializeWire(); challenge = stale.prepareRead();
    ++stale.key.version;
    CHECK(!stale.finish(challenge) && stale.response.empty());
    // Explicit root transition preserves the same owner table via fresh Grant.
    WireFixture rotated; rotated.initializeWire();
    TaskAuthorityBinding old; CHECK(rotated.engine.authorityBinding(old));
    rotated.adapter.invalidateRoots(); ++rotated.key.version;
    CHECK(!rotated.adapter.refreshRoots()); // Mismatch is Pending, not empty/retired.
    TaskAuthorityBinding current; uint8_t bytes[kTaskAuthorityChangeMaxSize]; ByteView encoded;
    CHECK(encodeTaskAuthorityChange(rotated.change(current, ByteView(old.rootDigest, 32U)), MutableByteSpan(bytes, sizeof(bytes)), encoded));
    challenge = rotated.prepare(TaskCommand::Authority, encoded);
    CHECK(rotated.finish(challenge)); CHECK(rotated.engine.state() == TaskEngineState::Ready);
    std::cout << "task adapter bytes=" << sizeof(TaskCommandAdapter) << '\n';
}

void testManagementProductLifecycle() {
    WireFixture f; f.initializeWire(); f.adapter.stop();
    struct Module {
        TaskEngine& value;
        Result begin() { return Result::success(); } // Fixture already loaded storage.
        TaskEngine& engine() { return value; }
    } module{f.engine};
    struct Transport : IProductLifecycle {
        unsigned resets = 0U;
        Result attach(Client&) override { return Result::success(); }
        Result start() override { return Result::success(); }
        void stop() override {}
        void poll(uint32_t) override {}
        Result resetAccess() override { ++resets; return Result::success(); }
        ProductLifecycleStatus status() const override { return ProductLifecycleStatus(); }
        ProductCapabilities capabilities() const override { return ProductCapabilities(ProductCapabilityAccessReset); }
    } transport;
    PropertySpec<bool> spec("power", readWrite()); Property<bool> power(spec);
    CHECK(power.onWrite([](bool) { CHECK(false); })); // Management must not apply it.
    DeviceSchema<Property<bool>> schema(power); Device device(schema);
    const TaskPropertyBinding bindings[] = {TaskPropertyBinding(power)};
    TaskPropertyTarget target(bindings, 1U);
    TaskProductLifecycle<Module> product(transport, module, target, &f.adapter);
    Product attached(device, product); CHECK(attached.begin());
    auto pending = f.prepareRead(); f.clock.now += 1000U; product.poll(1000U);
    CHECK(!f.finish(pending));
    pending = f.prepareRead(); product.stop(); CHECK(!f.finish(pending));
    CHECK(f.engine.count() == 0U && f.engine.state() == TaskEngineState::Ready);
    CHECK(product.start()); pending = f.prepareRead();
    CHECK(product.resetAccess()); CHECK(transport.resets == 1U && f.engine.state() == TaskEngineState::Cleared);
    TaskAuthorityObservation observed; f.cache.observe(observed);
    CHECK(observed.state == TaskAuthorityState::Retired && !f.finish(pending));
}
// The socket is a fake; MQTT framing, TransportHub, Runtime negotiation and
// TaskCloudTransport are the actual production classes, not manual callbacks.
struct TaskMqtt : IMqttClient {
    MqttMessageHandler handler = nullptr; void* context = nullptr;
    bool online = false, inside = false, block = false;
    unsigned connects = 0U; std::vector<Bytes> sent; std::vector<Bytes> incoming;
    std::function<void()> afterRx;
    Result connect(const MqttConnectOptions&) override { ++connects; online = true; return Result::success(); }
    void disconnect() override { online = false; }
    bool connected() const override { return online; }
    Result subscribe(StringView, uint8_t) override { return Result::success(); }
    Result publish(StringView, ByteView bytes, const MqttPublishOptions& options) override {
        if (bytes.data[3] == static_cast<uint8_t>(bbp2::MessageKind::TaskManagement)) CHECK(!inside);
        CHECK(!options.retained);
        if (block) return Result::failure(ErrorCode::WouldBlock);
        sent.emplace_back(bytes.data, bytes.data + bytes.size); return Result::success();
    }
    void poll(uint32_t) override {
        auto frames = incoming; incoming.clear();
        for (auto& bytes : frames) {
            inside = true; MqttMessageInfo info;
            handler(context, StringView("down"), ByteView(bytes.data(), bytes.size()), info);
            memset(bytes.data(), 0xEE, bytes.size()); // Invalidate borrowed native RX.
            inside = false;
        }
        if (afterRx) afterRx();
    }
    void setMessageHandler(MqttMessageHandler value, void* state) override { handler = value; context = state; }
};
struct CloudFixture : Fixture {
    static DeviceRuntimeConfig config() {
        DeviceRuntimeConfig c; c.maxFrameSize = 256U; c.maxReassemblySize = 256U;
        c.protocolFeatures = bbp2::FeatureNone; return c;
    }
    TaskMqtt socket;
    MqttFrameTransport mqtt;
    TaskCommandAdapter adapter;
    uint8_t wire[212] = {}, tx[256] = {};
    TaskCloudTransport<MqttFrameTransport> transport;
    TransportHub hub; EndpointDescriptor descriptor; EndpointRegistry endpoints; DeviceRuntime runtime;
    uint16_t sequence = 20U;
    CloudFixture() : mqtt(socket, clock), adapter(session, cache, engine, instance, sources),
        transport(mqtt, mqtt, clock, adapter, MutableByteSpan(wire, sizeof(wire))),
        endpoints(&descriptor, 1U), runtime(hub, endpoints, MutableByteSpan(tx, sizeof(tx)), config()) {
        descriptor.key = StringView("p"); descriptor.type = ValueType::Boolean;
        descriptor.kind = EndpointKind::Property; descriptor.access = AccessRead | AccessWrite | AccessNotify;
        SessionCredentials credentials; credentials.broker = BrokerKind::Blinker;
        credentials.host = StringView("host"); credentials.port = 1883U; credentials.security = MqttSecurity::PlainTcp;
        credentials.clientId = credentials.username = credentials.password = StringView("synthetic");
        credentials.publishTopic = StringView("up"); credentials.subscribeTopic = StringView("down");
        CHECK(mqtt.configure(credentials)); CHECK(adapter.start());
        CHECK(hub.addTransport(transport)); CHECK(runtime.start()); runtime.poll(1000U);
        flash.hook = [this]() { CHECK(!socket.inside); };
        key.hook = [this]() { CHECK(!socket.inside); };
        verifier.hook = [this]() { CHECK(!socket.inside); };
    }
    ~CloudFixture() { runtime.stop(); }
    Bytes frame(bbp2::MessageKind kind, ByteView body, uint8_t flags = 0U) {
        bbp2::FrameHeader header; header.kind = static_cast<uint8_t>(kind);
        header.flags = flags; header.sequence = ++sequence; header.bodyLength = static_cast<uint16_t>(body.size);
        Bytes bytes(10U + body.size); CHECK(bbp2::encodeHeader(header, MutableByteSpan(bytes.data(), bytes.size())));
        memcpy(bytes.data() + 10U, body.data, body.size); return bytes;
    }
    void hello(bool enabled = true, uint16_t maximum = 256U) {
        bbp2::HelloBody h; h.role = bbp2::PeerRole::Server; h.versions[0] = 2U; h.versionCount = 1U;
        h.maxFrameSize = maximum; h.maxReassemblySize = maximum;
        h.features = enabled ? static_cast<uint32_t>(bbp2::FeatureTaskManagement) : 0U;
        uint8_t bytes[128]; ByteView encoded;
        CHECK(bbp2::encodeHelloBody(h, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        socket.incoming.push_back(frame(bbp2::MessageKind::Hello, encoded)); runtime.poll(1000U);
    }
    void enqueue(TaskWireOperation operation, ByteView inner) {
        uint8_t bytes[kTaskCarrierMaxSize]; ByteView encoded;
        CHECK(encodeTaskCarrier(operation, inner, MutableByteSpan(bytes, sizeof(bytes)), encoded));
        socket.incoming.push_back(frame(bbp2::MessageKind::TaskManagement, encoded));
    }
    void enqueueAuthority() {
        TaskAuthorityBinding root; uint8_t bytes[kTaskAuthorityChangeMaxSize]; ByteView encoded;
        CHECK(encodeTaskAuthorityChange(change(root), MutableByteSpan(bytes, sizeof(bytes)), encoded));
        enqueue(TaskWireOperation::Authority, encoded);
    }
    ByteView response(TaskWireOperation expected) {
        const auto& bytes = socket.sent.back(); bbp2::FrameView f; TaskWireOperation operation; ByteView inner;
        CHECK(bbp2::parseFrame(ByteView(bytes.data(), bytes.size()), f));
        CHECK(f.header.kind == static_cast<uint8_t>(bbp2::MessageKind::TaskManagement));
        CHECK(f.header.sequence == sequence && f.header.flags == bbp2::FlagIsResponse);
        CHECK(decodeTaskCarrier(f.body, operation, inner)); CHECK(operation == expected); return inner;
    }
    TaskManagementChallenge challenge() {
        TaskManagementChallenge value; CHECK(decodeTaskManagementChallenge(response(TaskWireOperation::Challenge), value)); return value;
    }
    void commitCloud(const TaskManagementChallenge& pending) {
        const Bytes grant = verifier.sign(pending); enqueue(TaskWireOperation::Commit, ByteView(grant.data(), grant.size()));
        runtime.poll(1000U);
    }
    void verifyReceipt(const TaskManagementChallenge& pending) {
        uint8_t secret[32]; memset(secret, key.version, sizeof(secret)); TaskManagementResult value;
        CHECK(verifyTaskManagementResult(response(TaskWireOperation::Result), pending, TaskResultKey::DeviceKey,
            ByteView(secret, sizeof(secret)), value));
    }
};

void testRetirementCloudDispatch(bool rotated) {
    CloudFixture f; f.hello(); f.enqueueAuthority(); f.runtime.poll(1000U);
    f.commitCloud(f.challenge());
    TaskRetirementPrepare request; CHECK(f.engine.authorityBinding(request.authority)); request.requestId[0] = 81U;
    if (rotated) {
        const auto stored = request.authority;
        ++f.key.version; CHECK(!f.adapter.refreshRoots()); CHECK(f.cache.root(request.authority));
        memcpy(request.authority.taskScope, stored.taskScope, 16U);
    }
    uint8_t bytes[87]; ByteView encoded;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.clock.now += 10U; f.enqueue(TaskWireOperation::Retire, encoded); f.runtime.poll(1000U);
    const auto pending = f.challenge();
    f.socket.block = true; f.commitCloud(pending);
    CHECK(f.engine.state() == TaskEngineState::Retired && f.flash.writes == 2U);
    // Lose the durable result, then re-Hello and query with a fresh challenge.
    f.socket.online = false; f.runtime.poll(1000U); f.socket.block = false;
    f.clock.now += 5000U; f.runtime.poll(1000U); f.hello();
    request.requestId[0] = 82U;
    CHECK(encodeTaskRetirementPrepare(request, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.enqueue(TaskWireOperation::RetirementRead, encoded); f.runtime.poll(1000U);
    const auto query = f.challenge(); f.commitCloud(query); f.verifyReceipt(query);
    CHECK(f.flash.writes == 2U && f.socket.connects == 2U && f.target.applied == 0U);
    TaskManagementResult value; uint8_t secret[32]; memset(secret, f.key.version, 32U);
    CHECK(verifyTaskManagementResult(f.response(TaskWireOperation::Result), query, TaskResultKey::DeviceKey, ByteView(secret, 32U), value));
    CHECK(value.kind == TaskManagementResultKind::Retired && value.retirementId[0] == 81U);
}

void testRealCloudDispatch() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        CloudFixture f;
        if (mode != 0U) f.hello(mode != 1U, 211U);
        const auto sent = f.socket.sent.size(); f.enqueueAuthority(); f.runtime.poll(1000U);
        CHECK(f.socket.sent.size() == sent && !f.session.pending() && f.flash.writes == 0U);
    }
    CloudFixture f; f.hello(); f.enqueueAuthority(); f.runtime.poll(1000U);
    auto pending = f.challenge(); CHECK(f.flash.writes == 0U);
    f.commitCloud(pending); f.verifyReceipt(pending); CHECK(f.flash.writes == 1U && f.engine.state() == TaskEngineState::Ready);
    f.clock.now += 10U;
    auto mutation = f.create(); uint8_t bytes[197]; ByteView encoded;
    CHECK(encodeTaskMutationPrepare(mutation, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.enqueue(TaskWireOperation::Mutation, encoded); f.runtime.poll(1000U); pending = f.challenge();
    f.socket.block = true; f.commitCloud(pending); CHECK(f.flash.writes == 2U);
    f.runtime.poll(1000U); CHECK(f.flash.writes == 2U); // WouldBlock retries bytes, not commit.
    f.socket.block = false; f.runtime.poll(1000U); f.verifyReceipt(pending);
    // Ordinary BBP Hello/control lane still passes to the original Runtime.
    CHECK(f.socket.connects == 1U && f.engine.count() == 1U);
    f.clock.now += 10U; TaskReadPrepare read; memcpy(read.requestId, f.requestId, 16U);
    CHECK(encodeTaskReadPrepare(read, MutableByteSpan(bytes, sizeof(bytes)), encoded));
    f.enqueue(TaskWireOperation::Read, encoded); f.runtime.poll(1000U); pending = f.challenge();
    f.socket.online = false; f.runtime.poll(1000U); CHECK(!f.session.pending());
    f.clock.now += 5000U; f.runtime.poll(1000U); CHECK(f.socket.connects == 2U);
    f.hello(); f.commitCloud(pending);
    ByteView failure = f.response(TaskWireOperation::Failure);
    CHECK(failure.size == 3U && failure.data[1] == 3U && failure.data[2] == 2U); // stale Grant unauthorized.
    CHECK(f.flash.writes == 2U);
    // Buffered requests are not executed after stop, expiry, or root change.
    for (unsigned event = 0U; event < 3U; ++event) {
        CloudFixture stale; stale.hello(); stale.enqueueAuthority();
        stale.socket.afterRx = [&]() {
            if (event == 0U) stale.transport.stop();
            else if (event == 1U) stale.adapter.invalidateRoots();
            else stale.clock.now += 1000U;
        };
        const auto before = stale.socket.sent.size(); stale.runtime.poll(1000U);
        CHECK(!stale.session.pending() && stale.flash.writes == 0U && stale.socket.sent.size() == before);
    }
    // A malformed inner operation yields only an UNAUTHENTICATED failure hint.
    CloudFixture bad; bad.hello(); uint8_t invalid = 0xFFU;
    bad.enqueue(TaskWireOperation::Mutation, ByteView(&invalid, 1U)); bad.runtime.poll(1000U);
    CHECK(bad.response(TaskWireOperation::Failure).data[2] == 1U && bad.flash.writes == 0U);
    for (unsigned kind = 0U; kind < 3U; ++kind) {
        CloudFixture denied; denied.hello(); denied.enqueueAuthority();
        auto& packet = denied.socket.incoming.back();
        if (kind == 0U) packet[4] = bbp2::FlagAckRequired;
        else if (kind == 1U) packet[6] = packet[7] = 0U;
        else packet[12] = static_cast<uint8_t>(TaskWireOperation::Result);
        const auto before = denied.socket.sent.size(); denied.runtime.poll(1000U);
        CHECK(!denied.session.pending() && denied.socket.sent.size() == before);
    }
    // Disconnect during an actual store commit: durable success is NOT undone,
    // no response escapes and the exclusive scratch is cleared after unwinding.
    CloudFixture cut; cut.hello(); cut.enqueueAuthority(); cut.runtime.poll(1000U);
    const auto challenged = cut.challenge(); const auto beforeCut = cut.socket.sent.size();
    cut.flash.hook = [&]() { cut.transport.setNetworkAvailable(false); };
    cut.commitCloud(challenged); CHECK(cut.flash.writes == 1U && cut.socket.sent.size() == beforeCut);
    for (uint8_t byte : cut.wire) CHECK(byte == 0U);
    // An unsent challenge expires; no late byte retry revives its nonce.
    CloudFixture slow; slow.hello(); slow.enqueueAuthority(); slow.socket.block = true; slow.runtime.poll(1000U);
    CHECK(slow.session.pending()); const auto beforeSlow = slow.socket.sent.size();
    slow.clock.now += 1000U; slow.socket.block = false; slow.runtime.poll(1000U);
    CHECK(!slow.session.pending() && slow.socket.sent.size() == beforeSlow);
    std::cout << "task cloud carrier bytes=" << sizeof(f.transport) << " + workspace=" << sizeof(f.wire) << '\n';
}
} // namespace

int main() {
    testAuthenticatedRejection();
    testTargetCatalog();
    testRetirementSession(); testRetirementCloudDispatch(false); testRetirementCloudDispatch(true);
    testCrossRootRetirement(); testCrossRootRetirementFailures();
    testRetiredScopeCannotResume();
    testManualKeyContinuity();
    testLifecycle(); testOperations(); testWindows(); testRebindAndDurability(); testDigestCoverage(); testStartAndLostReceipt();
    testEncodedManagementFlow();
    testCommandAdapter(); testManagementProductLifecycle();
    testRealCloudDispatch();
    std::cout << "task management session: lifecycle/operations/nonce windows/durability/digests passed; cache="
              << sizeof(TaskAuthorityCache) << " session=" << sizeof(TaskManagementSession) << " bytes\n";
}
