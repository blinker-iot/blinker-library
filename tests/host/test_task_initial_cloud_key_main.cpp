#define main original_device_access_store_tests_main
#include "test_device_access_store_main.cpp"
#undef main
#include <BlinkerV2/runtime/TaskRootOwnerGate.h>

namespace {
class InitialCloudKeyObserver final : public blinker::IDeviceAccessChangeObserver {
public:
    explicit InitialCloudKeyObserver(blinker::TaskRootOwnerGate& gate)
        : gate_(gate), initialBegins(0U), initialEnds(0U), ordinaryBegins(0U) {}
    blinker::Result beginDeviceAccessChange() override {
        ++ordinaryBegins; return gate_.beginDeviceAccessChange();
    }
    void endDeviceAccessChange(blinker::Result result) override { gate_.endDeviceAccessChange(result); }
    blinker::Result beginCooperativeInitialCloudKeyChange() override {
        ++initialBegins; return gate_.beginCooperativeInitialCloudKeyChange();
    }
    void endCooperativeInitialCloudKeyChange(blinker::Result result) override {
        ++initialEnds; gate_.endCooperativeInitialCloudKeyChange(result);
    }
    blinker::TaskRootOwnerGate& gate_;
    unsigned initialBegins, initialEnds, ordinaryBegins;
};

struct InitialKeyFixture {
    MemoryStore memory;
    blinker::DeviceAccessStore store;
    blinker::TaskRootOwnerGate gate;
    InitialCloudKeyObserver observer;
    blinker::ControllerCredential admin;
    InitialKeyFixture()
        : memory(), store(memory), gate(), observer(gate),
          admin(credential(0x10U, 0x20U, 5U, 3U, blinker::kControllerPermissionAll)) {
        CHECK_TRUE(store.bootstrapAccessWithPresence(5U, admin, presenceKey(1U, 0x60U)));
        store.setChangeObserver(&observer);
    }
    blinker::Result install(const blinker::DeviceKey& key) {
        return store.installInitialCloudKeyVerified(key, 5U, admin);
    }
    void unchangedAccess() {
        blinker::ControllerCredential loaded;
        CHECK_TRUE(store.loadActive(admin.id(), admin.domain, 5U, loaded));
        CHECK_TRUE(blinker::sameControllerCredential(admin, loaded));
        blinker::DevicePresenceKey presence;
        CHECK_TRUE(store.loadPresence(5U, presence));
        CHECK_TRUE(blinker::sameDevicePresenceKey(presence, presenceKey(1U, 0x60U)));
    }
};

void noOwnerAndDefaultObserverCannotAdmitInitialKey() {
    InitialKeyFixture f;
    const uint32_t reads = f.memory.loadCalls;
    CHECK_EQ(blinker::ErrorCode::NotConfigured, f.install(deviceKey(1U)).code());
    CHECK_EQ(reads, f.memory.loadCalls);
    CHECK_EQ(1U, f.memory.replaceCalls);
    f.store.setChangeObserver(nullptr);
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature, f.install(deviceKey(1U)).code());
    ChangeObserver ordinary;
    f.store.setChangeObserver(&ordinary);
    CHECK_EQ(blinker::ErrorCode::UnsupportedFeature, f.install(deviceKey(1U)).code());
    CHECK_EQ(0U, ordinary.before);
    CHECK_EQ(reads, f.memory.loadCalls);
}

void ordinaryFirstAddIsDeniedAndBootstrapRemainsOriginal() {
    InitialKeyFixture f;
    CHECK_EQ(blinker::ErrorCode::StateConflict, f.store.replace(deviceKey(1U)).code());
    CHECK_EQ(1U, f.memory.replaceCalls);
    CHECK_EQ(1U, f.observer.ordinaryBegins);
    f.unchangedAccess();
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 5U, f.admin));
    const unsigned writes = memory.replaceCalls;
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 5U, f.admin));
    CHECK_EQ(writes, memory.replaceCalls);
}

