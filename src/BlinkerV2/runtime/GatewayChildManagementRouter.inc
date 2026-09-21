#include "GatewayChildManagementRouter.h"
#include "../core/SecureMemory.h"
#include <string.h>

namespace blinker {
namespace gateway_management_routing_detail {

enum class Kind : uint8_t { Proof, Revocation, Renewal, Permit };
struct Target {
    Kind kind;
    StringView logicalId;
    ByteView instance, operation, controller, renewalOperation;
    uint32_t epoch = 0U, credentialVersion = 0U;
    bool terminal = false;
};
bool equal(ByteView a, const void* b, size_t size) {
    return a.data != nullptr && a.size == size && memcmp(a.data, b, size) == 0;
}
Result decode(ByteView bytes, Target& target) {
    {
        gateway::GatewayProofCommandView value;
        if (gateway::decodeGatewayProofCommand(bytes, value)) {
            target.kind = Kind::Proof; target.logicalId = value.childLogicalDeviceId;
            target.instance = value.childDeviceInstanceId; target.operation = value.operationId; target.epoch = value.accessEpoch;
            return Result::success();
        }
    }
    {
        gateway::GatewayRevocationCommandView value;
        if (gateway::decodeGatewayRevocationCommand(bytes, value)) {
            target.kind = Kind::Revocation; target.instance = value.childDeviceInstanceId;
            target.operation = value.operationId; target.epoch = value.accessEpoch; target.controller = value.controllerId;
            target.credentialVersion = value.gatewayCredentialVersion;
            target.terminal = value.phase == gateway::GatewayRevocationPhase::Finalize ||
                value.phase == gateway::GatewayRevocationPhase::Supersede;
            return Result::success();
        }
    }
    {
        gateway::GatewayCredentialRenewalCommandView value;
        if (gateway::decodeGatewayCredentialRenewalCommand(bytes, value)) {
            target.kind = Kind::Renewal; target.instance = value.childDeviceInstanceId;
            target.operation = value.topologyOperationId; target.renewalOperation = value.renewalOperationId;
            target.epoch = value.accessEpoch;
            return Result::success();
        }
    }
    gateway::GatewayPermitJoinCommandView permit;
    const Result result = gateway::decodeGatewayPermitJoinCommand(bytes, permit);
    if (result) target.kind = Kind::Permit;
    return result;
}

Result matches(const Target& target, GatewayChildRecords& records, bool& matched) {
    matched = false;
    // Terminal absence is a fresh fact about the WHOLE protected directory.
    // A stale/corrupt projection must never acknowledge removal of authority.
    if (target.terminal) records.invalidate();
    const GatewayChildIdentity* identity = nullptr;
    Result result = records.identity(identity);
    if (!result) return result;
    const bool sameOperation = equal(target.operation, identity->operation, sizeof(identity->operation));
    const bool sameInstance = equal(target.instance, identity->instance, sizeof(identity->instance));
    if (identity->hasRenewal) {
        GatewayCredentialRenewalRecord journal;
        result = records.renewal.load(journal);
        const bool sameTopology = result && equal(target.operation, journal.topologyOperationId, sizeof(journal.topologyOperationId));
        const bool sameBinding = result && target.epoch == journal.accessEpoch &&
            equal(target.instance, journal.childDeviceInstanceId, sizeof(journal.childDeviceInstanceId));
        if (result && target.terminal && (sameTopology || (sameBinding &&
                equal(target.controller, journal.controllerId, sizeof(journal.controllerId)) &&
                (target.credentialVersion == journal.expectedCredentialVersion || target.credentialVersion == journal.credentialVersion)))) {
            result = Result::failure(ErrorCode::WouldBlock); // Never leave recoverable old authority behind.
        } else if (result && target.kind == Kind::Renewal && sameTopology) {
            if (!sameBinding || !equal(target.renewalOperation, journal.renewalOperationId, sizeof(journal.renewalOperationId))) {
                result = Result::failure(ErrorCode::StateConflict);
            } else matched = true;
        }
        clearGatewayCredentialRenewalRecord(journal);
        if (!result) return result;
    }
    if (identity->state == GatewayChildRecordState::Active) {
        if (sameOperation) {
            if (!sameInstance || target.epoch != identity->accessEpoch ||
                (!target.logicalId.empty() && (target.logicalId.size != identity->logicalIdSize ||
                    memcmp(target.logicalId.data, identity->logicalId, target.logicalId.size) != 0))) {
                return Result::failure(ErrorCode::StateConflict);
            }
            matched = true;
        } else if (matched) {
            return Result::failure(ErrorCode::StateConflict); // Journal and active record disagree.
        } else if (target.terminal && sameInstance && target.epoch == identity->accessEpoch &&
                   target.credentialVersion == identity->credentialVersion) {
            GatewayAccessRecord record;
            result = records.access.load(record);
            const bool sameAuthority = result && equal(target.controller, record.controllerId, sizeof(record.controllerId));
            clearGatewayAccessRecord(record);
            if (!result) return result;
            if (sameAuthority) return Result::failure(ErrorCode::SequenceConflict); // A different operation may retain the same secret.
        }
    } else if (identity->state == GatewayChildRecordState::Revoked && sameOperation) {
        if (target.kind != Kind::Revocation) return Result::failure(ErrorCode::StateConflict);
        matched = true; // The coordinator still checks exact tombstone/receipt/topology.
    }
    return Result::success();
}
} // namespace gateway_management_routing_detail

Result GatewayChildManagementRouter::select(ByteView encoded, IGatewayManagementControl*& selected, bool& absentTerminal) {
    using namespace gateway_management_routing_detail;
    selected = nullptr; absentTerminal = false;
    Target target;
    Result result = decode(encoded, target);
    if (!result) return result;
    if (target.kind == Kind::Permit) { selected = &permit_; return Result::success(); }
    if (directory_.size() == 0U) return Result::failure(ErrorCode::NotConfigured);
    GatewayChildDirectoryEntry* owner = nullptr;
    for (size_t i = 0U; i < directory_.size(); ++i) {
        GatewayChildDirectoryEntry* entry = directory_.entry(i);
        if (entry == nullptr) return Result::failure(ErrorCode::NotConfigured);
        for (size_t previous = 0U; previous < i; ++previous) {
            if (&directory_.entry(previous)->records == &entry->records ||
                (entry->control != nullptr && directory_.entry(previous)->control == entry->control)) {
                return Result::failure(ErrorCode::StateConflict);
            }
        }
        bool matched = false;
        result = matches(target, entry->records, matched);
        if (!result) return result;
        if (matched) {
            if (owner != nullptr) return Result::failure(ErrorCode::StateConflict);
            owner = entry;
        }
    }
    if (owner != nullptr) {
        selected = owner->control;
        return selected != nullptr ? Result::success() : Result::failure(ErrorCode::WouldBlock);
    }
    absentTerminal = target.terminal;
    return absentTerminal ? Result::success() : Result::failure(ErrorCode::NotFound);
}

Result GatewayChildManagementRouter::handleCommand(ByteView encoded) {
    if (terminalSize_ != 0U) return Result::failure(ErrorCode::WouldBlock);
    IGatewayManagementControl* selected = nullptr; bool absentTerminal = false;
    Result result = select(encoded, selected, absentTerminal);
    if (!result) return result; // Unknown/ambiguous commands never reset the current owner.
    if (selected == &permit_ && permitAdmission_ != nullptr && !permitAdmission_->allowNewManagementWork() &&
        !(active_ == selected && selected->continuesCommand(encoded))) return Result::failure(ErrorCode::WouldBlock);
    if (selected != &permit_ && permit_.ownsChildSession()) return Result::failure(ErrorCode::WouldBlock);
    if (active_ != nullptr && active_ != selected) {
        if (active_->busy() || active_->ownsChildSession() || !active_->pendingResult().empty()) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        active_->reset();
        if (active_->busy() || active_->ownsChildSession()) return Result::failure(ErrorCode::WouldBlock);
        active_ = nullptr;
    }
    if (absentTerminal) return finishAbsent(encoded);
    active_ = selected;
    return active_->handleCommand(encoded);
}

Result GatewayChildManagementRouter::finishAbsent(ByteView encoded) {
    gateway::GatewayRevocationCommandView command;
    Result result = gateway::decodeGatewayRevocationCommand(encoded, command);
    if (!result) return result;
    gateway::GatewayRevocationResultView reply;
    reply.operationId = command.operationId; reply.topologyVersion = command.topologyVersion;
    reply.phase = command.phase; reply.receiptDigest = command.receiptDigest;
    reply.status = command.phase == gateway::GatewayRevocationPhase::Finalize ?
        gateway::GatewayRevocationStatus::Finalized : gateway::GatewayRevocationStatus::Retired;
    result = gateway::encodeGatewayRevocationResult(reply, MutableByteSpan(terminal_, sizeof(terminal_)), terminalSize_);
    if (!result) { secureZero(MutableByteSpan(terminal_, sizeof(terminal_))); terminalSize_ = 0U; }
    return result;
}

bool GatewayChildManagementRouter::ownsChildSession() const {
    return permit_.ownsChildSession() || (active_ != nullptr && active_->ownsChildSession());
}
void GatewayChildManagementRouter::poll() {
    if (active_ != &permit_ && permit_.ownsChildSession()) permit_.poll();
    if (active_ != nullptr) active_->poll();
}
void GatewayChildManagementRouter::markResultPublished() {
    if (terminalSize_ != 0U) { secureZero(MutableByteSpan(terminal_, terminalSize_)); terminalSize_ = 0U; }
    else if (active_ != nullptr) active_->markResultPublished();
}
void GatewayChildManagementRouter::reset() {
    // Product/MQTT authorization retirement, not a per-child cancellation.
    // Keep the old owner pointer while its asynchronous close is draining.
    for (size_t i = 0U; i < directory_.size(); ++i) {
        auto* entry = directory_.entry(i);
        if (entry != nullptr && entry->control != nullptr) entry->control->reset();
    }
    permit_.reset(); secureZero(MutableByteSpan(terminal_, sizeof(terminal_))); terminalSize_ = 0U;
}

} // namespace blinker
