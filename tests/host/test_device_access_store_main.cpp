#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "Device access check failed at line "                \
                      << __LINE__ << ": " << #condition << std::endl;          \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

std::vector<uint8_t> fromHex(const char* text) {
    const size_t length = std::strlen(text);
    CHECK_TRUE((length % 2U) == 0U);
    std::vector<uint8_t> output(length / 2U);
    for (size_t index = 0U; index < output.size(); ++index) {
        const char pair[3] = {
            text[index * 2U], text[index * 2U + 1U], '\0'};
        output[index] = static_cast<uint8_t>(
            std::strtoul(pair, nullptr, 16));
    }
    return output;
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

class MemoryStore final : public blinker::IAtomicBlobStore {
public:
    MemoryStore()
        : data(), size(0U), present(false), failReplace(false), failClear(false),
          ignoreClear(false),
          replaceCalls(0U), clearCalls(0U) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        written = size;
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, data, size);
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        ++replaceCalls;
        if (failReplace) {
            failReplace = false;
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        CHECK_TRUE(value.size <= sizeof(data));
        std::memset(data, 0, sizeof(data));
        std::memcpy(data, value.data, value.size);
        size = value.size;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        ++clearCalls;
        if (failClear) {
            failClear = false;
            return blinker::Result::failure(blinker::ErrorCode::WouldBlock);
        }
        if (ignoreClear) return blinker::Result::success();
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        std::memset(data, 0, sizeof(data));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return blinker::StorageProtection::EncryptedFlash;
    }

    uint8_t data[264];
    size_t size;
    bool present;
    bool failReplace;
    bool failClear;
    bool ignoreClear;
    uint32_t replaceCalls;
    uint32_t clearCalls;
};

blinker::DeviceKey deviceKey(uint8_t base) {
    blinker::DeviceKey value;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        value.bytes[index] = static_cast<uint8_t>(base + index);
    }
    return value;
}

blinker::DevicePresenceKey presenceKey(
    uint32_t version,
    uint8_t base) {
    blinker::DevicePresenceKey value;
    value.version = version;
    for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
        value.bytes[index] = static_cast<uint8_t>(base + index);
    }
    return value;
}

blinker::ControllerCredential credential(
    uint8_t idBase,
    uint8_t secretBase,
    uint32_t epoch,
    uint32_t version,
    uint32_t permissions) {
    blinker::ControllerCredential value;
    value.domain = blinker::ControllerCredentialDomain::PlatformAccess;
    value.ownershipGeneration = epoch;
    value.credentialVersion = version;
    value.permissions = permissions;
    for (size_t index = 0U; index < sizeof(value.controllerId); ++index) {
        value.controllerId[index] = static_cast<uint8_t>(idBase + index);
    }
    for (size_t index = 0U; index < sizeof(value.secret); ++index) {
        value.secret[index] = static_cast<uint8_t>(secretBase + index);
    }
    return value;
}

void repairCrc(MemoryStore& memory) {
    CHECK_TRUE(memory.size == blinker::DeviceAccessStore::legacySerializedSize ||
               memory.size == blinker::DeviceAccessStore::previousSerializedSize ||
               memory.size == blinker::DeviceAccessStore::serializedSize);
    const size_t crcOffset = memory.size - 4U;
    writeU32(
        memory.data + crcOffset,
        blinker::computeCrc32(blinker::ByteView(memory.data, crcOffset)));
}

