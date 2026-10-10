#ifndef BLINKER_TEST_DEVICE_ACCESS_MIGRATION_FIXTURE_H
#define BLINKER_TEST_DEVICE_ACCESS_MIGRATION_FIXTURE_H

#include <BlinkerV2Advanced.h>
#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/core/SecureMemory.h>

#include <string.h>

namespace blinker_test {

// Canonical schema-v2 fixture shared by hardware persistence gates. Each
// platform sketch owns storage lifecycle, reset and serial reporting only.
class DeviceAccessMigrationFixture {
public:
    static blinker::ControllerCredential sharedCredential() {
        return fixtureCredential(0x40U, 0x50U, 1U, sharedPermissions());
    }

    static blinker::ControllerCredential gatewayCredential() {
        return fixtureCredential(0x60U, 0x70U, 1U, sharedPermissions());
    }

    static bool matches(
        blinker::DeviceAccessStore& access,
        size_t expectedCount) {
        blinker::DeviceKey key;
        uint32_t epoch = 0U;
        size_t count = 0U;
        blinker::ControllerCredential loaded;
        const blinker::DeviceKey expectedKey = fixtureDeviceKey();
        const blinker::ControllerCredential admin = adminCredential();
        if (!access.load(key) ||
            !blinker::sameDeviceKey(key, expectedKey) ||
            !access.loadAccessEpoch(epoch) || epoch != accessEpoch() ||
            !access.count(count) || count != expectedCount ||
            !access.loadActive(
                admin.id(), admin.domain, accessEpoch(), loaded) ||
            !blinker::sameControllerCredential(admin, loaded)) {
            blinker::clearDeviceKey(key);
            blinker::clearControllerCredential(loaded);
            return false;
        }
        blinker::clearDeviceKey(key);
        blinker::clearControllerCredential(loaded);
        if (expectedCount == 1U) return true;

        const blinker::ControllerCredential expected[] = {
            sharedCredential(), gatewayCredential()};
        for (size_t index = 0U; index < 2U; ++index) {
            if (!access.loadActive(
                    expected[index].id(),
                    expected[index].domain,
                    accessEpoch(),
                    loaded) ||
                !blinker::sameControllerCredential(
                    expected[index], loaded)) {
                blinker::clearControllerCredential(loaded);
                return false;
            }
            blinker::clearControllerCredential(loaded);
        }
        return true;
    }

    static blinker::Result seedPreviousSchema(
        blinker::IAtomicBlobStore& storage) {
        uint8_t encoded[
            blinker::DeviceAccessStore::previousSerializedSize] = {};
        encoded[0] = 'B';
        encoded[1] = 'A';
        encoded[2] = 'S';
        encoded[3] = '2';
        encoded[4] = 2U;
        encoded[5] = 2U;
        encoded[6] = 1U;
        encoded[7] = 1U;
        writeU32(encoded + 8U, accessEpoch());

        const blinker::DeviceKey key = fixtureDeviceKey();
        memcpy(encoded + 12U, key.bytes, sizeof(key.bytes));
        const blinker::ControllerCredential admin = adminCredential();
        uint8_t* slot = encoded + 64U;
        slot[0] = 1U;
        slot[1] = static_cast<uint8_t>(admin.suite);
        slot[2] = static_cast<uint8_t>(admin.domain);
        writeU32(slot + 4U, admin.ownershipGeneration);
        writeU32(slot + 8U, admin.credentialVersion);
        writeU32(slot + 12U, admin.permissions);
        memcpy(slot + 16U, admin.controllerId, sizeof(admin.controllerId));
        memcpy(slot + 32U, admin.secret, sizeof(admin.secret));
        writeU32(
            encoded + sizeof(encoded) - 4U,
            blinker::computeCrc32(
                blinker::ByteView(encoded, sizeof(encoded) - 4U)));
        const blinker::Result result = storage.replace(
            blinker::ByteView(encoded, sizeof(encoded)));
        blinker::secureZero(
            blinker::MutableByteSpan(encoded, sizeof(encoded)));
        return result;
    }

private:
    static uint32_t accessEpoch() { return 5U; }

    static uint32_t sharedPermissions() {
        return blinker::kControllerPermissionObserve |
               blinker::kControllerPermissionControl;
    }

    static void writeU32(uint8_t* output, uint32_t value) {
        output[0] = static_cast<uint8_t>(value >> 24U);
        output[1] = static_cast<uint8_t>(value >> 16U);
        output[2] = static_cast<uint8_t>(value >> 8U);
        output[3] = static_cast<uint8_t>(value);
    }

    static blinker::DeviceKey fixtureDeviceKey() {
        blinker::DeviceKey value;
        for (size_t index = 0U; index < sizeof(value.bytes); ++index) {
            value.bytes[index] = static_cast<uint8_t>(index + 1U);
        }
        return value;
    }

    static blinker::ControllerCredential fixtureCredential(
        uint8_t idBase,
        uint8_t secretBase,
        uint32_t version,
        uint32_t permissions) {
        blinker::ControllerCredential value;
        value.suite =
            blinker::ControllerCredentialSuite::HmacSha256_32;
        value.domain =
            blinker::ControllerCredentialDomain::PlatformAccess;
        value.ownershipGeneration = accessEpoch();
        value.credentialVersion = version;
        value.permissions = permissions;
        for (size_t index = 0U;
             index < sizeof(value.controllerId);
             ++index) {
            value.controllerId[index] =
                static_cast<uint8_t>(idBase + index);
        }
        for (size_t index = 0U; index < sizeof(value.secret); ++index) {
            value.secret[index] =
                static_cast<uint8_t>(secretBase + index);
        }
        return value;
    }

    static blinker::ControllerCredential adminCredential() {
        return fixtureCredential(
            0x10U,
            0x20U,
            3U,
            blinker::kControllerPermissionAll);
    }
};

} // namespace blinker_test

#endif
