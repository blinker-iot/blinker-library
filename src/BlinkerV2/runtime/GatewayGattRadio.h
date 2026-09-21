#ifndef BLINKER_RUNTIME_GATEWAYGATTRADIO_H
#define BLINKER_RUNTIME_GATEWAYGATTRADIO_H

#include "GatewayChildConnection.h"
#include "GatewayManagementControlMux.h"
#include "../interface/IBleCentralPort.h"
#include "../interface/IClock.h"

namespace blinker {

// One boot barrier shared by all slots. RAM promises on the phone can outlive
// an MCU reboot; waiting one protocol maximum does not need Flash or wall time.
class GatewayGattBootGuard {
public:
    explicit GatewayGattBootGuard(IClock& clock) : clock_(clock), since_(0), started_(false), ready_(false) {}
    bool ready() {
        if (!started_) { since_ = clock_.monotonicMillis(); started_ = true; }
        if (!ready_ && static_cast<uint32_t>(clock_.monotonicMillis() - since_) >=
                gateway::kGatewayExecutionMaximumLeaseMillis) ready_ = true;
        return ready_;
    }
private:
    IClock& clock_;
    uint32_t since_;
    bool started_, ready_;
};

// Per-child GATT policy shared by ordinary work and management/permit
// acquisition. No SDK, timer task, owner registry, retry queue or wire authority.
// Other southbound transports reuse the route/owner contracts, not GATT states.
template <typename Child, typename Port, typename Bridge>
class BasicGatewayGattRadio final : public IGatewayManagementAdmission {
public:
    // Engineering values for the paired fairness gate, not wire constants or
    // a promised end-to-end connection latency. Management keeps its budget.
    enum : uint32_t {
        MaximumAcquisitionMillis = 16000U,
        MaximumOccupancyMillis = 10000U,
        MaximumCycleMillis = MaximumAcquisitionMillis + MaximumOccupancyMillis,
        MinimumOpportunityMillis = 6000U
    };
    BasicGatewayGattRadio(Child& child, Port& port,
        BasicGatewayChildConnection<Child>& connection, Bridge& bridge,
        GatewayManagementControlMux& management, GatewayGattBootGuard& boot, IClock& clock)
        : child_(child), port_(port), connection_(connection), bridge_(bridge),
          management_(management), boot_(boot), clock_(clock),
          acquisitionAt_(0U), occupiedAt_(0U), quietAt_(0U),
          online_(false), initiationAllowed_(false), permitHeld_(false), cycleActive_(false), occupied_(false), quiet_(false), fairnessDue_(false) {
        management_.setAdmission(*this);
    }

    // Called before northbound callbacks and again after lifecycle changes.
    // Admission itself is live: a Decision delivered during MQTT poll blocks
    // a later management command in that SAME poll, not one loop later.
    void prepare(bool online, bool mayInitiate = true) {
        (void)boot_.ready();
        online_ = online;
        initiationAllowed_ = mayInitiate;
        connection_.poll();
        updateFairness();
        connection_.allowAcquisition(online_ && !permitHeld_ &&
            nativeAcquisitionAvailable() && (initiationAllowed_ || !needsInitiation()) &&
            (management_.ownsChildSession() ||
                (!fairnessDue_ && !bridge_.directYieldHolding() && bridge_.executionWanted())));
    }

    bool allowNewManagementWork() override {
        updateFairness(); // The clock may cross the boundary inside MQTT poll.
        return online_ && !permitHeld_ && !fairnessDue_ && !bridge_.directYieldHolding() && nativeAcquisitionAvailable();
    }

    void poll(bool online, uint32_t budgetMicros) {
        prepare(online, initiationAllowed_);
        if (!online_) {
            bridge_.reset();
        }
        if (child_.state() != GatewayChildSessionState::Stopped) child_.poll(budgetMicros);
        connection_.poll();
        if (online_) {
            if (permitHeld_) (void)bridge_.pollControlPlane();
            else (void)bridge_.poll();
        }
        prepare(online_, initiationAllowed_);
        if (!permitHeld_ && !management_.ownsChildSession() && !management_.busy()) {
            if (online_ && !fairnessDue_ && !bridge_.directYieldHolding() && bridge_.executionWanted()) {
                // Bound the entire search, including retries. Native
                // Connecting starts occupancy; Secure is not a timer input.
                if (nativeAcquisitionAvailable() && (initiationAllowed_ || !needsInitiation())) noteAcquisition();
                (void)connection_.ensureConnected();
            } else {
                (void)release();
            }
        }
        updateFairness(); // Credit only observed logical AND native quiescence.
        if (online_) {
            const bool released = quiescent();
            bridge_.pollDirectYield(released);
        }
    }

    bool wantsInitiation() {
        return needsInitiation() && allowNewManagementWork() &&
            (bridge_.executionWanted() || management_.busy() || management_.ownsChildSession());
    }

