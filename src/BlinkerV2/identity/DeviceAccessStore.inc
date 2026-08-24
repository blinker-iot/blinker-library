#include "DeviceAccessStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kAccessStoreMagic[4] = {'B', 'A', 'S', '2'};
const uint8_t kAccessStoreSchemaVersion = 1U;
const uint8_t kHasDeviceKey = 0x01U;
const uint32_t kSharedPermissions =
    kControllerPermissionObserve | kControllerPermissionControl;
const size_t kEpochOffset = 8U;
const size_t kKeyOffset = 12U;
const size_t kSlotsOffset = 44U;
const size_t kAccessStoreCrcOffset = DeviceAccessStore::serializedSize - 4U;

void accessStoreWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t accessStoreReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool accessStoreAllZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

bool isAdmin(const ControllerCredential& credential) {
    return credential.permissions == kControllerPermissionAll;
}

bool isShared(const ControllerCredential& credential) {
    return credential.permissions == kSharedPermissions;
}

Result validateRoles(
    const ControllerCredentialTable& table,
    uint32_t accessEpoch) {
    size_t adminCount = 0U;
    size_t sharedCount = 0U;
    for (size_t index = 0U; index < table.count(); ++index) {
        const ControllerCredential& credential = table.at(index);
        if (credential.domain !=
                ControllerCredentialDomain::PlatformAccess ||
            credential.ownershipGeneration != accessEpoch) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        if (isAdmin(credential)) {
            ++adminCount;
        } else if (isShared(credential)) {
            ++sharedCount;
        } else {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
    }
    if (adminCount > 1U || sharedCount > 1U ||
        (table.count() != 0U && adminCount != 1U)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

Result encodeState(
    bool hasKey,
    const DeviceKey& key,
    uint32_t accessEpoch,
    const ControllerCredentialTable& table,
    MutableByteSpan output) {
    if (output.data == nullptr ||
        output.size < DeviceAccessStore::serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (accessEpoch == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = hasKey ? validateDeviceKey(key) : Result::success();
    if (result) result = validateRoles(table, accessEpoch);
    if (!result) return result;

    memset(output.data, 0, DeviceAccessStore::serializedSize);
    memcpy(output.data, kAccessStoreMagic, sizeof(kAccessStoreMagic));
    output.data[4] = kAccessStoreSchemaVersion;
    output.data[5] = static_cast<uint8_t>(
        ControllerCredentialTable::capacity);
    output.data[6] = static_cast<uint8_t>(table.count());
    output.data[7] = hasKey ? kHasDeviceKey : 0U;
    accessStoreWriteU32(output.data + kEpochOffset, accessEpoch);
    if (hasKey) {
        memcpy(output.data + kKeyOffset, key.bytes, sizeof(key.bytes));
    }
    result = table.encode(MutableByteSpan(
        output.data + kSlotsOffset,
        ControllerCredentialTable::encodedSize));
    if (result) {
    accessStoreWriteU32(
        output.data + kAccessStoreCrcOffset,
        computeCrc32(ByteView(output.data, kAccessStoreCrcOffset)));
    }
    return result;
}

Result decodeState(
    ByteView input,
    bool& hasKey,
    DeviceKey& key,
    uint32_t& accessEpoch,
    ControllerCredentialTable& table) {
    hasKey = false;
    clearDeviceKey(key);
    accessEpoch = 0U;
    if (input.data == nullptr ||
        input.size != DeviceAccessStore::serializedSize ||
        memcmp(input.data, kAccessStoreMagic, sizeof(kAccessStoreMagic)) != 0 ||
        input.data[4] != kAccessStoreSchemaVersion ||
        input.data[5] != static_cast<uint8_t>(
            ControllerCredentialTable::capacity) ||
        input.data[6] > ControllerCredentialTable::capacity ||
        (input.data[7] & ~kHasDeviceKey) != 0U ||
        accessStoreReadU32(input.data + kAccessStoreCrcOffset) !=
            computeCrc32(ByteView(input.data, kAccessStoreCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const uint32_t decodedEpoch = accessStoreReadU32(
        input.data + kEpochOffset);
    const bool decodedHasKey = (input.data[7] & kHasDeviceKey) != 0U;
    if (decodedEpoch == 0U ||
        (!decodedHasKey && !accessStoreAllZero(ByteView(
            input.data + kKeyOffset, kDeviceKeySize)))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    DeviceKey decodedKey;
    if (decodedHasKey) {
        memcpy(decodedKey.bytes, input.data + kKeyOffset, sizeof(decodedKey.bytes));
        Result result = validateDeviceKey(decodedKey);
        if (!result) {
            clearDeviceKey(decodedKey);
            return result;
        }
    }
    Result result = table.decode(
        ByteView(
            input.data + kSlotsOffset,
            ControllerCredentialTable::encodedSize),
        input.data[6]);
    if (result.code() == ErrorCode::InvalidArgument) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = validateRoles(table, decodedEpoch);
    if (result) {
        hasKey = decodedHasKey;
        key = decodedKey;
        accessEpoch = decodedEpoch;
    }
    clearDeviceKey(decodedKey);
    return result;
}

Result loadState(
    IAtomicBlobStore& storage,
    bool& hasKey,
    DeviceKey& key,
    uint32_t& accessEpoch,
    ControllerCredentialTable& table) {
    uint8_t encoded[DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    Result result = storage.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) {
        result = decodeState(
            ByteView(encoded, written), hasKey, key, accessEpoch, table);
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

bool sameTable(
    const ControllerCredentialTable& first,
    const ControllerCredentialTable& second) {
    if (first.count() != second.count()) return false;
    for (size_t index = 0U; index < first.count(); ++index) {
        if (!sameControllerCredential(first.at(index), second.at(index))) {
            return false;
        }
    }
    return true;
}

Result replaceAndVerify(
    IAtomicBlobStore& storage,
    bool hasKey,
    const DeviceKey& key,
    uint32_t accessEpoch,
    const ControllerCredentialTable& table) {
    uint8_t encoded[DeviceAccessStore::serializedSize] = {};
    Result result = encodeState(
        hasKey,
        key,
        accessEpoch,
        table,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    bool verifiedHasKey = false;
    DeviceKey verifiedKey;
    uint32_t verifiedEpoch = 0U;
    ControllerCredentialTable verifiedTable;
    result = loadState(
        storage,
        verifiedHasKey,
        verifiedKey,
        verifiedEpoch,
        verifiedTable);
    const bool same = result && verifiedHasKey == hasKey &&
        verifiedEpoch == accessEpoch &&
        (!hasKey || sameDeviceKey(verifiedKey, key)) &&
        sameTable(verifiedTable, table);
    clearDeviceKey(verifiedKey);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result clearAndVerify(IAtomicBlobStore& storage) {
    Result result = storage.clear();
    if (!result && result.code() != ErrorCode::NotFound) return result;
    uint8_t ignored[DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    result = storage.load(MutableByteSpan(ignored, sizeof(ignored)), written);
    secureZero(MutableByteSpan(ignored, sizeof(ignored)));
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result loadExisting(
    IAtomicBlobStore& storage,
    bool& hasKey,
    DeviceKey& key,
    uint32_t& accessEpoch,
    ControllerCredentialTable& table) {
    return loadState(storage, hasKey, key, accessEpoch, table);
}

} // namespace

Result DeviceAccessStore::load(DeviceKey& output) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    if (result && !hasKey) result = Result::failure(ErrorCode::NotFound);
    if (result) output = key;
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::replace(const DeviceKey& key) {
    Result result = validateDeviceKey(key);
    if (!result) return result;
    bool hasKey = false;
    DeviceKey current;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    result = loadExisting(
        storage_, hasKey, current, accessEpoch, table);
    if (result && hasKey && sameDeviceKey(current, key)) {
        clearDeviceKey(current);
        return Result::success();
    }
    if (result) {
        result = replaceAndVerify(
            storage_, true, key, accessEpoch, table);
    }
    clearDeviceKey(current);
    return result;
}

Result DeviceAccessStore::clear() {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    if (result.code() == ErrorCode::NotFound) {
        clearDeviceKey(key);
        return Result::success();
    }
    if (result && hasKey) {
        clearDeviceKey(key);
        result = replaceAndVerify(
            storage_, false, key, accessEpoch, table);
    }
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::loadAccessEpoch(uint32_t& output) {
    output = 0U;
    bool hasKey = false;
    DeviceKey key;
    ControllerCredentialTable table;
    Result result = loadExisting(storage_, hasKey, key, output, table);
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::loadActive(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    ControllerCredential& output) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t storedEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, storedEpoch, table);
    clearDeviceKey(key);
    if (result && storedEpoch != accessEpoch) {
        return Result::failure(ErrorCode::NotFound);
    }
    return result ? table.loadActive(
                        controllerId, domain, accessEpoch, output)
                  : result;
}

Result DeviceAccessStore::loadAt(
    size_t index,
    ControllerCredential& output) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    clearDeviceKey(key);
    if (result && index >= table.count()) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = table.at(index);
    return result;
}

Result DeviceAccessStore::count(size_t& output) {
    output = 0U;
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    clearDeviceKey(key);
    if (result) output = table.count();
    return result;
}

Result DeviceAccessStore::installVerified(
    const ControllerCredential& credential) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    bool changed = false;
    if (result && (credential.domain !=
                       ControllerCredentialDomain::PlatformAccess ||
                   credential.ownershipGeneration != accessEpoch)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && !isAdmin(credential) && !isShared(credential)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    for (size_t index = 0U; result && index < table.count(); ++index) {
        const ControllerCredential& installed = table.at(index);
        const bool sameId = memcmp(
            installed.controllerId,
            credential.controllerId,
            kControllerIdSize) == 0;
        if (!sameId && isAdmin(installed) == isAdmin(credential)) {
            result = Result::failure(ErrorCode::StateConflict);
        }
    }
    if (result) result = table.install(credential, changed);
    if (result) {
        const Result roles = validateRoles(table, accessEpoch);
        if (!roles) result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && changed) {
        result = replaceAndVerify(
            storage_, hasKey, key, accessEpoch, table);
    }
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::rotateVerified(
    uint32_t expectedCredentialVersion,
    const ControllerCredential& credential) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    ControllerCredential current;
    if (result) {
        result = table.loadActive(
            credential.id(),
            credential.domain,
            credential.ownershipGeneration,
            current);
    }
    if (result && (credential.ownershipGeneration != accessEpoch ||
                   current.permissions != credential.permissions)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    bool changed = false;
    if (result) {
        result = table.rotate(
            expectedCredentialVersion, credential, changed);
    }
    if (result && changed) {
        result = replaceAndVerify(
            storage_, hasKey, key, accessEpoch, table);
    }
    clearControllerCredential(current);
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::revokeVerified(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    uint32_t expectedCredentialVersion) {
    bool hasKey = false;
    DeviceKey key;
    uint32_t storedEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, storedEpoch, table);
    ControllerCredential current;
    if (result) {
        result = table.loadActive(
            controllerId, domain, accessEpoch, current);
    }
    if (result.code() == ErrorCode::NotFound) {
        bool changed = false;
        const Result absent = table.revoke(
            controllerId,
            domain,
            accessEpoch,
            expectedCredentialVersion,
            changed);
        clearControllerCredential(current);
        clearDeviceKey(key);
        return absent;
    }
    if (result && (storedEpoch != accessEpoch || isAdmin(current))) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    bool changed = false;
    if (result) {
        result = table.revoke(
            controllerId,
            domain,
            accessEpoch,
            expectedCredentialVersion,
            changed);
    }
    if (result && changed) {
        result = replaceAndVerify(
            storage_, hasKey, key, storedEpoch, table);
    }
    clearControllerCredential(current);
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::clearAll() {
    bool hasKey = false;
    DeviceKey key;
    uint32_t accessEpoch = 0U;
    ControllerCredentialTable table;
    Result result = loadExisting(
        storage_, hasKey, key, accessEpoch, table);
    if (result.code() == ErrorCode::NotFound) {
        clearDeviceKey(key);
        return Result::success();
    }
    if (result && !hasKey) {
        result = clearAndVerify(storage_);
    } else if (result && table.count() != 0U) {
        table.clear();
        result = replaceAndVerify(
            storage_, hasKey, key, accessEpoch, table);
    }
    clearDeviceKey(key);
    return result;
}

Result DeviceAccessStore::bootstrap(
    const DeviceKey& deviceKey,
    uint32_t accessEpoch,
    const ControllerCredential& initialController) {
    Result result = validateDeviceKey(deviceKey);
    if (!result || accessEpoch == 0U ||
        initialController.domain !=
            ControllerCredentialDomain::PlatformAccess ||
        initialController.ownershipGeneration != accessEpoch ||
        !isAdmin(initialController)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    result = validateControllerCredential(initialController);
    if (!result) return result;

    bool hasKey = false;
    DeviceKey currentKey;
    uint32_t currentEpoch = 0U;
    ControllerCredentialTable table;
    result = loadExisting(
        storage_, hasKey, currentKey, currentEpoch, table);
    if (result) {
        const bool same = hasKey && sameDeviceKey(currentKey, deviceKey) &&
            currentEpoch == accessEpoch && table.count() == 1U &&
            sameControllerCredential(table.at(0U), initialController);
        clearDeviceKey(currentKey);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearDeviceKey(currentKey);
    if (result.code() != ErrorCode::NotFound) return result;

    bool changed = false;
    result = table.install(initialController, changed);
    return result && changed
               ? replaceAndVerify(
                     storage_, true, deviceKey, accessEpoch, table)
               : result;
}

Result DeviceAccessStore::bootstrapAccess(
    uint32_t accessEpoch,
    const ControllerCredential& initialController) {
    if (accessEpoch == 0U ||
        initialController.domain !=
            ControllerCredentialDomain::PlatformAccess ||
        initialController.ownershipGeneration != accessEpoch ||
        !isAdmin(initialController)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = validateControllerCredential(initialController);
    if (!result) return result;

    bool hasKey = false;
    DeviceKey key;
    uint32_t currentEpoch = 0U;
    ControllerCredentialTable table;
    result = loadExisting(
        storage_, hasKey, key, currentEpoch, table);
    if (result) {
        const bool same = !hasKey && currentEpoch == accessEpoch &&
            table.count() == 1U &&
            sameControllerCredential(table.at(0U), initialController);
        clearDeviceKey(key);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    clearDeviceKey(key);
    if (result.code() != ErrorCode::NotFound) return result;

    bool changed = false;
    result = table.install(initialController, changed);
    return result && changed
               ? replaceAndVerify(
                     storage_, false, DeviceKey(), accessEpoch, table)
               : result;
}

Result DeviceAccessStore::eraseAccess() {
    return clearAndVerify(storage_);
}

} // namespace blinker