void testBootstrapAndExactVector() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const blinker::DeviceKey key = deviceKey(1U);
    const blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        5U,
        3U,
        blinker::kControllerPermissionAll);

    CHECK_TRUE(store.bootstrap(key, 5U, admin));
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::serializedSize),
        memory.size);
    const std::vector<uint8_t> expected = fromHex(
        "4241533203030101000000050102030405060708090a0b0c0d0e0f1011121314"
        "15161718191a1b1c1d1e1f200000000000000000000000000000000000000000"
        "0101020000000005000000030000000f101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "5bcc8638");
    CHECK_EQ(expected.size(), memory.size);
    CHECK_TRUE(std::memcmp(expected.data(), memory.data, memory.size) == 0);

    const uint32_t writes = memory.replaceCalls;
    CHECK_TRUE(store.bootstrap(key, 5U, admin));
    CHECK_EQ(writes, memory.replaceCalls);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.bootstrap(deviceKey(2U), 5U, admin).code());

    blinker::DeviceKey loadedKey;
    CHECK_TRUE(store.load(loadedKey));
    CHECK_TRUE(blinker::sameDeviceKey(key, loadedKey));
    uint32_t epoch = 0U;
    CHECK_TRUE(store.loadAccessEpoch(epoch));
    CHECK_EQ(static_cast<uint32_t>(5U), epoch);
    blinker::ControllerCredential loaded;
    CHECK_TRUE(store.loadActive(
        admin.id(), admin.domain, 5U, loaded));
    CHECK_TRUE(blinker::sameControllerCredential(admin, loaded));
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.loadActive(admin.id(), admin.domain, 6U, loaded).code());
}

void testKeyMutationPreservesDirectAccess() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const blinker::DeviceKey first = deviceKey(1U);
    const blinker::DeviceKey second = deviceKey(0x40U);
    const blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        7U,
        1U,
        blinker::kControllerPermissionAll);
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.replace(first).code());
    CHECK_TRUE(store.bootstrap(first, 7U, admin));
    CHECK_TRUE(store.replace(second));
    blinker::ControllerCredential loaded;
    CHECK_TRUE(store.loadActive(
        admin.id(), admin.domain, 7U, loaded));
    CHECK_TRUE(store.clear());
    blinker::DeviceKey ignored;
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(ignored).code());
    uint32_t epoch = 0U;
    CHECK_TRUE(store.loadAccessEpoch(epoch));
    CHECK_EQ(static_cast<uint32_t>(7U), epoch);
    CHECK_TRUE(store.loadActive(
        admin.id(), admin.domain, 7U, loaded));
    CHECK_TRUE(store.replace(first));
    CHECK_TRUE(store.load(ignored));
    CHECK_TRUE(blinker::sameDeviceKey(first, ignored));
}

void testAdminSharedRules() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const uint32_t sharedPermissions =
        blinker::kControllerPermissionObserve |
        blinker::kControllerPermissionControl;
    const blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        9U,
        1U,
        blinker::kControllerPermissionAll);
    blinker::ControllerCredential shared = credential(
        0x30U, 0x50U, 9U, 1U, sharedPermissions);
    const blinker::ControllerCredential gateway = credential(
        0x50U, 0x70U, 9U, 1U, sharedPermissions);
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 9U, admin));
    CHECK_TRUE(store.installVerified(shared));
    CHECK_TRUE(store.installVerified(gateway));
    const uint32_t installedWrites = memory.replaceCalls;
    CHECK_TRUE(store.installVerified(gateway));
    CHECK_EQ(installedWrites, memory.replaceCalls);
    // Reconstructing the store must not turn an exact third-slot replay into
    // another durable write, including when all controller slots are occupied.
    blinker::DeviceAccessStore reopened(memory);
    CHECK_TRUE(reopened.installVerified(gateway));
    CHECK_EQ(installedWrites, memory.replaceCalls);
    blinker::ControllerCredential conflicting = gateway;
    conflicting.secret[0] ^= 1U;
    CHECK_EQ(blinker::ErrorCode::StateConflict,
             reopened.installVerified(conflicting).code());
    CHECK_EQ(installedWrites, memory.replaceCalls);
    blinker::ControllerCredential unchanged;
    CHECK_TRUE(reopened.loadActive(gateway.id(), gateway.domain, 9U, unchanged));
    CHECK_TRUE(blinker::sameControllerCredential(gateway, unchanged));
    blinker::clearControllerCredential(conflicting);
    blinker::clearControllerCredential(unchanged);
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.installVerified(credential(
            0x70U, 0x90U, 9U, 1U, sharedPermissions)).code());
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.installVerified(credential(
            0x05U,
            0x70U,
            9U,
            1U,
            blinker::kControllerPermissionAll)).code());

    blinker::ControllerCredential escalated = shared;
    escalated.credentialVersion = 2U;
    escalated.permissions = blinker::kControllerPermissionAll;
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.rotateVerified(1U, escalated).code());
    shared.credentialVersion = 2U;
    shared.secret[0] ^= 0x55U;
    CHECK_TRUE(store.rotateVerified(1U, shared));
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.revokeVerified(
            admin.id(), admin.domain, 9U, 1U).code());
    CHECK_TRUE(store.revokeVerified(
        shared.id(), shared.domain, 9U, 2U));
    size_t count = 0U;
    CHECK_TRUE(store.count(count));
    CHECK_EQ(static_cast<size_t>(2U), count);

    CHECK_TRUE(store.clearAll());
    CHECK_TRUE(store.count(count));
    CHECK_EQ(static_cast<size_t>(0U), count);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.installVerified(shared).code());
    CHECK_TRUE(store.installVerified(admin));
}

