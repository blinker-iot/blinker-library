#ifndef BLINKER_RUNTIME_GATEWAYDIRECTYIELDLEASE_H
#define BLINKER_RUNTIME_GATEWAYDIRECTYIELDLEASE_H

#include "GatewayExecutionLease.h"

namespace blinker {

// Receiver-clock promise, not a second connection manager. Adapter validates
// MQTT/admission first; composition blocks all acquisition while holding and
// supplies physical quiescence only AFTER ordinary/management/permit/native
// owners have released. This class never disconnects someone else's owner.
class GatewayDirectYieldLease {
public:
    GatewayDirectYieldLease()
        : lease_(), control_(0U), activeControl_(0U), retiredAt_(0U), retiredMillis_(0U) {}
    Result begin(IRandom& random) { clear(); return lease_.begin(random); }
    Result begin(ByteView incarnation) { control_ = activeControl_ = 0U; return lease_.begin(incarnation); }
    void clear() { lease_.clear(); control_ = activeControl_ = retiredAt_ = retiredMillis_ = 0U; }
    // Retire wire authority without breaking a promise already observed by a
    // phone. MQTT reconnect is not evidence that the phone released its ACL.
    // Repeated retirement retains the original deadline, not now + duration.
    void retire(uint32_t now) {
        const uint32_t remaining = lease_.remainingMillis(now);
        const uint32_t elapsed = static_cast<uint32_t>(now - retiredAt_);
        const uint32_t old = elapsed < retiredMillis_ ? retiredMillis_ - elapsed : 0U;
        retiredAt_ = now;
        retiredMillis_ = remaining > old ? remaining : old;
        lease_.clear(); control_ = activeControl_ = 0U;
    }
    ByteView incarnation() const { return lease_.incarnation(); }

    Result probe(uint32_t control, ByteView incarnation, uint32_t now) {
        if (!lease_.matchesIncarnation(incarnation) || control == 0U || control < control_) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (control == control_) return Result::failure(lease_.pending(now)
            ? ErrorCode::AlreadyExists : ErrorCode::SequenceConflict);
        // No queue or replacement of a pending receiver challenge. A newer
        // Probe may retry within its ORIGINAL budget; old probes never reissue.
        if (lease_.pending(now)) return Result::failure(ErrorCode::WouldBlock);
        Result result = lease_.request(now);
        if (result) control_ = control;
        return result;
    }

    Result challenge(uint32_t now, gateway::GatewayDirectYieldView& out) {
        out = gateway::GatewayDirectYieldView();
        if (!lease_.pending(now)) return Result::failure(ErrorCode::WouldBlock);
        out.controlId = control_;
        out.demand.operation = gateway::GatewayExecutionOperation::Challenge;
        out.demand.incarnation = lease_.incarnation();
        out.demand.challengeId = lease_.challengeId();
        return Result::success();
    }

    Result apply(const gateway::GatewayDirectYieldView& value, uint32_t now) {
        const gateway::GatewayExecutionDemandView& d = value.demand;
        if (d.operation != gateway::GatewayExecutionOperation::Decision || value.controlId == 0U) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        const bool pending = value.controlId == control_ && d.challengeId == lease_.challengeId();
        const bool active = value.controlId == activeControl_ && d.challengeId == lease_.activeChallengeId();
        if (!pending && !(active && d.leaseMillis == 0U)) return Result::failure(ErrorCode::StateConflict);
        // Exact duplicate may request the same receipt but cannot move its
        // clock anchor, revive a cancelled promise or alter the granted time.
        if (active && lease_.matchesActive(d.incarnation, d.challengeId, d.leaseMillis, now)) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        Result result = lease_.apply(d.incarnation, d.challengeId, d.leaseMillis, now);
        if (result && d.leaseMillis != 0U) activeControl_ = value.controlId;
        return result;
    }

    bool holding(uint32_t now) {
        if (static_cast<uint32_t>(now - retiredAt_) >= retiredMillis_) retiredMillis_ = 0U;
        return lease_.wanted(now) || retiredMillis_ != 0U;
    }

    // quiescent includes the SDK's asynchronous closing barrier and every
    // other library radio owner. It is NOT !child.secure(), zero cloud demand,
    // an accepted disconnect request, or lack of a nearby advertisement.
    Result receipt(uint32_t now, bool quiescent, gateway::GatewayDirectYieldView& out) {
        out = gateway::GatewayDirectYieldView();
        if (!quiescent || !lease_.wanted(now)) return Result::failure(ErrorCode::WouldBlock);
        out.controlId = activeControl_;
        out.demand.operation = gateway::GatewayExecutionOperation::Yielded;
        out.demand.incarnation = lease_.incarnation();
        out.demand.challengeId = lease_.activeChallengeId();
        out.demand.leaseMillis = lease_.activeMillis();
        return Result::success();
    }

private:
    GatewayExecutionLease lease_;
    uint32_t control_, activeControl_;
    uint32_t retiredAt_, retiredMillis_;
};
static_assert(sizeof(GatewayDirectYieldLease) <= 56U, "Direct yield including retired promise must stay within 56 bytes");

} // namespace blinker
#endif
