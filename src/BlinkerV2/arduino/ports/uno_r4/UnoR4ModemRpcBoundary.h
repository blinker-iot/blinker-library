#ifndef BLINKER_UNO_R4_MODEM_RPC_BOUNDARY_H
#define BLINKER_UNO_R4_MODEM_RPC_BOUNDARY_H

#include "UnoR4ModemOwner.h"

// Defined once by the isolated Modem overlay; the UART overlay observes the
// SAME boundary/owner instead of manufacturing a second timeout or owner.
extern "C" bool blinker_r4_rpc_active();
extern "C" bool blinker_r4_rpc_expired();
extern "C" bool blinker_r4_rpc_faulted();
extern "C" void blinker_r4_rpc_fault();
extern "C" [[noreturn]] void blinker_r4_rpc_fail_stop();

namespace blinker {

// Internal cooperative timing/ownership only: no UART, allocator or recovery.
// The isolated Modem overlay owns one instance. It begins BEFORE command TX,
// keeps it across write_nowait -> passthrough -> final response, and finishes
// it only after a complete response. Expiry/fault requires safe UART stop
// before returning a possibly still-referenced caller buffer.
class RenesasUnoModemRpcBoundary {
public:
    RenesasUnoModemRpcBoundary(RenesasUnoModemOwner& owner, uint32_t rpcBudget)
        : owner_(owner), rpcBudget_(rpcBudget), started_(0U), budget_(0U), active_(false) {}

    bool begin(uint32_t requestedBudget) {
        // A second command would abandon the first header/response. Never
        // turn that protocol ambiguity into a harmless retry/renewed budget.
        if (active_) { fault(); return false; }
        if (owner_.quarantined_ || owner_.rpc_ != nullptr ||
            rpcBudget_ == 0U || rpcBudget_ > INT32_MAX ||
            requestedBudget == 0U || requestedBudget > INT32_MAX) return false;
        const uint32_t now = owner_.now_();
        uint32_t budget = requestedBudget < rpcBudget_ ? requestedBudget : rpcBudget_;
        if (owner_.active_ != nullptr) {
            const uint32_t elapsed = now - owner_.started_;
            if (elapsed >= owner_.budget_) {
                owner_.outcome_ = owner_.wrote_
                    ? RenesasUnoModemOwner::WriteUncertain
                    : RenesasUnoModemOwner::DeadlineBeforeWrite;
                return false; // Nothing new was sent; no fabricated late ACK.
            }
            const uint32_t remaining = owner_.budget_ - elapsed;
            if (remaining < budget) budget = remaining;
        }
        started_ = now;
        budget_ = budget;
        owner_.rpc_ = this;
        active_ = true;
        return true;
    }

    bool active() const { return active_; }
    bool faulted() const { return owner_.quarantined_; }
    bool expired() const {
        // Unsigned elapsed supports millis rollover; both admitted budgets are
        // <= INT32_MAX and the bounded wait must actually poll this predicate.
        return active_ && (owner_.quarantined_ || owner_.now_() - started_ >= budget_);
    }

    bool finish() {
        if (!active_) return false;
        // A response completed at/after the deadline is not confirmation.
        // Even a complete ERROR must finish inside this same RPC budget.
        if (expired()) { fault(); return false; }
        release();
        return true;
    }

    void fault() {
        owner_.rpcFault();
        release(); // Quarantine is sticky; this never authorizes another RPC.
    }

private:
    void release() {
        if (owner_.rpc_ == this) owner_.rpc_ = nullptr;
        active_ = false;
    }
    RenesasUnoModemOwner& owner_;
    uint32_t rpcBudget_, started_, budget_;
    bool active_;
    RenesasUnoModemRpcBoundary(const RenesasUnoModemRpcBoundary&) = delete;
    RenesasUnoModemRpcBoundary& operator=(const RenesasUnoModemRpcBoundary&) = delete;
};

} // namespace blinker
#endif
