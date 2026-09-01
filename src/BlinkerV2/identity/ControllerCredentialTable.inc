#include "ControllerCredentialTable.h"

#include <string.h>

namespace blinker {

namespace {

const uint8_t kActiveState = 1U;

void credentialTableWriteU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t credentialTableReadU32(const uint8_t* input) {
    return (static_cast<uint32_t>(input[0]) << 24U) |
           (static_cast<uint32_t>(input[1]) << 16U) |
           (static_cast<uint32_t>(input[2]) << 8U) |
           static_cast<uint32_t>(input[3]);
}

bool credentialTableAllZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

int compareId(ByteView first, ByteView second) {
    return memcmp(first.data, second.data, kControllerIdSize);
}

bool validSelector(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch) {
    if (controllerId.data == nullptr ||
        controllerId.size != kControllerIdSize ||
        credentialTableAllZero(controllerId)) {
        return false;
    }
    if (domain == ControllerCredentialDomain::Local) {
        return accessEpoch == 0U;
    }
    return domain == ControllerCredentialDomain::PlatformAccess &&
           accessEpoch != 0U;
}

} // namespace

ControllerCredentialTable::ControllerCredentialTable()
    : credentials_(), count_(0U) {}

ControllerCredentialTable::~ControllerCredentialTable() {
    clear();
}

void ControllerCredentialTable::clear() {
    for (size_t index = 0U; index < capacity; ++index) {
        clearControllerCredential(credentials_[index]);
    }
    count_ = 0U;
}

size_t ControllerCredentialTable::lowerBound(
    ByteView controllerId) const {
    size_t index = 0U;
    while (index < count_ &&
           compareId(credentials_[index].id(), controllerId) < 0) {
        ++index;
    }
    return index;
}

Result ControllerCredentialTable::decode(
    ByteView slots,
    size_t count) {
    clear();
    const size_t slotCapacity = slots.size / slotSize;
    if (slots.data == nullptr || slots.size == 0U ||
        slots.size % slotSize != 0U || slotCapacity > capacity ||
        count > slotCapacity) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    for (size_t index = 0U; index < slotCapacity; ++index) {
        const uint8_t* slot = slots.data + index * slotSize;
        if (index >= count) {
            if (!credentialTableAllZero(ByteView(slot, slotSize))) {
                clear();
                return Result::failure(ErrorCode::InvalidEncoding);
            }
            continue;
        }
        if (slot[0] != kActiveState || slot[3] != 0U) {
            clear();
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        ControllerCredential& credential = credentials_[index];
        credential.suite =
            static_cast<ControllerCredentialSuite>(slot[1]);
        credential.domain =
            static_cast<ControllerCredentialDomain>(slot[2]);
        credential.ownershipGeneration = credentialTableReadU32(slot + 4U);
        credential.credentialVersion = credentialTableReadU32(slot + 8U);
        credential.permissions = credentialTableReadU32(slot + 12U);
        memcpy(credential.controllerId, slot + 16U, kControllerIdSize);
        memcpy(
            credential.secret,
            slot + 32U,
            kControllerCredentialSecretSize);
        Result result = validateControllerCredential(credential);
        if (!result || (index != 0U && compareId(
                credentials_[index - 1U].id(), credential.id()) >= 0)) {
            clear();
            return Result::failure(ErrorCode::InvalidEncoding);
        }
    }
    count_ = count;
    return Result::success();
}

Result ControllerCredentialTable::encode(
    MutableByteSpan slots) const {
    if (slots.data == nullptr || slots.size < encodedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memset(slots.data, 0, encodedSize);
    for (size_t index = 0U; index < count_; ++index) {
        const ControllerCredential& credential = credentials_[index];
        Result result = validateControllerCredential(credential);
        if (!result || (index != 0U && compareId(
                credentials_[index - 1U].id(), credential.id()) >= 0)) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        uint8_t* slot = slots.data + index * slotSize;
        slot[0] = kActiveState;
        slot[1] = static_cast<uint8_t>(credential.suite);
        slot[2] = static_cast<uint8_t>(credential.domain);
        credentialTableWriteU32(slot + 4U, credential.ownershipGeneration);
        credentialTableWriteU32(slot + 8U, credential.credentialVersion);
        credentialTableWriteU32(slot + 12U, credential.permissions);
        memcpy(slot + 16U, credential.controllerId, kControllerIdSize);
        memcpy(
            slot + 32U,
            credential.secret,
            kControllerCredentialSecretSize);
    }
    return Result::success();
}

Result ControllerCredentialTable::loadActive(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    ControllerCredential& output) const {
    if (!validSelector(controllerId, domain, accessEpoch)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t index = lowerBound(controllerId);
    if (index >= count_ || compareId(
            credentials_[index].id(), controllerId) != 0 ||
        credentials_[index].domain != domain ||
        credentials_[index].ownershipGeneration != accessEpoch) {
        return Result::failure(ErrorCode::NotFound);
    }
    output = credentials_[index];
    return Result::success();
}

Result ControllerCredentialTable::install(
    const ControllerCredential& credential,
    bool& changed) {
    changed = false;
    Result result = validateControllerCredential(credential);
    if (!result) return result;
    const size_t index = lowerBound(credential.id());
    if (index < count_ && compareId(
            credentials_[index].id(), credential.id()) == 0) {
        return sameControllerCredential(credentials_[index], credential)
                   ? Result::success()
                   : Result::failure(ErrorCode::StateConflict);
    }
    if (count_ == capacity) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    for (size_t move = count_; move > index; --move) {
        credentials_[move] = credentials_[move - 1U];
    }
    credentials_[index] = credential;
    ++count_;
    changed = true;
    return Result::success();
}

Result ControllerCredentialTable::rotate(
    uint32_t expectedCredentialVersion,
    const ControllerCredential& credential,
    bool& changed) {
    changed = false;
    Result result = validateControllerCredential(credential);
    if (!result) return result;
    if (expectedCredentialVersion == 0U ||
        credential.credentialVersion <= expectedCredentialVersion) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t index = lowerBound(credential.id());
    if (index >= count_ || compareId(
            credentials_[index].id(), credential.id()) != 0) {
        return Result::failure(ErrorCode::NotFound);
    }
    if (sameControllerCredential(credentials_[index], credential)) {
        return Result::success();
    }
    if (credentials_[index].domain != credential.domain ||
        credentials_[index].ownershipGeneration !=
            credential.ownershipGeneration) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (credentials_[index].credentialVersion !=
        expectedCredentialVersion) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    credentials_[index] = credential;
    changed = true;
    return Result::success();
}

Result ControllerCredentialTable::revoke(
    ByteView controllerId,
    ControllerCredentialDomain domain,
    uint32_t accessEpoch,
    uint32_t expectedCredentialVersion,
    bool& changed) {
    changed = false;
    if (!validSelector(controllerId, domain, accessEpoch) ||
        expectedCredentialVersion == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t index = lowerBound(controllerId);
    if (index >= count_ || compareId(
            credentials_[index].id(), controllerId) != 0) {
        return Result::success();
    }
    if (credentials_[index].domain != domain ||
        credentials_[index].ownershipGeneration != accessEpoch) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (credentials_[index].credentialVersion !=
        expectedCredentialVersion) {
        return Result::failure(ErrorCode::SequenceConflict);
    }
    clearControllerCredential(credentials_[index]);
    for (size_t move = index + 1U; move < count_; ++move) {
        credentials_[move - 1U] = credentials_[move];
    }
    clearControllerCredential(credentials_[count_ - 1U]);
    --count_;
    changed = true;
    return Result::success();
}

bool ControllerCredentialTable::hasManager(uint32_t accessEpoch) const {
    for (size_t index = 0U; index < count_; ++index) {
        const ControllerCredential& credential = credentials_[index];
        if (credential.domain ==
                ControllerCredentialDomain::PlatformAccess &&
            credential.ownershipGeneration == accessEpoch &&
            (credential.permissions &
             kControllerPermissionManageControllers) != 0U) {
            return true;
        }
    }
    return false;
}

} // namespace blinker
