#ifndef BLINKER_RUNTIME_GATEWAYCHILDCONNECTION_H
#define BLINKER_RUNTIME_GATEWAYCHILDCONNECTION_H

#include "GatewayChildSession.h"

namespace blinker {

// Connection mechanism only. Callers validate authority; composition decides
// when acquisition is allowed. Neither protocol coordinators nor RouteBridge
// may start a native session through a second path.
class IGatewayChildConnection {
public:
    virtual ~IGatewayChildConnection() {}
    // Success means secure now; WouldBlock never means a queued Action.
    virtual Result ensureConnected() = 0;
    // Retires this exact attempt, never a later owner's connection.
    virtual bool retire(uint32_t expectedAttemptId) = 0;
};

template <typename ChildSession>
class BasicGatewayChildConnection final : public IGatewayChildConnection {
public:
    explicit BasicGatewayChildConnection(ChildSession& child)
        : child_(child), closingAttempt_(0U), acquisitionAllowed_(false) {}

    void allowAcquisition(bool allowed) { acquisitionAllowed_ = allowed; }

    Result ensureConnected() override {
        const bool retiring = closingAttempt_ != 0U;
        poll();
        // Finishing an old close is not also permission to start a new attempt
        // inside a caller's secure-reuse check. Re-evaluate on the next call.
        if (retiring || !acquisitionAllowed_ || closingAttempt_ != 0U) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        if (child_.secure()) return Result::success();
        if (child_.state() == GatewayChildSessionState::Stopped) {
            const Result started = child_.start();
            if (!started) return started;
        }
        if (child_.state() == GatewayChildSessionState::Idle) {
            const Result connected = child_.connect();
            if (!connected && connected.code() != ErrorCode::AlreadyExists) return connected;
        }
        return Result::failure(child_.state() == GatewayChildSessionState::Fault
            ? ErrorCode::NotConnected : ErrorCode::WouldBlock);
    }

    bool retire(uint32_t expectedAttemptId) override {
        if (released(expectedAttemptId)) return true;
        // One physical session. A stale caller cannot replace an outstanding
        // close with another generation. poll never reconnects or creates work.
        if (closingAttempt_ != 0U && closingAttempt_ != expectedAttemptId) {
            poll();
            if (closingAttempt_ != 0U) return false;
        }
        closingAttempt_ = expectedAttemptId;
        poll();
        return closingAttempt_ == 0U;
    }

    void poll() {
        if (closingAttempt_ == 0U) return;
        if (!released(closingAttempt_)) {
            const GatewayChildSessionState state = child_.state();
            if (state == GatewayChildSessionState::Connecting ||
                state == GatewayChildSessionState::Authenticating ||
                state == GatewayChildSessionState::Secure) {
                (void)child_.disconnect();
            }
        }
        if (released(closingAttempt_)) closingAttempt_ = 0U;
    }

    bool closing() const { return closingAttempt_ != 0U && !released(closingAttempt_); }

private:
    bool released(uint32_t attempt) const {
        if (attempt == 0U || child_.attemptId() != attempt) return true;
        const GatewayChildSessionState state = child_.state();
        // ChildSession may report Idle only after native release. Fault with
        // the same attempt is unknown capacity, not permission to reacquire.
        return state == GatewayChildSessionState::Idle || state == GatewayChildSessionState::Stopped;
    }

    ChildSession& child_;
    uint32_t closingAttempt_;
    bool acquisitionAllowed_;
    BasicGatewayChildConnection(const BasicGatewayChildConnection&) = delete;
    BasicGatewayChildConnection& operator=(const BasicGatewayChildConnection&) = delete;
};

} // namespace blinker
#endif
