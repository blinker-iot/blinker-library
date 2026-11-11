#ifndef BLINKER_RUNTIME_INITIAL_PROVISIONING_WINDOW_INTENT_H
#define BLINKER_RUNTIME_INITIAL_PROVISIONING_WINDOW_INTENT_H

#include "../core/Result.h"

namespace blinker {

// An internal, startup-only maintenance intent. No durable state, timers or
// native calls: the product still owns provisioning and confirmed shutdown.
class InitialProvisioningWindowIntent {
public:
    InitialProvisioningWindowIntent() : state_(State::None) {}

    Result request(bool unattached, bool stopped, bool stopPending) {
        if (!unattached || !stopped || stopPending || state_ == State::Consumed)
            return Result::failure(ErrorCode::StateConflict);
        if (state_ == State::Pending)
            return Result::failure(ErrorCode::AlreadyExists);
        state_ = State::Pending;
        return Result::success();
    }

    bool consume() {
        const bool requested = state_ == State::Pending;
        state_ = State::Consumed;
        return requested;
    }

private:
    enum class State : uint8_t { None, Pending, Consumed };
    State state_;
};

} // namespace blinker

#endif