void firstInstallPreservesRotatedAdminPresenceAndHeldOwner() {
    InitialKeyFixture f;
    CHECK_TRUE(f.store.hasChangeObserver(&f.observer));
    CHECK_TRUE(!f.store.hasChangeObserver(nullptr));
    CHECK_TRUE(f.gate.enterOwner());
    const uint32_t writes = f.memory.replaceCalls;
    CHECK_TRUE(f.install(deviceKey(1U)));
    CHECK_EQ(writes + 1U, f.memory.replaceCalls);
    CHECK_EQ(1U, f.observer.initialBegins);
    CHECK_EQ(1U, f.observer.initialEnds);
    CHECK_EQ(0U, f.observer.ordinaryBegins);
    CHECK_TRUE(!f.gate.idle() && !f.gate.uncertain());
    f.unchangedAccess();
    blinker::DeviceKey key;
    CHECK_TRUE(f.store.load(key) && blinker::sameDeviceKey(key, deviceKey(1U)));
    f.gate.leaveOwner(); CHECK_TRUE(f.gate.idle());
}

void sameKeyUsesFenceWithoutWritingAndDifferentKeyCannotRotate() {
    InitialKeyFixture f; CHECK_TRUE(f.gate.enterOwner());
    CHECK_TRUE(f.install(deviceKey(1U)));
    const uint32_t writes = f.memory.replaceCalls;
    CHECK_TRUE(f.install(deviceKey(1U)));
    CHECK_EQ(writes, f.memory.replaceCalls);
    CHECK_EQ(2U, f.observer.initialBegins);
    CHECK_EQ(2U, f.observer.initialEnds);
    CHECK_EQ(blinker::ErrorCode::StateConflict, f.install(deviceKey(2U)).code());
    CHECK_TRUE(!f.gate.uncertain());
    CHECK_EQ(writes, f.memory.replaceCalls);
    f.gate.leaveOwner();
    CHECK_TRUE(f.store.replace(deviceKey(1U))); // Original exact replay, ordinary fence.
    CHECK_EQ(1U, f.observer.ordinaryBegins);
    CHECK_EQ(writes, f.memory.replaceCalls);
}

void exactAdminAndEpochCasCannotBeSubstituted() {
    InitialKeyFixture f; CHECK_TRUE(f.gate.enterOwner());
    for (unsigned index = 0U; index < 4U; ++index) {
        blinker::ControllerCredential expected = f.admin;
        if (index == 0U) expected.controllerId[0] ^= 1U;
        if (index == 1U) ++expected.credentialVersion;
        if (index == 2U) expected.secret[0] ^= 1U;
        if (index == 3U) ++expected.ownershipGeneration;
        CHECK_TRUE(!f.store.installInitialCloudKeyVerified(deviceKey(1U),
            expected.ownershipGeneration, expected));
        CHECK_TRUE(!f.gate.uncertain());
        CHECK_EQ(1U, f.memory.replaceCalls);
    }
    blinker::ControllerCredential invalid = f.admin;
    invalid.permissions = blinker::kControllerPermissionObserve;
    CHECK_TRUE(!f.store.installInitialCloudKeyVerified(deviceKey(1U), 5U, invalid));
    invalid = f.admin; invalid.domain = blinker::ControllerCredentialDomain::Local;
    CHECK_TRUE(!f.store.installInitialCloudKeyVerified(deviceKey(1U), 5U, invalid));
    CHECK_TRUE(f.install(deviceKey(1U))); // Known no-write CAS rejections did not poison.
    f.gate.leaveOwner();
}

void unknownWriteCannotEscapeThroughSameKeyOrLegacyReplay() {
    InitialKeyFixture f; CHECK_TRUE(f.gate.enterOwner());
    f.memory.failAfterReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock, f.install(deviceKey(1U)).code());
    CHECK_TRUE(f.gate.uncertain());
    const uint32_t reads = f.memory.loadCalls;
    const uint32_t writes = f.memory.replaceCalls;
    CHECK_EQ(blinker::ErrorCode::StateConflict, f.install(deviceKey(1U)).code());
    CHECK_EQ(reads, f.memory.loadCalls); // Poison is checked before a possibly dirty root read.
    f.gate.leaveOwner();
    CHECK_EQ(blinker::ErrorCode::StateConflict, f.store.replace(deviceKey(1U)).code());
    CHECK_EQ(reads, f.memory.loadCalls);
    CHECK_EQ(writes, f.memory.replaceCalls);
}