void testLegacyMigrationAndPresenceCas() {
    MemoryStore memory;
    const std::vector<uint8_t> legacy = fromHex(
        "4241533201020101000000050102030405060708090a0b0c0d0e0f1011121314"
        "15161718191a1b1c1d1e1f200101020000000005000000030000000f10111213"
        "1415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f30313233"
        "3435363738393a3b3c3d3e3f0000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000009df24a8b");
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::legacySerializedSize),
        legacy.size());
    CHECK_TRUE(memory.replace(blinker::ByteView(
        legacy.data(), legacy.size())));

    blinker::DeviceAccessStore store(memory);
    uint32_t epoch = 0U;
    CHECK_TRUE(store.loadAccessEpoch(epoch));
    CHECK_EQ(static_cast<uint32_t>(5U), epoch);
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::legacySerializedSize),
        memory.size);
    blinker::DevicePresenceKey loaded;
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.loadPresence(5U, loaded).code());

    const blinker::DevicePresenceKey first = presenceKey(1U, 0x80U);
    CHECK_TRUE(store.replacePresenceVerified(5U, 0U, first));
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::serializedSize),
        memory.size);
    CHECK_EQ(static_cast<uint8_t>(3U), memory.data[4]);
    CHECK_TRUE((memory.data[7] & 0x02U) != 0U);
    CHECK_TRUE(store.loadPresence(5U, loaded));
    CHECK_TRUE(blinker::sameDevicePresenceKey(first, loaded));

    const uint32_t writes = memory.replaceCalls;
    CHECK_TRUE(store.replacePresenceVerified(5U, 0U, first));
    CHECK_EQ(writes, memory.replaceCalls);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.replacePresenceVerified(5U, 1U, first).code());
    CHECK_EQ(writes, memory.replaceCalls);
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.replacePresenceVerified(
            6U, 1U, presenceKey(2U, 0x90U)).code());
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.replacePresenceVerified(
            5U, 0U, presenceKey(2U, 0x90U)).code());

    const blinker::DevicePresenceKey second = presenceKey(2U, 0x90U);
    CHECK_TRUE(store.replacePresenceVerified(5U, 1U, second));
    CHECK_TRUE(store.loadPresence(5U, loaded));
    CHECK_TRUE(blinker::sameDevicePresenceKey(second, loaded));
}

void testPreviousMigrationAndThirdSlot() {
    MemoryStore memory;
    const std::vector<uint8_t> previous = fromHex(
        "4241533202020101000000050102030405060708090a0b0c0d0e0f1011121314"
        "15161718191a1b1c1d1e1f200000000000000000000000000000000000000000"
        "0101020000000005000000030000000f101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "5249dbbe");
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::previousSerializedSize),
        previous.size());
    CHECK_TRUE(memory.replace(blinker::ByteView(
        previous.data(), previous.size())));
    blinker::DeviceAccessStore store(memory);
    size_t count = 0U;
    CHECK_TRUE(store.count(count));
    CHECK_EQ(static_cast<size_t>(1U), count);
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::previousSerializedSize),
        memory.size);

    const uint32_t permissions =
        blinker::kControllerPermissionObserve |
        blinker::kControllerPermissionControl;
    CHECK_TRUE(store.installVerified(credential(
        0x40U, 0x50U, 5U, 1U, permissions)));
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceAccessStore::serializedSize),
        memory.size);
    CHECK_EQ(static_cast<uint8_t>(3U), memory.data[4]);
    CHECK_EQ(static_cast<uint8_t>(3U), memory.data[5]);
    CHECK_TRUE(store.installVerified(credential(
        0x60U, 0x70U, 5U, 1U, permissions)));
    CHECK_TRUE(store.count(count));
    CHECK_EQ(static_cast<size_t>(3U), count);
}