    bool canLend() { return allowNewManagementWork() && !bridge_.executionWanted() && quiescent(); }
    bool directPromiseHolding() { return bridge_.directYieldHolding(); }
    bool holdForPermit() {
        if (!canLend()) return false;
        permitHeld_ = true;
        connection_.allowAcquisition(false);
        return true;
    }
    // Target selection is rechecked after borrowing, before native connect.
    bool permitTargetAllowed() {
        return online_ && boot_.ready() && (!fairnessDue_ || permitHeld_) && !bridge_.directYieldHolding() &&
            !bridge_.executionWanted() && !management_.busy() && !management_.ownsChildSession() &&
            !connection_.closing() && child_.state() == GatewayChildSessionState::Stopped;
    }
    void returnPermit() { permitHeld_ = false; }
    bool stopped() const {
        return !permitHeld_ && child_.state() == GatewayChildSessionState::Stopped &&
            port_.state() == BleCentralPortState::Stopped && !connection_.closing();
    }

    void stop() {
        prepare(false);
        bridge_.reset();
        if (!permitHeld_) (void)release(); // The adapter, not this child, drains its borrowed port.
    }

private:
    void noteAcquisition() {
        if (!cycleActive_) {
            cycleActive_ = true;
            acquisitionAt_ = clock_.monotonicMillis();
        }
        quiet_ = false;
    }

    void updateFairness() {
        const bool idle = quiescent();
        if (!idle) noteAcquisition();
        if (!cycleActive_) return;
        const uint32_t now = clock_.monotonicMillis();
        const uint32_t elapsed = static_cast<uint32_t>(now - acquisitionAt_);
        // Check before accepting a late native transition: it cannot extend
        // an expired search. Neither a failed attempt nor MQTT grants refunds.
        if (elapsed >= MaximumCycleMillis ||
            (!occupied_ && elapsed >= MaximumAcquisitionMillis)) fairnessDue_ = true;
        const BleCentralPortState native = port_.state();
        if (!occupied_ && (management_.busy() || management_.ownsChildSession() ||
                permitHeld_ || native == BleCentralPortState::Connecting ||
                native == BleCentralPortState::Discovering || native == BleCentralPortState::Ready ||
                native == BleCentralPortState::Disconnecting)) {
            // Charge initiation/discovery, not just authenticated use. An
            // already admitted management transaction retains its own budget;
            // chains of new transactions must still yield after this window.
            occupied_ = true;
            occupiedAt_ = now;
        }
        if (occupied_ && static_cast<uint32_t>(now - occupiedAt_) >= MaximumOccupancyMillis) fairnessDue_ = true;
        if (!idle) return;
        if (!quiet_) { quiet_ = true; quietAt_ = now; }
        if (static_cast<uint32_t>(now - quietAt_) >= MinimumOpportunityMillis) {
            // Natural idle and cloud-directed yielding count too. A short
            // gap, MQTT reset or new owner cannot refund the occupied budget.
            cycleActive_ = occupied_ = false;
            quiet_ = false;
            fairnessDue_ = false;
        }
    }

    bool needsInitiation() const {
        return child_.state() == GatewayChildSessionState::Stopped || child_.state() == GatewayChildSessionState::Idle;
    }

    bool nativeAcquisitionAvailable() {
        if (!boot_.ready() || connection_.closing()) return false;
        if (child_.state() == GatewayChildSessionState::Stopped) {
            return port_.state() == BleCentralPortState::Stopped;
        }
        if (child_.state() == GatewayChildSessionState::Idle) return port_.state() == BleCentralPortState::Idle;
        return child_.state() != GatewayChildSessionState::Fault &&
            child_.state() != GatewayChildSessionState::Disconnecting;
    }

    bool quiescent() const {
        return !management_.busy() && !management_.ownsChildSession() &&
            !permitHeld_ && !connection_.closing() &&
            child_.state() == GatewayChildSessionState::Stopped &&
            port_.state() == BleCentralPortState::Stopped;
    }

    Result release() {
        connection_.allowAcquisition(false);
        (void)connection_.retire(child_.attemptId());
        if (child_.state() == GatewayChildSessionState::Idle && port_.state() == BleCentralPortState::Idle) {
            child_.stop(); // Only after both logical and native close completed.
        }
        return child_.state() == GatewayChildSessionState::Stopped &&
                   port_.state() == BleCentralPortState::Stopped
            ? Result::success() : Result::failure(ErrorCode::WouldBlock);
    }

    Child& child_;
    Port& port_;
    BasicGatewayChildConnection<Child>& connection_;
    Bridge& bridge_;
    GatewayManagementControlMux& management_;
    GatewayGattBootGuard& boot_;
    IClock& clock_;
    uint32_t acquisitionAt_;
    uint32_t occupiedAt_;
    uint32_t quietAt_;
    bool online_;
    bool initiationAllowed_;
    bool permitHeld_;
    bool cycleActive_;
    bool occupied_;
    bool quiet_;
    bool fairnessDue_;
};
} // namespace blinker
#endif
