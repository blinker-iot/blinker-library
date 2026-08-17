#include "OwnershipClaimRecord.h"

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool nonZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

} // namespace

OwnershipClaimRecord::OwnershipClaimRecord()
    : ownershipGeneration(0U), requestId(), claimNonce() {}

Result validateOwnershipClaimRecord(const OwnershipClaimRecord& record) {
    if (record.ownershipGeneration == 0U ||
        !nonZero(ByteView(record.requestId, sizeof(record.requestId))) ||
        !nonZero(ByteView(record.claimNonce, sizeof(record.claimNonce)))) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

bool sameOwnershipClaimRecord(
    const OwnershipClaimRecord& first,
    const OwnershipClaimRecord& second) {
    return first.ownershipGeneration == second.ownershipGeneration &&
           constantTimeEqual(
               ByteView(first.requestId, sizeof(first.requestId)),
               ByteView(second.requestId, sizeof(second.requestId))) &&
           constantTimeEqual(
               ByteView(first.claimNonce, sizeof(first.claimNonce)),
               ByteView(second.claimNonce, sizeof(second.claimNonce)));
}

void clearOwnershipClaimRecord(OwnershipClaimRecord& record) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&record),
        sizeof(record)));
}

} // namespace blinker
