#include "ControllerCredentialStore.h"

#include <string.h>

#include "../core/Crc32.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const uint8_t kMagic[4] = {'B', 'C', 'T', '2'};
const uint8_t kSchemaVersion = 1U;
const uint8_t kActiveState = 1U;
const size_t kHeaderSize = 8U;
const size_t kSlotSize = 64U;
const size_t kCrcOffset = ControllerCredentialStore::serializedSize - 4U;

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

int compareControllerId(ByteView first, ByteView second) {
    return memcmp(first.data, second.data, kControllerIdSize);
}

bool validSelector(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration) {
    if (controllerId.data == nullptr ||
        controllerId.size != kControllerIdSize || allZero(controllerId)) {
        return false;
    }
    if (domain == ControllerCredentialDomain::Local) {
        return ownershipGeneration == 0U;
    }
    return domain == ControllerCredentialDomain::Ownership &&
           ownershipGeneration != 0U;
}

void clearCredentials(
    ControllerCredential* credentials,
    size_t count) {
    if (credentials == nullptr) return;
    for (size_t index = 0U; index < count; ++index) {
        clearControllerCredential(credentials[index]);
    }
}

size_t lowerBound(
    const ControllerCredential* credentials,
    size_t count,
    ByteView controllerId) {
    size_t index = 0U;
    while (index < count &&
           compareControllerId(credentials[index].id(), controllerId) < 0) {
        ++index;
    }
    return index;
}

} // namespace

