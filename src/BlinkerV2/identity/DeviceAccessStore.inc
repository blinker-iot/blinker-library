#include "DeviceAccessStore.h"

#include <limits.h>
#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kAccessStoreMagic[4] = {'B', 'A', 'S', '2'};
const uint8_t kLegacySchemaVersion = 1U;
const uint8_t kPreviousSchemaVersion = 2U;
const uint8_t kSchemaVersion = 3U;
const uint8_t kHasDeviceKey = 0x01U;
const uint8_t kHasPresenceKey = 0x02U;
const uint32_t kSharedPermissions =
    kControllerPermissionObserve | kControllerPermissionControl;
const size_t kEpochOffset = 8U;
const size_t kDeviceKeyOffset = 12U;
const size_t kLegacySlotsOffset = 44U;
const size_t kPresenceVersionOffset = 44U;
const size_t kPresenceKeyOffset = 48U;
const size_t kSlotsOffset = 64U;
const size_t kLegacyCrcOffset =
    DeviceAccessStore::legacySerializedSize - 4U;
const size_t kPreviousCrcOffset =
    DeviceAccessStore::previousSerializedSize - 4U;
const size_t kCrcOffset = DeviceAccessStore::serializedSize - 4U;

struct AccessState {
    bool hasDeviceKey;
    DeviceKey deviceKey;
    uint32_t accessEpoch;
    DevicePresenceKey presence;
    ControllerCredentialTable controllers;

    AccessState()
        : hasDeviceKey(false), deviceKey(), accessEpoch(0U),
          presence(), controllers() {}

    bool hasPresence() const { return presence.version != 0U; }

