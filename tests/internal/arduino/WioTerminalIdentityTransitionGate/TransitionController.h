#ifndef BLINKER_TEST_WIO_IDENTITY_TRANSITION_CONTROLLER_H
#define BLINKER_TEST_WIO_IDENTITY_TRANSITION_CONTROLLER_H

#include <BlinkerV2/core/Result.h>
#include <BlinkerV2/core/Span.h>

namespace wio_identity_transition {
using blinker::ByteView;
using blinker::ErrorCode;
using blinker::Result;
enum class Command : uint8_t { Ping, PrepareTemp, CaptureCurrent, RestoreOriginal, Status };
enum class State : uint8_t { Fresh, Prepared, Restored, Failed };
class TransitionPort {
public:
    virtual ~TransitionPort() {}
    virtual bool healthy() const = 0;
    virtual Result prepareTemporary() = 0;
    virtual Result captureCurrent() = 0;
    virtual Result restoreOriginal(ByteView original) = 0;
};

// Physical maintenance only. Pending intents and proof of a NEW whole-board
// power cycle belong to the host. Reconstructing this RAM object is NOT proof.
class TransitionController {
public:
    explicit TransitionController(TransitionPort& port)
        : port_(port), state_(State::Fresh), prepareUsed_(false), restoreUsed_(false),
          captureUsed_(false), active_(false), invalidated_(false) {}
    State state() const { return state_; }
    void malformedInput() { state_ = State::Failed; invalidated_ = true; }
    Result execute(Command command, ByteView original = ByteView()) {
        if (command == Command::Ping || command == Command::Status) return Result::success();
        if (active_ || state_ == State::Failed || state_ == State::Restored)
            return Result::failure(ErrorCode::StateConflict);
        ActiveCall active(active_);
        if (!port_.healthy()) { state_ = State::Failed; return Result::failure(ErrorCode::StateConflict); }
        Result result;
        State success = state_;
        switch (command) {
        case Command::PrepareTemp:
            if (prepareUsed_ || state_ != State::Fresh) return Result::failure(ErrorCode::StateConflict);
            prepareUsed_ = true; state_ = State::Failed;
            result = port_.prepareTemporary(); success = State::Prepared;
            break;
        case Command::CaptureCurrent:
            if (captureUsed_) return Result::failure(ErrorCode::StateConflict);
            captureUsed_ = true; state_ = State::Failed;
            result = port_.captureCurrent();
            break;
        case Command::RestoreOriginal:
            if (restoreUsed_) return Result::failure(ErrorCode::StateConflict);
            restoreUsed_ = true; state_ = State::Failed;
            result = port_.restoreOriginal(original); success = State::Restored;
            break;
        default: return Result::failure(ErrorCode::InvalidArgument);
        }
        if (invalidated_) return Result::failure(ErrorCode::StateConflict);
        if (result) state_ = success;
        return result;
    }
private:
    class ActiveCall {
    public:
        explicit ActiveCall(bool& active) : active_(active) { active_ = true; }
        ~ActiveCall() { active_ = false; }
    private:
        bool& active_;
    };
    TransitionPort& port_;
    State state_;
    bool prepareUsed_, restoreUsed_, captureUsed_, active_, invalidated_;
    TransitionController(const TransitionController&) = delete;
    TransitionController& operator=(const TransitionController&) = delete;
};
} // namespace wio_identity_transition
#endif
