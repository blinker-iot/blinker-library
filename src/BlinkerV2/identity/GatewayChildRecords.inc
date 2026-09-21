#include "GatewayChildRecords.h"
#include <string.h>

namespace blinker {

Result GatewayChildRecords::identity(const GatewayChildIdentity*& output) {
    output = nullptr;
    if (access.protection() == StorageProtection::PlainFlash ||
        renewal.protection() == StorageProtection::PlainFlash) {
        invalidate();
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!current_) {
        const Result result = refresh();
        if (!result) return result;
    }
    output = &identity_;
    return Result::success();
}

Result GatewayChildRecords::refresh() {
    identity_ = GatewayChildIdentity();
    // Separate scopes keep large secret-bearing decoder workspaces from
    // overlapping. The canonical stores clear their own encoded buffers.
    Result result;
    {
        GatewayAccessRecord record;
        result = access.load(record);
        if (result) {
            identity_.state = GatewayChildRecordState::Active;
            identity_.logicalIdSize = record.childLogicalDeviceIdSize;
            memcpy(identity_.logicalId, record.childLogicalDeviceId, record.childLogicalDeviceIdSize);
            memcpy(identity_.instance, record.childDeviceInstanceId, sizeof(identity_.instance));
            memcpy(identity_.operation, record.operationId, sizeof(identity_.operation));
            identity_.accessEpoch = record.accessEpoch;
            identity_.credentialVersion = record.credentialVersion;
        }
        clearGatewayAccessRecord(record);
    }
    if (result.code() == ErrorCode::StateConflict) {
        GatewayRevocationTombstone tombstone;
        result = access.loadRevocation(tombstone);
        if (result) {
            identity_.state = GatewayChildRecordState::Revoked;
            memcpy(identity_.operation, tombstone.operationId, sizeof(identity_.operation));
        }
        clearGatewayRevocationTombstone(tombstone);
        // StateConflict followed by NotFound is not a verified empty slot.
        if (!result) { invalidate(); return result; }
    } else if (!result && result.code() != ErrorCode::NotFound) {
        invalidate();
        return result;
    }
    {
        GatewayCredentialRenewalRecord pending;
        result = renewal.load(pending);
        identity_.hasRenewal = result.ok();
        clearGatewayCredentialRenewalRecord(pending);
    }
    if (!result && result.code() != ErrorCode::NotFound) {
        invalidate();
        return result;
    }
    if (identity_.state == GatewayChildRecordState::Empty && identity_.hasRenewal) {
        identity_.state = GatewayChildRecordState::RenewalOnly;
    }
    current_ = true;
    return Result::success();
}

} // namespace blinker
