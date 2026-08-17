#include "OwnershipRecord.h"

#include <string.h>

namespace blinker {

namespace {

bool allZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

} // namespace

OwnershipRecord::OwnershipRecord()
    : generation(0U),
      state(OwnershipState::Unclaimed),
      logicalDeviceIdLength(0U),
      grantFingerprint(),
      logicalDeviceId() {}

Result validateOwnershipRecord(const OwnershipRecord& record) {
    if (record.generation == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const ByteView fingerprint(
        record.grantFingerprint,
        sizeof(record.grantFingerprint));
    if (record.state == OwnershipState::Active) {
        if (allZero(fingerprint)) {
            return Result::failure(ErrorCode::InvalidEncoding);
        }
        return validateLogicalDeviceId(record.logicalId());
    }
    if (record.state != OwnershipState::Unclaimed ||
        record.logicalDeviceIdLength != 0U || !allZero(fingerprint) ||
        !allZero(ByteView(
            reinterpret_cast<const uint8_t*>(record.logicalDeviceId),
            sizeof(record.logicalDeviceId)))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

bool sameOwnershipRecord(
    const OwnershipRecord& first,
    const OwnershipRecord& second) {
    return first.logicalDeviceIdLength <= kLogicalDeviceIdCapacity &&
           second.logicalDeviceIdLength <= kLogicalDeviceIdCapacity &&
           first.generation == second.generation &&
           first.state == second.state &&
           first.logicalDeviceIdLength == second.logicalDeviceIdLength &&
           memcmp(
               first.grantFingerprint,
               second.grantFingerprint,
               sizeof(first.grantFingerprint)) == 0 &&
           memcmp(
               first.logicalDeviceId,
               second.logicalDeviceId,
               first.logicalDeviceIdLength) == 0;
}

void clearOwnershipRecord(OwnershipRecord& record) {
    record.generation = 0U;
    record.state = OwnershipState::Unclaimed;
    record.logicalDeviceIdLength = 0U;
    memset(record.grantFingerprint, 0, sizeof(record.grantFingerprint));
    memset(record.logicalDeviceId, 0, sizeof(record.logicalDeviceId));
}

Result makeOwnershipTombstone(
    uint32_t nextGeneration,
    OwnershipRecord& output) {
    if (nextGeneration == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    clearOwnershipRecord(output);
    output.generation = nextGeneration;
    return Result::success();
}

} // namespace blinker