    void clearSecrets() {
        hasDeviceKey = false;
        clearDeviceKey(deviceKey);
        accessEpoch = 0U;
        clearDevicePresenceKey(presence);
        controllers.clear();
    }
};

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t readU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool allZero(ByteView value) {
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
    if (adminCount > 1U || sharedCount > 2U ||
        (table.count() != 0U && adminCount != 1U)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

Result validateState(const AccessState& state) {
    if (state.accessEpoch == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = state.hasDeviceKey
        ? validateDeviceKey(state.deviceKey)
        : Result::success();
    if (result && state.hasPresence()) {
        result = validateDevicePresenceKey(state.presence);
    }
    if (result) result = validateRoles(state.controllers, state.accessEpoch);
    return result;
}

Result encodeState(const AccessState& state, MutableByteSpan output) {
    if (output.data == nullptr ||
        output.size < DeviceAccessStore::serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    Result result = validateState(state);
    if (!result) return result;

    memset(output.data, 0, DeviceAccessStore::serializedSize);
    memcpy(output.data, kAccessStoreMagic, sizeof(kAccessStoreMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(
        ControllerCredentialTable::capacity);
    output.data[6] = static_cast<uint8_t>(state.controllers.count());
    output.data[7] = static_cast<uint8_t>(
        (state.hasDeviceKey ? kHasDeviceKey : 0U) |
        (state.hasPresence() ? kHasPresenceKey : 0U));
    writeU32(output.data + kEpochOffset, state.accessEpoch);
    if (state.hasDeviceKey) {
        memcpy(
            output.data + kDeviceKeyOffset,
            state.deviceKey.bytes,
            sizeof(state.deviceKey.bytes));
    }
    if (state.hasPresence()) {
        writeU32(output.data + kPresenceVersionOffset, state.presence.version);
        memcpy(
            output.data + kPresenceKeyOffset,
            state.presence.bytes,
            sizeof(state.presence.bytes));
    }
    result = state.controllers.encode(MutableByteSpan(
        output.data + kSlotsOffset,
        ControllerCredentialTable::encodedSize));
    if (result) {
        writeU32(
            output.data + kCrcOffset,
            computeCrc32(ByteView(output.data, kCrcOffset)));
    }
    return result;
}

Result decodeState(ByteView input, AccessState& state) {
    state.clearSecrets();
    const bool legacySize =
        input.size == DeviceAccessStore::legacySerializedSize;
    const bool previousSize =
        input.size == DeviceAccessStore::previousSerializedSize;
    const bool currentSize =
        input.size == DeviceAccessStore::serializedSize;
    if (input.data == nullptr ||
        (!legacySize && !previousSize && !currentSize) ||
        memcmp(input.data, kAccessStoreMagic, sizeof(kAccessStoreMagic)) != 0) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const bool legacy =
        legacySize && input.data[4] == kLegacySchemaVersion;
    const bool previous =
        previousSize && input.data[4] == kPreviousSchemaVersion;
    const bool current =
        currentSize && input.data[4] == kSchemaVersion;
    if (!legacy && !previous && !current) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const uint8_t allowedFlags = (previous || current)
        ? static_cast<uint8_t>(kHasDeviceKey | kHasPresenceKey)
        : kHasDeviceKey;
    const size_t slotsOffset = legacy ? kLegacySlotsOffset : kSlotsOffset;
    const size_t crcOffset = legacy
        ? kLegacyCrcOffset
        : (previous ? kPreviousCrcOffset : kCrcOffset);
    const size_t slotCapacity = current
        ? ControllerCredentialTable::capacity
        : 2U;
    if (input.data[5] != static_cast<uint8_t>(slotCapacity) ||
        input.data[6] > slotCapacity ||
        (input.data[7] & ~allowedFlags) != 0U ||
        readU32(input.data + crcOffset) !=
            computeCrc32(ByteView(input.data, crcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    state.hasDeviceKey = (input.data[7] & kHasDeviceKey) != 0U;
    state.accessEpoch = readU32(input.data + kEpochOffset);
    if (state.accessEpoch == 0U ||
        (!state.hasDeviceKey && !allZero(ByteView(
            input.data + kDeviceKeyOffset, kDeviceKeySize)))) {
        state.clearSecrets();
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (state.hasDeviceKey) {
        memcpy(
            state.deviceKey.bytes,
            input.data + kDeviceKeyOffset,
            sizeof(state.deviceKey.bytes));
        Result result = validateDeviceKey(state.deviceKey);
        if (!result) {
            state.clearSecrets();
            return result;
        }
    }

    if ((previous || current) &&
        (input.data[7] & kHasPresenceKey) != 0U) {
        state.presence.version = readU32(
            input.data + kPresenceVersionOffset);
        memcpy(
            state.presence.bytes,
            input.data + kPresenceKeyOffset,
            sizeof(state.presence.bytes));
        Result result = validateDevicePresenceKey(state.presence);
        if (!result) {
            state.clearSecrets();
            return Result::failure(ErrorCode::InvalidEncoding);
        }
    } else if ((previous || current) &&
               (readU32(input.data + kPresenceVersionOffset) != 0U ||
                !allZero(ByteView(
                    input.data + kPresenceKeyOffset,
                    kDevicePresenceKeySize)))) {
        state.clearSecrets();
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    Result result = state.controllers.decode(
        ByteView(
            input.data + slotsOffset,
            slotCapacity * ControllerCredentialTable::slotSize),
        input.data[6]);
    if (result.code() == ErrorCode::InvalidArgument) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) {
        result = validateRoles(state.controllers, state.accessEpoch);
    }
    if (!result) {
        state.clearSecrets();
        return result;
    }
    return Result::success();
}

Result loadState(IAtomicBlobStore& storage, AccessState& state) {
    uint8_t encoded[DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    Result result = storage.load(
        MutableByteSpan(encoded, sizeof(encoded)), written);
    if (result) {
        result = decodeState(ByteView(encoded, written), state);
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

bool sameState(const AccessState& first, const AccessState& second) {
    return first.hasDeviceKey == second.hasDeviceKey &&
           first.accessEpoch == second.accessEpoch &&
           (!first.hasDeviceKey || sameDeviceKey(
               first.deviceKey, second.deviceKey)) &&
           first.hasPresence() == second.hasPresence() &&
           (!first.hasPresence() || sameDevicePresenceKey(
               first.presence, second.presence)) &&
           sameTable(first.controllers, second.controllers);
}

Result replaceAndVerify(
    IAtomicBlobStore& storage,
    const AccessState& state) {
    uint8_t encoded[DeviceAccessStore::serializedSize] = {};
    Result result = encodeState(
        state,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    AccessState verified;
    result = loadState(storage, verified);
    const bool same = result && sameState(verified, state);
    verified.clearSecrets();
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

Result validateBootstrap(
    bool hasDeviceKey,
    const DeviceKey& deviceKey,
    uint32_t accessEpoch,
    const ControllerCredential& initialController,
    const DevicePresenceKey* presence) {
    Result result = hasDeviceKey
        ? validateDeviceKey(deviceKey)
        : Result::success();
    if (!result || accessEpoch == 0U ||
        initialController.domain !=
            ControllerCredentialDomain::PlatformAccess ||
        initialController.ownershipGeneration != accessEpoch ||
        !isAdmin(initialController)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    result = validateControllerCredential(initialController);
    if (result && presence != nullptr) {
        result = validateDevicePresenceKey(*presence);
    }
    return result;
}

Result bootstrapState(
    IAtomicBlobStore& storage,
    bool hasDeviceKey,
    const DeviceKey& deviceKey,
    uint32_t accessEpoch,
    const ControllerCredential& initialController,
    const DevicePresenceKey* presence) {
    Result result = validateBootstrap(
        hasDeviceKey,
        deviceKey,
        accessEpoch,
        initialController,
        presence);
    if (!result) return result;

    AccessState state;
    result = loadState(storage, state);
    if (result) {
        const bool same =
            state.hasDeviceKey == hasDeviceKey &&
            (!hasDeviceKey || sameDeviceKey(state.deviceKey, deviceKey)) &&
            state.accessEpoch == accessEpoch &&
            state.controllers.count() == 1U &&
            sameControllerCredential(
                state.controllers.at(0U), initialController) &&
            state.hasPresence() == (presence != nullptr) &&
            (presence == nullptr || sameDevicePresenceKey(
                state.presence, *presence));
        state.clearSecrets();
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    state.clearSecrets();
    if (result.code() != ErrorCode::NotFound) return result;

    state.hasDeviceKey = hasDeviceKey;
    if (hasDeviceKey) state.deviceKey = deviceKey;
    state.accessEpoch = accessEpoch;
    if (presence != nullptr) state.presence = *presence;
    bool changed = false;
    result = state.controllers.install(initialController, changed);
    if (result && changed) result = replaceAndVerify(storage, state);
    state.clearSecrets();
    return result;
}

} // namespace

Result DeviceAccessStore::load(DeviceKey& output) {
    AccessState state;
    Result result = loadState(storage_, state);
    if (result && !state.hasDeviceKey) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = state.deviceKey;
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::replace(const DeviceKey& key) {
    Result result = validateDeviceKey(key);
    if (!result) return result;
    AccessState state;
    result = loadState(storage_, state);
    if (result && state.hasDeviceKey &&
        sameDeviceKey(state.deviceKey, key)) {
        state.clearSecrets();
        return Result::success();
    }
    if (result) {
        state.hasDeviceKey = true;
        state.deviceKey = key;
        result = replaceAndVerify(storage_, state);
    }
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::clear() {
    AccessState state;
    Result result = loadState(storage_, state);
    if (result.code() == ErrorCode::NotFound) return Result::success();
    if (result && state.hasDeviceKey) {
        state.hasDeviceKey = false;
        clearDeviceKey(state.deviceKey);
        result = replaceAndVerify(storage_, state);
    }
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::loadAccessEpoch(uint32_t& output) {
    output = 0U;
    AccessState state;
    Result result = loadState(storage_, state);
    if (result) output = state.accessEpoch;
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::loadActive(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    ControllerCredential& output) {
    AccessState state;
    Result result = loadState(storage_, state);
    if (result && state.accessEpoch != accessEpoch) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) {
        result = state.controllers.loadActive(
            controllerId, domain, accessEpoch, output);
    }
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::loadAt(
    size_t index,
    ControllerCredential& output) {
    AccessState state;
    Result result = loadState(storage_, state);
    if (result && index >= state.controllers.count()) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = state.controllers.at(index);
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::count(size_t& output) {
    output = 0U;
    AccessState state;
    Result result = loadState(storage_, state);
    if (result) output = state.controllers.count();
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::installVerified(
    const ControllerCredential& credential) {
    AccessState state;
    Result result = loadState(storage_, state);
    bool changed = false;
    if (result && (credential.domain !=
                       ControllerCredentialDomain::PlatformAccess ||
                   credential.ownershipGeneration != state.accessEpoch)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && !isAdmin(credential) && !isShared(credential)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    for (size_t index = 0U;
         result && index < state.controllers.count();
         ++index) {
        const ControllerCredential& installed = state.controllers.at(index);
        const bool sameId = memcmp(
            installed.controllerId,
            credential.controllerId,
            kControllerIdSize) == 0;
        if (!sameId && isAdmin(installed) && isAdmin(credential)) {
            result = Result::failure(ErrorCode::StateConflict);
        }
    }
    if (result) result = state.controllers.install(credential, changed);
    if (result) {
        const Result roles = validateRoles(
            state.controllers, state.accessEpoch);
        if (!roles) result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && changed) result = replaceAndVerify(storage_, state);
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::rotateVerified(
    uint32_t expectedCredentialVersion,
    const ControllerCredential& credential) {
    AccessState state;
    Result result = loadState(storage_, state);
    ControllerCredential current;
    if (result) {
        result = state.controllers.loadActive(
            credential.id(),
            credential.domain,
            credential.ownershipGeneration,
            current);
    }
    if (result && (credential.ownershipGeneration != state.accessEpoch ||
                   current.permissions != credential.permissions)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    bool changed = false;
    if (result) {
        result = state.controllers.rotate(
            expectedCredentialVersion, credential, changed);
    }
    if (result && changed) result = replaceAndVerify(storage_, state);
    clearControllerCredential(current);
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::revokeVerified(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    uint32_t expectedCredentialVersion) {
    AccessState state;
    Result result = loadState(storage_, state);
    ControllerCredential current;
    if (result) {
        result = state.controllers.loadActive(
            controllerId, domain, accessEpoch, current);
    }
    if (result.code() == ErrorCode::NotFound) {
        bool changed = false;
        const Result absent = state.controllers.revoke(
            controllerId,
            domain,
            accessEpoch,
            expectedCredentialVersion,
            changed);
        clearControllerCredential(current);
        state.clearSecrets();
        return absent;
    }
    if (result && (state.accessEpoch != accessEpoch || isAdmin(current))) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    bool changed = false;
    if (result) {
        result = state.controllers.revoke(
            controllerId,
            domain,
            accessEpoch,
            expectedCredentialVersion,
            changed);
    }
    if (result && changed) result = replaceAndVerify(storage_, state);
    clearControllerCredential(current);
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::clearAll() {
    AccessState state;
    Result result = loadState(storage_, state);
    if (result.code() == ErrorCode::NotFound) return Result::success();
    if (result && !state.hasDeviceKey) {
        state.clearSecrets();
        return clearAndVerify(storage_);
    }
    if (result &&
        (state.controllers.count() != 0U || state.hasPresence())) {
        state.controllers.clear();
        clearDevicePresenceKey(state.presence);
        result = replaceAndVerify(storage_, state);
    }
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::bootstrap(
    const DeviceKey& deviceKey,
    uint32_t accessEpoch,
    const ControllerCredential& initialController) {
    return bootstrapState(
        storage_,
        true,
        deviceKey,
        accessEpoch,
        initialController,
        nullptr);
}

Result DeviceAccessStore::bootstrapAccess(
    uint32_t accessEpoch,
    const ControllerCredential& initialController) {
    return bootstrapState(
        storage_,
        false,
        DeviceKey(),
        accessEpoch,
        initialController,
        nullptr);
}

Result DeviceAccessStore::loadPresence(
    uint32_t accessEpoch,
    DevicePresenceKey& output) {
    clearDevicePresenceKey(output);
    AccessState state;
    Result result = loadState(storage_, state);
    if (result && state.accessEpoch != accessEpoch) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result && !state.hasPresence()) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = state.presence;
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::replacePresenceVerified(
    uint32_t accessEpoch,
    uint32_t expectedVersion,
    const DevicePresenceKey& value) {
    Result result = validateDevicePresenceKey(value);
    if (!result || accessEpoch == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    AccessState state;
    result = loadState(storage_, state);
    if (result && state.accessEpoch != accessEpoch) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && state.hasPresence() &&
        sameDevicePresenceKey(state.presence, value)) {
        const bool exactRetry =
            expectedVersion != UINT32_MAX &&
            value.version == expectedVersion + 1U;
        state.clearSecrets();
        return exactRetry
                   ? Result::success()
                   : Result::failure(ErrorCode::StateConflict);
    }
    const uint32_t currentVersion = state.hasPresence()
        ? state.presence.version
        : 0U;
    if (result &&
        (currentVersion != expectedVersion || expectedVersion == UINT32_MAX ||
         value.version != expectedVersion + 1U)) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result) {
        state.presence = value;
        result = replaceAndVerify(storage_, state);
    }
    state.clearSecrets();
    return result;
}

Result DeviceAccessStore::bootstrapWithPresence(
    const DeviceKey& deviceKey,
    uint32_t accessEpoch,
    const ControllerCredential& initialController,
    const DevicePresenceKey& presence) {
    return bootstrapState(
        storage_,
        true,
        deviceKey,
        accessEpoch,
        initialController,
        &presence);
}

Result DeviceAccessStore::bootstrapAccessWithPresence(
    uint32_t accessEpoch,
    const ControllerCredential& initialController,
    const DevicePresenceKey& presence) {
    return bootstrapState(
        storage_,
        false,
        DeviceKey(),
        accessEpoch,
        initialController,
        &presence);
}

Result DeviceAccessStore::eraseAccess() {
    return clearAndVerify(storage_);
}

} // namespace blinker
