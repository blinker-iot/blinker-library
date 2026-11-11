#ifndef BLINKER_TEST_WIO_TASK_PROBE_CONTROLLER_H
#define BLINKER_TEST_WIO_TASK_PROBE_CONTROLLER_H

#include <BlinkerV2/core/Result.h>
#include <stdint.h>

namespace wio_task_probe {
using blinker::ErrorCode;
using blinker::Result;

enum class Command : uint8_t {
    Ping, OpenCapture, RegionAttest, Read, Replace1, IdentityEnd, ClearFixture, Status
};
enum class State : uint8_t { Fresh, Captured, Admitted, Written, IdentityEnded, Cleared, Failed };
inline const char* stateName(State value) {
    switch (value) {
    case State::Fresh: return "Fresh";
    case State::Captured: return "Captured";
    case State::Admitted: return "Admitted";
    case State::Written: return "Written";
    case State::IdentityEnded: return "IdentityEnded";
    case State::Cleared: return "Cleared";
    default: return "Failed";
    }
}

// Only sequencing and admission. The port owns the real bank/store/codec;
// neither this controller nor its host fake is another task executor.
class ProbePort {
public:
    virtual ~ProbePort() {}
    virtual bool healthy() const = 0;
    virtual Result captureBaseline() = 0;
    virtual Result attestRegion() = 0;
    virtual Result readFixture(bool present) = 0;
    virtual Result replaceFixture() = 0;
    virtual Result endIdentity() = 0;
    virtual Result clearFixture() = 0;
};

class ProbeController {
public:
    explicit ProbeController(ProbePort& port)
        : port_(port), state_(State::Fresh), attempted_(0U), active_(false), invalidated_(false) {}
    State state() const { return state_; }
    uint8_t attemptedMask() const { return attempted_; }
    void malformedInput() { state_ = State::Failed; invalidated_ = true; }
    Result execute(Command command) {
        if (command == Command::Ping || command == Command::Status) return Result::success();
        if (active_) return rejected(); // native yield cannot nest raw I/O
        if (state_ == State::Failed) return Result::failure(ErrorCode::StateConflict);
        ActiveCall active(active_);
        if (!port_.healthy()) return failed(Result::failure(ErrorCode::StateConflict));
        Result result;
        switch (command) {
        case Command::OpenCapture:
            if (state_ != State::Fresh) return rejected();
            result = port_.captureBaseline();
            if (result && !invalidated_) state_ = State::Captured;
            break;
        case Command::RegionAttest:
            if (state_ != State::Captured) return rejected();
            result = port_.attestRegion();
            if (result && !invalidated_) state_ = State::Admitted;
            break;
        case Command::Read:
            if (state_ != State::Admitted && state_ != State::Written &&
                state_ != State::IdentityEnded && state_ != State::Cleared) return rejected();
            result = port_.readFixture(state_ == State::Written || state_ == State::IdentityEnded);
            if (result.code() == ErrorCode::NotFound &&
                (state_ == State::Admitted || state_ == State::Cleared) && !invalidated_) return result;
            break;
        case Command::Replace1:
            if (state_ != State::Admitted || (attempted_ & 1U) != 0U) return rejected();
            attempted_ |= 1U; state_ = State::Failed; // consume BEFORE any port I/O
            result = port_.replaceFixture();
            if (result && !invalidated_) state_ = State::Written;
            break;
        case Command::IdentityEnd:
            if (state_ != State::Written) return rejected();
            result = port_.endIdentity();
            if (result && !invalidated_) state_ = State::IdentityEnded;
            break;
        case Command::ClearFixture:
            if (state_ != State::IdentityEnded || (attempted_ & 2U) != 0U) return rejected();
            attempted_ |= 2U; state_ = State::Failed;
            result = port_.clearFixture();
            if (result && !invalidated_) state_ = State::Cleared;
            break;
        default: return rejected();
        }
        if (invalidated_) return failed(Result::failure(ErrorCode::StateConflict));
        return result ? result : failed(result);
    }
private:
    class ActiveCall {
    public:
        explicit ActiveCall(bool& active) : active_(active) { active_ = true; }
        ~ActiveCall() { active_ = false; }
    private:
        bool& active_;
    };
    static Result rejected() { return Result::failure(ErrorCode::StateConflict); }
    Result failed(Result result) { state_ = State::Failed; return result; }
    ProbePort& port_;
    State state_;
    uint8_t attempted_;
    bool active_, invalidated_;
    ProbeController(const ProbeController&) = delete;
    ProbeController& operator=(const ProbeController&) = delete;
};
} // namespace wio_task_probe
#endif