void failedStoreAndCorruptRootPoisonWithoutRetry() {
    InitialKeyFixture failure; CHECK_TRUE(failure.gate.enterOwner());
    failure.memory.failReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock, failure.install(deviceKey(1U)).code());
    CHECK_TRUE(failure.gate.uncertain());
    failure.gate.leaveOwner();
    InitialKeyFixture corrupt; CHECK_TRUE(corrupt.gate.enterOwner());
    corrupt.memory.data[0] ^= 1U;
    CHECK_EQ(blinker::ErrorCode::InvalidEncoding, corrupt.install(deviceKey(1U)).code());
    CHECK_TRUE(corrupt.gate.uncertain());
    CHECK_EQ(1U, corrupt.memory.replaceCalls);
    corrupt.gate.leaveOwner();
}

void keyBorrowAndPresenceRemainExclusiveAndNeverUnlockNativeFence() {
    blinker::TaskRootOwnerGate gate;
    CHECK_TRUE(gate.enterOwner());
    CHECK_TRUE(gate.beginCooperativeInitialCloudKeyChange());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, gate.beginCooperativePresenceChange().code());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, gate.beginCooperativeInitialCloudKeyChange().code());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, gate.beginDeviceAccessChange().code());
    CHECK_TRUE(!gate.idle());
    gate.endCooperativeInitialCloudKeyChange(blinker::Result::success());
    CHECK_TRUE(gate.beginCooperativePresenceChange());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, gate.beginCooperativeInitialCloudKeyChange().code());
    gate.endCooperativePresenceChange(blinker::Result::success());
    CHECK_TRUE(!gate.idle());
    gate.leaveOwner(); CHECK_TRUE(gate.idle());
}

void bootstrapExactAliasCannotConfirmAnUnknownFirstKey() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const blinker::ControllerCredential admin = credential(
        0x10U, 0x20U, 5U, 3U, blinker::kControllerPermissionAll);
    CHECK_TRUE(store.bootstrapAccess(5U, admin)); // No Presence in this exact alias.
    blinker::TaskRootOwnerGate gate;
    store.setChangeObserver(&gate);
    CHECK_TRUE(gate.enterOwner());
    memory.failAfterReplace = true;
    CHECK_EQ(blinker::ErrorCode::WouldBlock,
        store.installInitialCloudKeyVerified(deviceKey(1U), 5U, admin).code());
    CHECK_TRUE(gate.uncertain());
    gate.leaveOwner();
    const uint32_t writes = memory.replaceCalls;
    CHECK_TRUE(!store.bootstrap(deviceKey(1U), 5U, admin));
    CHECK_EQ(writes, memory.replaceCalls);

    MemoryStore cleanMemory;
    blinker::DeviceAccessStore clean(cleanMemory);
    CHECK_TRUE(clean.bootstrap(deviceKey(1U), 5U, admin));
    blinker::TaskRootOwnerGate cleanGate;
    clean.setChangeObserver(&cleanGate);
    const uint32_t cleanWrites = cleanMemory.replaceCalls;
    CHECK_TRUE(clean.bootstrap(deviceKey(1U), 5U, admin));
    CHECK_TRUE(cleanGate.enterOwner());
    CHECK_EQ(blinker::ErrorCode::WouldBlock, clean.bootstrap(deviceKey(1U), 5U, admin).code());
    cleanGate.leaveOwner();
    CHECK_EQ(cleanWrites, cleanMemory.replaceCalls);
}
} // namespace

int main() {
    noOwnerAndDefaultObserverCannotAdmitInitialKey();
    ordinaryFirstAddIsDeniedAndBootstrapRemainsOriginal();
    firstInstallPreservesRotatedAdminPresenceAndHeldOwner();
    sameKeyUsesFenceWithoutWritingAndDifferentKeyCannotRotate();
    exactAdminAndEpochCasCannotBeSubstituted();
    unknownWriteCannotEscapeThroughSameKeyOrLegacyReplay();
    failedStoreAndCorruptRootPoisonWithoutRetry();
    keyBorrowAndPresenceRemainExclusiveAndNeverUnlockNativeFence();
    bootstrapExactAliasCannotConfirmAnUnknownFirstKey();
    std::cout << "Initial Cloud Key Store/Gate: 9 finite cases passed" << std::endl;
}
