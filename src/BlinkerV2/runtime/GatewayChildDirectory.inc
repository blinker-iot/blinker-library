#include "GatewayChildDirectory.h"
#include "../core/SecureMemory.h"
#include <string.h>

namespace blinker {

Result GatewayChildDirectory::select(StringView logicalId, ByteView instance, ByteView operation,
                                    bool initial, GatewayChildDirectoryEntry*& output) {
    output = nullptr;
    if (entries_ == nullptr || count_ == 0U) return Result::failure(ErrorCode::CapacityExceeded);
    GatewayChildDirectoryEntry* vacant = nullptr;
    for (size_t i = 0U; i < count_; ++i) {
        for (size_t previous = 0U; previous < i; ++previous) {
            if (&entries_[previous].records == &entries_[i].records) {
                return Result::failure(ErrorCode::StateConflict);
            }
        }
        const GatewayChildIdentity* identity = nullptr;
        const Result result = entries_[i].records.identity(identity);
        if (!result) return result; // Unreadable is neither empty nor a safe duplicate check.
        if (identity->state == GatewayChildRecordState::Empty) {
            if (vacant == nullptr) vacant = &entries_[i];
            continue;
        }
        const bool sameOperation = operation.size == sizeof(identity->operation) &&
            memcmp(operation.data, identity->operation, operation.size) == 0;
        if (identity->state != GatewayChildRecordState::Active) {
            if (sameOperation) return Result::failure(ErrorCode::StateConflict);
            continue; // Tombstone and orphan renewal journal both reserve capacity.
        }
        const bool sameId = logicalId.size == identity->logicalIdSize &&
            memcmp(logicalId.data, identity->logicalId, logicalId.size) == 0;
        const bool sameInstance = instance.size == sizeof(identity->instance) &&
            memcmp(instance.data, identity->instance, instance.size) == 0;
        if (!sameId && !sameInstance && !sameOperation) continue;
        if (!sameId || !sameInstance || output != nullptr || (!initial && !sameOperation) ||
            (initial && identity->hasRenewal && !sameOperation)) {
            return Result::failure(ErrorCode::StateConflict);
        }
        output = &entries_[i];
    }
    if (output == nullptr && initial) output = vacant;
    return output != nullptr ? Result::success() :
        Result::failure(initial ? ErrorCode::CapacityExceeded : ErrorCode::NotFound);
}

Result GatewayChildDirectory::process(const GatewayAccessDeliveryContext& context, ByteView exactEnvelope,
                                     MutableByteSpan ackOutput, size_t& ackSize) {
    ackSize = 0U;
    GatewayChildDirectoryEntry* selected = nullptr;
    Result result;
    // Selection is never authorization; retain the selected processor's
    // complete AEAD/context/time/commit checks.
    {
        gateway::GatewayCredentialRenewalEnvelopeView envelope;
        result = gateway::decodeGatewayCredentialRenewalEnvelope(exactEnvelope, envelope);
        if (result) {
            result = select(envelope.childLogicalDeviceId, envelope.childDeviceInstanceId,
                            envelope.topologyOperationId, false, selected);
        } else {
            gateway::GatewayAccessEnvelopeView initial;
            result = gateway::decodeGatewayAccessEnvelope(exactEnvelope, initial);
            if (result) result = select(initial.childLogicalDeviceId, initial.childDeviceInstanceId,
                                        initial.operationId, true, selected);
        }
    }
    if (result) result = selected->delivery.process(context, exactEnvelope, ackOutput, ackSize);
    if (!result) { secureZero(ackOutput); ackSize = 0U; }
    return result;
}

void GatewayChildDirectory::invalidate() {
    for (size_t i = 0U; entries_ != nullptr && i < count_; ++i) entries_[i].records.invalidate();
}

} // namespace blinker