Result ControllerCredentialStore::encode(
    const ControllerCredential* credentials,
    size_t count,
    MutableByteSpan output) {
    if (count > capacity || (count != 0U && credentials == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < serializedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    for (size_t index = 0U; index < count; ++index) {
        Result result = validateControllerCredential(credentials[index]);
        if (!result) return result;
        if (index != 0U && compareControllerId(
                credentials[index - 1U].id(),
                credentials[index].id()) >= 0) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
    }

    memset(output.data, 0, serializedSize);
    memcpy(output.data, kMagic, sizeof(kMagic));
    output.data[4] = kSchemaVersion;
    output.data[5] = static_cast<uint8_t>(capacity);
    output.data[6] = static_cast<uint8_t>(count);
    for (size_t index = 0U; index < count; ++index) {
        const ControllerCredential& credential = credentials[index];
        uint8_t* slot = output.data + kHeaderSize + index * kSlotSize;
        slot[0] = kActiveState;
        slot[1] = static_cast<uint8_t>(credential.suite);
        slot[2] = static_cast<uint8_t>(credential.domain);
        writeU32(slot + 4U, credential.ownershipGeneration);
        writeU32(slot + 8U, credential.credentialVersion);
        writeU32(slot + 12U, credential.permissions);
        memcpy(slot + 16U, credential.controllerId, kControllerIdSize);
        memcpy(
            slot + 32U,
            credential.secret,
            kControllerCredentialSecretSize);
    }
    writeU32(
        output.data + kCrcOffset,
        computeCrc32(ByteView(output.data, kCrcOffset)));
    return Result::success();
}

Result ControllerCredentialStore::decode(
    ByteView input,
    ControllerCredential* credentials,
    size_t& count) {
    count = 0U;
    if (credentials == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (input.data == nullptr || input.size != serializedSize ||
        memcmp(input.data, kMagic, sizeof(kMagic)) != 0 ||
        input.data[4] != kSchemaVersion ||
        input.data[5] != static_cast<uint8_t>(capacity) ||
        input.data[6] > capacity || input.data[7] != 0U ||
        readU32(input.data + kCrcOffset) !=
            computeCrc32(ByteView(input.data, kCrcOffset))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }

    const size_t decodedCount = input.data[6];
    for (size_t index = 0U; index < capacity; ++index) {
        const uint8_t* slot =
            input.data + kHeaderSize + index * kSlotSize;
        if (index >= decodedCount) {
            if (!allZero(ByteView(slot, kSlotSize))) {
                clearCredentials(credentials, capacity);
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            continue;
        }
        if (slot[0] != kActiveState || slot[3] != 0U) {
            clearCredentials(credentials, capacity);
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        ControllerCredential& credential = credentials[index];
        credential.suite =
            static_cast<ControllerCredentialSuite>(slot[1]);
        credential.domain =
            static_cast<ControllerCredentialDomain>(slot[2]);
        credential.ownershipGeneration = readU32(slot + 4U);
        credential.credentialVersion = readU32(slot + 8U);
        credential.permissions = readU32(slot + 12U);
        memcpy(credential.controllerId, slot + 16U, kControllerIdSize);
        memcpy(
            credential.secret,
            slot + 32U,
            kControllerCredentialSecretSize);
        Result result = validateControllerCredential(credential);
        if (!result || (index != 0U && compareControllerId(
                credentials[index - 1U].id(), credential.id()) >= 0)) {
            clearCredentials(credentials, capacity);
            return Result::failure(ErrorCode::InvalidEncoding);
        }
    }
    count = decodedCount;
    return Result::success();
}

Result ControllerCredentialStore::loadTable(
    ControllerCredential* credentials,
    size_t& count) {
    count = 0U;
    uint8_t encoded[serializedSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(encoded, sizeof(encoded)),
        written);
    if (result && written != sizeof(encoded)) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) {
        result = decode(ByteView(encoded, written), credentials, count);
    }
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    return result;
}

Result ControllerCredentialStore::replaceAndVerify(
    const ControllerCredential* credentials,
    size_t count) {
    uint8_t encoded[serializedSize] = {};
    Result result = encode(
        credentials,
        count,
        MutableByteSpan(encoded, sizeof(encoded)));
    if (result) result = storage_.replace(ByteView(encoded, sizeof(encoded)));
    secureZero(MutableByteSpan(encoded, sizeof(encoded)));
    if (!result) return result;

    ControllerCredential verified[capacity];
    size_t verifiedCount = 0U;
    result = loadTable(verified, verifiedCount);
    bool same = result && verifiedCount == count;
    for (size_t index = 0U; same && index < count; ++index) {
        same = sameControllerCredential(verified[index], credentials[index]);
    }
    clearCredentials(verified, capacity);
    return same ? Result::success()
                : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result ControllerCredentialStore::clearAndVerify() {
    Result result = storage_.clear();
    if (!result && result.code() != ErrorCode::NotFound) return result;

    ControllerCredential ignored[capacity];
    size_t ignoredCount = 0U;
    result = loadTable(ignored, ignoredCount);
    clearCredentials(ignored, capacity);
    return result.code() == ErrorCode::NotFound
               ? Result::success()
               : (result ? Result::failure(ErrorCode::InternalError) : result);
}

Result ControllerCredentialStore::count(size_t& output) {
    output = 0U;
    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    Result result = loadTable(credentials, loadedCount);
    clearCredentials(credentials, capacity);
    if (!result && result.code() == ErrorCode::NotFound) {
        return Result::success();
    }
    if (result) output = loadedCount;
    return result;
}

Result ControllerCredentialStore::loadAt(
    size_t index,
    ControllerCredential& output) {
    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    Result result = loadTable(credentials, loadedCount);
    if (result && index >= loadedCount) {
        result = Result::failure(ErrorCode::NotFound);
    }
    if (result) output = credentials[index];
    clearCredentials(credentials, capacity);
    return result;
}

Result ControllerCredentialStore::loadActive(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration,
    ControllerCredential& output) {
    if (!validSelector(controllerId, domain, ownershipGeneration)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    Result result = loadTable(credentials, loadedCount);
    if (result) {
        const size_t index = lowerBound(
            credentials,
            loadedCount,
            controllerId);
        if (index >= loadedCount || compareControllerId(
                credentials[index].id(), controllerId) != 0 ||
            credentials[index].domain != domain ||
            credentials[index].ownershipGeneration != ownershipGeneration) {
            result = Result::failure(ErrorCode::NotFound);
        } else {
            output = credentials[index];
        }
    }
    clearCredentials(credentials, capacity);
    return result;
}

Result ControllerCredentialStore::installVerified(
    const ControllerCredential& credential) {
    Result result = validateControllerCredential(credential);
    if (!result) return result;

    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    result = loadTable(credentials, loadedCount);
    if (!result && result.code() == ErrorCode::NotFound) {
        result = Result::success();
        loadedCount = 0U;
    }
    if (!result) {
        clearCredentials(credentials, capacity);
        return result;
    }

    const size_t index = lowerBound(
        credentials,
        loadedCount,
        credential.id());
    if (index < loadedCount && compareControllerId(
            credentials[index].id(), credential.id()) == 0) {
        const bool same = sameControllerCredential(
            credentials[index], credential);
        clearCredentials(credentials, capacity);
        return same ? Result::success()
                    : Result::failure(ErrorCode::StateConflict);
    }
    if (loadedCount == capacity) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    for (size_t move = loadedCount; move > index; --move) {
        credentials[move] = credentials[move - 1U];
    }
    credentials[index] = credential;
    ++loadedCount;
    result = replaceAndVerify(credentials, loadedCount);
    clearCredentials(credentials, capacity);
    return result;
}

Result ControllerCredentialStore::rotateVerified(
    uint32_t expectedCredentialVersion,
    const ControllerCredential& credential) {
    Result result = validateControllerCredential(credential);
    if (!result) return result;
    if (expectedCredentialVersion == 0U ||
        credential.credentialVersion <= expectedCredentialVersion) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    result = loadTable(credentials, loadedCount);
    if (!result) {
        clearCredentials(credentials, capacity);
        return result;
    }
    const size_t index = lowerBound(
        credentials,
        loadedCount,
        credential.id());
    if (index >= loadedCount || compareControllerId(
            credentials[index].id(), credential.id()) != 0) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::NotFound);
    }
    if (sameControllerCredential(credentials[index], credential)) {
        clearCredentials(credentials, capacity);
        return Result::success();
    }
    if (credentials[index].domain != credential.domain ||
        credentials[index].ownershipGeneration !=
            credential.ownershipGeneration) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::StateConflict);
    }
    if (credentials[index].credentialVersion !=
        expectedCredentialVersion) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    credentials[index] = credential;
    result = replaceAndVerify(credentials, loadedCount);
    clearCredentials(credentials, capacity);
    return result;
}

Result ControllerCredentialStore::revokeVerified(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t ownershipGeneration,
    uint32_t expectedCredentialVersion) {
    if (!validSelector(controllerId, domain, ownershipGeneration) ||
        expectedCredentialVersion == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    ControllerCredential credentials[capacity];
    size_t loadedCount = 0U;
    Result result = loadTable(credentials, loadedCount);
    if (!result && result.code() == ErrorCode::NotFound) {
        clearCredentials(credentials, capacity);
        return Result::success();
    }
    if (!result) {
        clearCredentials(credentials, capacity);
        return result;
    }
    const size_t index = lowerBound(
        credentials,
        loadedCount,
        controllerId);
    if (index >= loadedCount || compareControllerId(
            credentials[index].id(), controllerId) != 0) {
        clearCredentials(credentials, capacity);
        return Result::success();
    }
    if (credentials[index].domain != domain ||
        credentials[index].ownershipGeneration != ownershipGeneration) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::StateConflict);
    }
    if (credentials[index].credentialVersion !=
        expectedCredentialVersion) {
        clearCredentials(credentials, capacity);
        return Result::failure(ErrorCode::SequenceConflict);
    }
    clearControllerCredential(credentials[index]);
    for (size_t move = index + 1U; move < loadedCount; ++move) {
        credentials[move - 1U] = credentials[move];
    }
    clearControllerCredential(credentials[loadedCount - 1U]);
    --loadedCount;
    result = loadedCount == 0U
                 ? clearAndVerify()
                 : replaceAndVerify(credentials, loadedCount);
    clearCredentials(credentials, capacity);
    return result;
}

Result ControllerCredentialStore::clearAll() {
    return clearAndVerify();
}

} // namespace blinker
