#ifndef BLINKER_UNO_R4_MODEM_OWNER_H
#define BLINKER_UNO_R4_MODEM_OWNER_H

#include <BlinkerV2/core/Result.h>
#include <stdint.h>

namespace blinker {

// Cooperative, single-threaded ownership of the SDK's ONE modem. No UART
// flushing or timeout reset can prove recovery after an ambiguous response.
// There is deliberately no fault-unlock API: restart the connectivity module.
class RenesasUnoModemOwner {
public:
    typedef uint32_t (*Now)();
    typedef void (*Timeout)(uint32_t);
    enum Outcome : uint8_t { None, DeadlineBeforeWrite, ReadUncertain, WriteUncertain };

    RenesasUnoModemOwner(Now now, Timeout timeout, uint32_t baseline)
        : now_(now), timeout_(timeout), baseline_(baseline), namespace_(nullptr),
          active_(nullptr), started_(0U), budget_(0U), outcome_(None),
          quarantined_(false), wrote_(false) {}

    bool networkAllowed() const { return !quarantined_ && active_ == nullptr; }
    bool quarantined() const { return quarantined_; }
    Outcome outcome() const { return outcome_; }
    bool claimNamespace(const void* owner) {
        if (owner == nullptr || !networkAllowed() || (namespace_ != nullptr && namespace_ != owner)) return false;
        namespace_ = owner; return true;
    }
    void releaseNamespace(const void* owner) { if (namespace_ == owner) namespace_ = nullptr; }

    class Operation {
    public:
        Operation(RenesasUnoModemOwner& owner, const void* token, uint32_t budget)
            : owner_(owner), token_(token), result_(owner.acquire(token, budget)) {}
        ~Operation() { if (result_) owner_.release(token_); }
        const Result& result() const { return result_; }
    private:
        RenesasUnoModemOwner& owner_; const void* token_; Result result_;
        Operation(const Operation&) = delete;
        Operation& operator=(const Operation&) = delete;
    };

    Result prepare() {
        if (active_ == nullptr || quarantined_) return Result::failure(ErrorCode::StateConflict);
        const uint32_t elapsed = now_() - started_;
        if (elapsed >= budget_) {
            outcome_ = wrote_ ? WriteUncertain : DeadlineBeforeWrite;
            return Result::failure(ErrorCode::WouldBlock);
        }
        // SDK buf_read tests elapsed > timeout; reserve its one millisecond.
        timeout_(budget_ - elapsed - 1U);
        return Result::success();
    }
    void writing() { wrote_ = true; }
    void prepareWriteResponse() {
        // A PUT header has already promised a binary payload. Finish that
        // payload, but do not grant it a fresh whole-operation response wait.
        const uint32_t elapsed = now_() - started_;
        timeout_(elapsed < budget_ ? budget_ - elapsed - 1U : 0U);
    }
    Result complete(bool valid) {
        if (active_ == nullptr || quarantined_) return Result::failure(ErrorCode::StateConflict);
        if (!valid || now_() - started_ >= budget_) {
            quarantined_ = true;
            outcome_ = wrote_ ? WriteUncertain : ReadUncertain;
            return Result::failure(ErrorCode::WouldBlock);
        }
        return Result::success();
    }
    void baseline() { timeout_(baseline_); }

private:
    Result acquire(const void* token, uint32_t budget) {
        if (token == nullptr || budget == 0U || budget > INT32_MAX)
            return Result::failure(ErrorCode::InvalidArgument);
        if (quarantined_) return Result::failure(ErrorCode::StateConflict);
        if (active_ != nullptr || namespace_ != token) return Result::failure(ErrorCode::WouldBlock);
        active_ = token; started_ = now_(); budget_ = budget; wrote_ = false; outcome_ = None;
        return Result::success();
    }
    void release(const void* token) {
        if (active_ != token) return;
        baseline(); active_ = nullptr;
    }
    Now now_; Timeout timeout_; uint32_t baseline_;
    const void* namespace_; const void* active_;
    uint32_t started_, budget_; Outcome outcome_; bool quarantined_, wrote_;
};

RenesasUnoModemOwner& renesasUnoModemOwner();

} // namespace blinker
#endif
