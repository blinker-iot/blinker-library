// Original real product/Store fixtures only; the BC-2a suite is not executed.
#define BLINKER_BC2A_TEST_MAIN bc2aExistingProductSuiteMain
#include "test_cooperative_initial_cloud_key_product_main.cpp"
#undef BLINKER_BC2A_TEST_MAIN

namespace {
// External atomic-storage fake: callbacks model cancellation/time advancing
// while the actual DeviceAccessStore reloads its root. No admission decision
// or Store/Codec implementation is replaced by this test fixture.
class LateRootBlob final : public IAtomicBlobStore {
public:
    LateRootBlob() : bytes(), present(false), reads(0U), writes(0U), failAfterWrite(false), duringRead() {}
    Result load(MutableByteSpan output, size_t& written) override {
        ++reads; written = 0U;
        if (duringRead) { const auto callback = duringRead; duringRead = nullptr; callback(); }
        if (!present) return Result::failure(ErrorCode::NotFound);
        if (output.data == nullptr || output.size < sizeof(bytes)) return Result::failure(ErrorCode::BufferTooSmall);
        std::memcpy(output.data, bytes, sizeof(bytes)); written = sizeof(bytes);
        return Result::success();
    }
    Result replace(ByteView input) override {
        CHECK(input.data != nullptr && input.size == sizeof(bytes));
        ++writes; std::memcpy(bytes, input.data, sizeof(bytes)); present = true;
        return failAfterWrite ? Result::failure(ErrorCode::WouldBlock) : Result::success();
    }
    Result clear() override { CHECK(false); return Result::failure(ErrorCode::UnsupportedFeature); }
    StorageProtection protection() const override { return StorageProtection::PlainFlash; }
    uint8_t bytes[DeviceAccessStore::serializedSize];
    bool present;
    unsigned reads, writes;
    bool failAfterWrite;
    std::function<void()> duringRead;
};

ControllerCredential lateAdmin() { ControllerCredential admin = ip4Admin(); admin.credentialVersion = 3U; return admin; }
struct LateRootFixture {
    LateRootBlob memory;
    DeviceAccessStore access;
    TaskRootOwnerGate gate;
    ControllerCredential admin;
    LateRootFixture() : memory(), access(memory), gate(), admin(lateAdmin()) {
        CHECK(access.bootstrapAccess(1U, admin));
        access.setChangeObserver(&gate); CHECK(gate.enterOwner());
    }
    ~LateRootFixture() { access.setChangeObserver(nullptr); gate.leaveOwner(); clearControllerCredential(admin); }
};

// Test port for a LIVE consumer, not a production authorization substitute.
// It borrows the expected inputs only during this synchronous call and retains
// no actual root secret from validateInitialCloudKeyCommit's borrowed argument.
class LateGuard final : public IInitialCloudKeyCommitGuard {
public:
    LateGuard(LateRootFixture& fixture, const DeviceKey& candidate)
        : f(fixture), expected(candidate), calls(0U), live(true), now(0U), deadline(10U),
          expectedWrites(fixture.memory.writes), minimumReads(fixture.memory.reads + 1U) {}
    Result validateInitialCloudKeyCommit(const DeviceKey& candidate, uint32_t epoch,
        const ControllerCredential& actualAdmin) override {
        ++calls;
        CHECK(f.memory.reads >= minimumReads && f.memory.writes == expectedWrites);
        CHECK(!f.gate.idle() && f.gate.beginDeviceAccessChange().code() == ErrorCode::WouldBlock);
        CHECK(epoch == 1U && sameDeviceKey(candidate, expected));
        CHECK(&actualAdmin != &f.admin && sameControllerCredential(actualAdmin, f.admin));
        return live && now < deadline ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
    }
    LateRootFixture& f;
    const DeviceKey& expected;
    unsigned calls;
    bool live;
    uint32_t now, deadline;
    unsigned expectedWrites, minimumReads;
};

void lateGuardSeesActualRootAfterCasBeforeWrite() {
    LateRootFixture f; const DeviceKey key = bc2Key(); LateGuard guard(f, key);
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, guard));
    CHECK(guard.calls == 1U && f.memory.writes == guard.expectedWrites + 1U && !f.gate.uncertain());
    ControllerCredential retained;
    CHECK(f.access.loadActive(f.admin.id(), ControllerCredentialDomain::PlatformAccess, 1U, retained));
    CHECK(sameControllerCredential(retained, f.admin)); clearControllerCredential(retained);
}

void sameKeyNoOpStillRequiresTheLiveGuard() {
    LateRootFixture f; const DeviceKey key = bc2Key();
    CHECK(f.access.installInitialCloudKeyVerified(key, 1U, f.admin));
    LateGuard refused(f, key); refused.live = false;
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, refused).code() == ErrorCode::AuthenticationRequired);
    CHECK(refused.calls == 1U && f.memory.writes == refused.expectedWrites && !f.gate.uncertain());
    LateGuard fresh(f, key);
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, fresh));
    CHECK(fresh.calls == 1U && f.memory.writes == fresh.expectedWrites && !f.gate.uncertain());
}

