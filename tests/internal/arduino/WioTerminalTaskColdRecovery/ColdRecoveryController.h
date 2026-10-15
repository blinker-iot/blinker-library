#ifndef BLINKER_TEST_WIO_TASK_COLD_CONTROLLER_H
#define BLINKER_TEST_WIO_TASK_COLD_CONTROLLER_H

#include <BlinkerV2/core/Result.h>
#include <stdint.h>

namespace wio_task_cold {
using blinker::ErrorCode;
using blinker::Result;

enum class Command : uint8_t { Ping, Observe, Status };
enum class State : uint8_t { Fresh, ObservedSnapshot, ObservedNotFound, Failed };
inline const char* stateName(State state) {
    switch (state) {
    case State::Fresh: return "Fresh";
    case State::ObservedSnapshot: return "ObservedSnapshot";
    case State::ObservedNotFound: return "ObservedNotFound";
    default: return "Failed";
    }
}
class ColdRecoveryReader {
public:
    virtual ~ColdRecoveryReader() {}
    virtual Result observe() = 0;
    virtual bool observed() const = 0;
    virtual bool hasSnapshot() const = 0;
};

// One explicit observation, including failed attempts. A successful read never
// confirms the earlier write or unlocks its unknown/pending host intent.
class ColdRecoveryController {
public:
    explicit ColdRecoveryController(ColdRecoveryReader& reader)
        : reader_(reader), state_(State::Fresh), consumed_(false), active_(false), invalidated_(false) {}
    State state() const { return state_; }
    void malformedInput() { invalidated_ = true; state_ = State::Failed; }
    Result execute(Command command) {
        if (command == Command::Ping || command == Command::Status) return Result::success();
        if (active_ || consumed_ || state_ != State::Fresh)
            return Result::failure(ErrorCode::StateConflict);
        ActiveCall active(active_);
        consumed_ = true; state_ = State::Failed; // consume BEFORE any media I/O
        const Result result = reader_.observe();
        if (invalidated_) return Result::failure(ErrorCode::StateConflict);
        if (reader_.observed()) {
            if (result && reader_.hasSnapshot()) { state_ = State::ObservedSnapshot; return result; }
            if (result.code() == ErrorCode::NotFound && !reader_.hasSnapshot()) {
                state_ = State::ObservedNotFound; return result;
            }
        }
        return result ? Result::failure(ErrorCode::StateConflict) : result;
    }
private:
    class ActiveCall {
    public:
        explicit ActiveCall(bool& active) : active_(active) { active_ = true; }
        ~ActiveCall() { active_ = false; }
    private:
        bool& active_;
    };
    ColdRecoveryReader& reader_;
    State state_;
    bool consumed_, active_, invalidated_;
    ColdRecoveryController(const ColdRecoveryController&) = delete;
    ColdRecoveryController& operator=(const ColdRecoveryController&) = delete;
};
} // namespace wio_task_cold
#endif