void testAtomicBootstrapWithPresence() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const blinker::DeviceKey key = deviceKey(1U);
    const blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        12U,
        1U,
        blinker::kControllerPermissionAll);
    const blinker::DevicePresenceKey presence = presenceKey(1U, 0x70U);
    CHECK_TRUE(store.bootstrapWithPresence(key, 12U, admin, presence));
    const uint32_t writes = memory.replaceCalls;
    CHECK_TRUE(store.bootstrapWithPresence(key, 12U, admin, presence));
    CHECK_EQ(writes, memory.replaceCalls);

    blinker::DevicePresenceKey loaded;
    CHECK_TRUE(store.loadPresence(12U, loaded));
    CHECK_TRUE(blinker::sameDevicePresenceKey(presence, loaded));
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.bootstrapWithPresence(
            key, 12U, admin, presenceKey(1U, 0x71U)).code());
    CHECK_EQ(
        blinker::ErrorCode::StateConflict,
        store.bootstrapWithPresence(
            key, 12U, admin, presenceKey(2U, 0x70U)).code());

    // A device may be erased after its server-side presence key has already
    // rotated. Re-enrollment installs the signed current version; bootstrap
    // must not assume that every empty device starts at version 1.
    MemoryStore recoveredMemory;
    blinker::DeviceAccessStore recoveredStore(recoveredMemory);
    const blinker::DevicePresenceKey recoveredPresence =
        presenceKey(2U, 0x72U);
    CHECK_TRUE(recoveredStore.bootstrapWithPresence(
        key, 12U, admin, recoveredPresence));
    blinker::DevicePresenceKey recoveredLoaded;
    CHECK_TRUE(recoveredStore.loadPresence(12U, recoveredLoaded));
    CHECK_TRUE(blinker::sameDevicePresenceKey(
        recoveredPresence, recoveredLoaded));

    CHECK_TRUE(store.clearAll());
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.loadPresence(12U, loaded).code());
    blinker::DeviceKey loadedKey;
    CHECK_TRUE(store.load(loadedKey));
    CHECK_TRUE(blinker::sameDeviceKey(key, loadedKey));
}

void testFaultsAndErase() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    const blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        11U,
        1U,
        blinker::kControllerPermissionAll);
    memory.failReplace = true;
    CHECK_EQ(
        blinker::ErrorCode::WouldBlock,
        store.bootstrap(deviceKey(1U), 11U, admin).code());
    CHECK_TRUE(store.bootstrap(deviceKey(1U), 11U, admin));
    memory.data[100U] ^= 1U;
    uint32_t epoch = 0U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        store.loadAccessEpoch(epoch).code());
    memory.data[100U] ^= 1U;
    repairCrc(memory);
    CHECK_TRUE(store.eraseAccess());
    CHECK_TRUE(store.eraseAccess());
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.loadAccessEpoch(epoch).code());
}

void testBleOnlyControllerClearReopensEnrollment() {
    MemoryStore memory;
    blinker::DeviceAccessStore store(memory);
    blinker::ControllerCredential admin = credential(
        0x10U,
        0x20U,
        3U,
        1U,
        blinker::kControllerPermissionAll);
    CHECK_TRUE(store.bootstrapAccessWithPresence(
        3U, admin, presenceKey(1U, 0x60U)));
    blinker::DevicePresenceKey loaded;
    CHECK_TRUE(store.loadPresence(3U, loaded));
    CHECK_TRUE(store.clearAll());
    uint32_t epoch = 0U;
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.loadAccessEpoch(epoch).code());

    admin.ownershipGeneration = 4U;
    CHECK_TRUE(store.bootstrapAccess(4U, admin));
}