void cancellationAndDeadlineDuringRealRootReadRejectWithoutPoison() {
    for (bool expire : {false, true}) {
        LateRootFixture f; const DeviceKey key = bc2Key(); LateGuard guard(f, key);
        f.memory.duringRead = [&]() { if (expire) guard.now = guard.deadline; else guard.live = false; };
        CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, guard).code() == ErrorCode::AuthenticationRequired);
        CHECK(guard.calls == 1U && f.memory.writes == guard.expectedWrites && !f.gate.uncertain());
        DeviceKey missing; CHECK(f.access.load(missing).code() == ErrorCode::NotFound);
        LateGuard explicitFresh(f, key);
        CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, explicitFresh));
        CHECK(explicitFresh.calls == 1U && f.memory.writes == guard.expectedWrites + 1U);
    }
}

void rootMismatchAndUnknownWriteNeverReachALaterGuard() {
    LateRootFixture f; const DeviceKey key = bc2Key(); LateGuard guard(f, key);
    ControllerCredential stale = f.admin; stale.secret[0] ^= 1U;
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, stale, guard).code() == ErrorCode::StateConflict);
    CHECK(guard.calls == 0U && f.memory.writes == guard.expectedWrites && !f.gate.uncertain());
    clearControllerCredential(stale);
    f.memory.failAfterWrite = true;
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, guard).code() == ErrorCode::WouldBlock);
    CHECK(guard.calls == 1U && f.gate.uncertain());
    const unsigned reads = f.memory.reads, writes = f.memory.writes;
    LateGuard cannotReopen(f, key);
    CHECK(f.access.installInitialCloudKeyAuthorized(key, 1U, f.admin, cannotReopen).code() == ErrorCode::StateConflict);
    CHECK(cannotReopen.calls == 0U && f.memory.reads == reads && f.memory.writes == writes);
}

class ProductLateGuard final : public IInitialCloudKeyCommitGuard {
public:
    ProductLateGuard(Bc2Fixture& fixture, const ControllerCredential& admin)
        : f(fixture), expected(admin), calls(0U), live(false), writes(nvs.sets), roots(bc2RootReads) {}
    Result validateInitialCloudKeyCommit(const DeviceKey& candidate, uint32_t epoch,
        const ControllerCredential& actualAdmin) override {
        ++calls;
        CHECK(!f.extension.quiescent() && nvs.sets == writes && bc2SnapshotReads != 0U);
        CHECK(bc2RootReads >= roots + 4U); // Real product pre-read AND lower Store/CAS read.
        CHECK(epoch == 1U && sameDeviceKey(candidate, bc2Key()));
        CHECK(&actualAdmin != &expected && sameControllerCredential(actualAdmin, expected));
        return live ? Result::success() : Result::failure(ErrorCode::AuthenticationRequired);
    }
    Bc2Fixture& f;
    const ControllerCredential& expected;
    unsigned calls;
    bool live;
    unsigned writes, roots;
};

void realProductForwardsGuardWithoutDowngradingToVerified() {
    bc2Reset(); Bc2Fixture f; const ControllerCredential admin = ip4Admin();
    IInitialCloudKeyInstaller& installer = f.extension;
    ProductLateGuard cancelled(f, admin);
    CHECK(installer.installInitialCloudKeyAuthorized(bc2Key(), 1U, admin, cancelled).code() == ErrorCode::AuthenticationRequired);
    CHECK(cancelled.calls == 1U && nvs.sets == cancelled.writes && f.extension.quiescent());
    ProductLateGuard explicitFresh(f, admin); explicitFresh.live = true;
    CHECK(installer.installInitialCloudKeyAuthorized(bc2Key(), 1U, admin, explicitFresh));
    CHECK(explicitFresh.calls == 1U && nvs.sets == explicitFresh.writes + 1U && f.extension.quiescent());
    CHECK(bc2Opens == 2U); // Original TaskStore handle, not a second installer graph.
    f.noTaskWrite();
}

void originalBoundAndOwnerChecksPrecedeTheGuard() {
    {
        bc2Reset(); bc2SeedTask(false); Bc2Fixture f; const ControllerCredential admin = ip4Admin();
        ProductLateGuard guard(f, admin); guard.live = true;
        CHECK(f.extension.installInitialCloudKeyAuthorized(bc2Key(), 1U, admin, guard).code() == ErrorCode::StateConflict);
        CHECK(guard.calls == 0U && nvs.sets == guard.writes && f.extension.quiescent()); f.noTaskWrite();
    }
    {
        bc2Reset(); Bc2Fixture f; const ControllerCredential admin = ip4Admin();
        ProductLateGuard guard(f, admin); guard.live = true;
        CHECK(f.extension.enterMaintenance());
        CHECK(f.extension.installInitialCloudKeyAuthorized(bc2Key(), 1U, admin, guard).code() == ErrorCode::WouldBlock);
        CHECK(guard.calls == 0U && nvs.sets == guard.writes && bc2Opens == 1U && bc2SnapshotReads == 0U);
        f.extension.leaveMaintenance(); f.noTaskWrite();
    }
}
} // namespace

int main() {
    lateGuardSeesActualRootAfterCasBeforeWrite();
    sameKeyNoOpStillRequiresTheLiveGuard();
    cancellationAndDeadlineDuringRealRootReadRejectWithoutPoison();
    rootMismatchAndUnknownWriteNeverReachALaterGuard();
    realProductForwardsGuardWithoutDowngradingToVerified();
    originalBoundAndOwnerChecksPrecedeTheGuard();
    std::cout << "Initial Cloud Key commit guard: 6 finite groups passed (real Store/product, fake storage/consumer)\n";
    return 0;
}
