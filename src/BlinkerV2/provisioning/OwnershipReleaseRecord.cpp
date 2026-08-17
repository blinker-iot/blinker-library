#include "OwnershipReleaseRecord.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool allZero(const uint8_t* value, size_t size) {
    uint8_t combined = 0U;
    for (size_t index = 0U; index < size; ++index) {
        combined = static_cast<uint8_t>(combined | value[index]);
    }
    return combined == 0U;
}

bool validOperationPolicy(
    OwnershipReleaseOperation operation,
    OwnershipReleaseNetworkPolicy policy) {
    const bool operationValid =
        operation == OwnershipReleaseOperation::Reset ||
        operation == OwnershipReleaseOperation::Transfer;
    const bool policyValid =
        policy == OwnershipReleaseNetworkPolicy::Preserve ||
        policy == OwnershipReleaseNetworkPolicy::Clear;
    return operationValid && policyValid &&
           (operation != OwnershipReleaseOperation::Transfer ||
            policy == OwnershipReleaseNetworkPolicy::Clear);
}

} // namespace

OwnershipReleaseRecord::OwnershipReleaseRecord()
    : state(OwnershipReleaseRecordState::Prepared),
      operation(OwnershipReleaseOperation::Reset),
      networkPolicy(OwnershipReleaseNetworkPolicy::Preserve),
      logicalDeviceIdLength(0U),
      currentGeneration(0U),
      nextGeneration(0U),
      requestId(),
      releaseNonce(),
      grantId(),
      grantDigest(),
      logicalDeviceId() {}

Result validateOwnershipReleaseRecord(const OwnershipReleaseRecord& record) {
    if ((record.state != OwnershipReleaseRecordState::Prepared &&
         record.state != OwnershipReleaseRecordState::GrantAccepted &&
         record.state != OwnershipReleaseRecordState::CommitAcknowledged) ||
        !validOperationPolicy(record.operation, record.networkPolicy) ||
        record.currentGeneration == 0U ||
        record.currentGeneration == UINT32_MAX ||
        allZero(record.requestId, sizeof(record.requestId)) ||
        allZero(record.releaseNonce, sizeof(record.releaseNonce)) ||
        !validateLogicalDeviceId(record.logicalId())) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const bool prepared =
        record.state == OwnershipReleaseRecordState::Prepared;
    if (prepared) {
        return record.nextGeneration == 0U &&
                       allZero(record.grantId, sizeof(record.grantId)) &&
                       allZero(record.grantDigest, sizeof(record.grantDigest))
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }
    return record.nextGeneration == record.currentGeneration + 1U &&
                   !allZero(record.grantId, sizeof(record.grantId)) &&
                   !allZero(record.grantDigest, sizeof(record.grantDigest))
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

bool sameOwnershipReleaseRecord(
    const OwnershipReleaseRecord& first,
    const OwnershipReleaseRecord& second) {
    return first.state == second.state &&
           first.operation == second.operation &&
           first.networkPolicy == second.networkPolicy &&
           first.logicalDeviceIdLength == second.logicalDeviceIdLength &&
           first.currentGeneration == second.currentGeneration &&
           first.nextGeneration == second.nextGeneration &&
           memcmp(first.requestId, second.requestId, sizeof(first.requestId)) == 0 &&
           memcmp(first.releaseNonce, second.releaseNonce, sizeof(first.releaseNonce)) == 0 &&
           memcmp(first.grantId, second.grantId, sizeof(first.grantId)) == 0 &&
           memcmp(first.grantDigest, second.grantDigest, sizeof(first.grantDigest)) == 0 &&
           memcmp(
               first.logicalDeviceId,
               second.logicalDeviceId,
               sizeof(first.logicalDeviceId)) == 0;
}

void clearOwnershipReleaseRecord(OwnershipReleaseRecord& record) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&record),
        sizeof(record)));
    record.state = OwnershipReleaseRecordState::Prepared;
    record.operation = OwnershipReleaseOperation::Reset;
    record.networkPolicy = OwnershipReleaseNetworkPolicy::Preserve;
}

} // namespace blinker