void testWholeAccessResetAndReclaim() {
    for (unsigned int withCloudKey = 0U; withCloudKey < 2U; ++withCloudKey) {
        MemoryStore memory;
        blinker::DeviceAccessStore store(memory);
        const blinker::ControllerCredential admin = credential(
            0x10U, 0x20U, 3U, 1U, blinker::kControllerPermissionAll);
        const uint32_t permissions = blinker::kControllerPermissionObserve |
                                     blinker::kControllerPermissionControl;
        const blinker::ControllerCredential shared = credential(
            0x30U, 0x40U, 3U, 1U, permissions);
        const blinker::ControllerCredential gateway = credential(
            0x50U, 0x60U, 3U, 1U, permissions);
        const blinker::DevicePresenceKey presence = presenceKey(1U, 0x70U);
        CHECK_TRUE(withCloudKey ? store.bootstrapWithPresence(deviceKey(1U), 3U, admin, presence)
                                : store.bootstrapAccessWithPresence(3U, admin, presence));
        CHECK_TRUE(store.installVerified(shared));
        CHECK_TRUE(store.installVerified(gateway));
        const std::vector<uint8_t> before(memory.data, memory.data + memory.size);
        memory.failClear = true;
        CHECK_EQ(blinker::ErrorCode::WouldBlock, store.eraseAccess().code());
        CHECK_TRUE(before == std::vector<uint8_t>(memory.data, memory.data + memory.size));
        memory.ignoreClear = true;
        CHECK_EQ(blinker::ErrorCode::InternalError, store.eraseAccess().code());
        memory.ignoreClear = false;
        CHECK_TRUE(store.eraseAccess());
        CHECK_TRUE(store.eraseAccess());

        blinker::DeviceAccessStore reopened(memory);
        uint32_t epoch = 0U;
        blinker::DeviceKey loadedKey;
        blinker::DevicePresenceKey loadedPresence;
        blinker::ControllerCredential loaded;
        CHECK_EQ(blinker::ErrorCode::NotFound, reopened.load(loadedKey).code());
        CHECK_EQ(blinker::ErrorCode::NotFound, reopened.loadAccessEpoch(epoch).code());
        CHECK_EQ(blinker::ErrorCode::NotFound, reopened.loadPresence(3U, loadedPresence).code());
        const blinker::ControllerCredential old[] = {admin, shared, gateway};
        for (size_t index = 0U; index < 3U; ++index) {
            CHECK_EQ(blinker::ErrorCode::NotFound,
                     reopened.loadActive(old[index].id(), old[index].domain, 3U, loaded).code());
        }
        const blinker::ControllerCredential next = credential(
            0x80U, 0x90U, 4U, 1U, blinker::kControllerPermissionAll);
        CHECK_TRUE(withCloudKey
            ? reopened.bootstrapWithPresence(deviceKey(2U), 4U, next, presenceKey(1U, 0xa0U))
            : reopened.bootstrapAccessWithPresence(4U, next, presenceKey(1U, 0xa0U)));
        blinker::DeviceAccessStore rebound(memory);
        if (withCloudKey) {
            CHECK_TRUE(rebound.load(loadedKey));
            CHECK_TRUE(blinker::sameDeviceKey(deviceKey(2U), loadedKey));
            CHECK_TRUE(!blinker::sameDeviceKey(deviceKey(1U), loadedKey));
        } else {
            CHECK_EQ(blinker::ErrorCode::NotFound, rebound.load(loadedKey).code());
        }
        CHECK_TRUE(rebound.loadActive(next.id(), next.domain, 4U, loaded));
        CHECK_TRUE(blinker::sameControllerCredential(next, loaded));
        for (size_t index = 0U; index < 3U; ++index) {
            CHECK_TRUE(!rebound.loadActive(old[index].id(), old[index].domain, 3U, loaded));
            CHECK_TRUE(!rebound.loadActive(old[index].id(), old[index].domain, 4U, loaded));
        }
    }
}

} // namespace

int main() {
    testBootstrapAndExactVector();
    testKeyMutationPreservesDirectAccess();
    testAdminSharedRules();
    testLegacyMigrationAndPresenceCas();
    testPreviousMigrationAndThirdSlot();
    testAtomicBootstrapWithPresence();
    testFaultsAndErase();
    testBleOnlyControllerClearReopensEnrollment();
    testWholeAccessResetAndReclaim();
    std::cout << "Device access store tests passed" << std::endl;
    return 0;
}
