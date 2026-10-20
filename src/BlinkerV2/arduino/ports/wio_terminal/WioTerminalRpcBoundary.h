#ifndef BLINKER_WIO_TERMINAL_RPC_BOUNDARY_H
#define BLINKER_WIO_TERMINAL_RPC_BOUNDARY_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

// Internal, isolated STA/Plain candidate only. Native definitions live once in
// the projected RPC init TU; ordinary Arduino builds do not include this file.
#ifdef __cplusplus
extern "C" {
#endif
uint32_t blinker_wio_rpc_enter_critical(void);
void blinker_wio_rpc_leave_critical(uint32_t saved);
uint32_t blinker_wio_rpc_now(void);
void blinker_wio_rpc_pause(void);
void blinker_wio_rpc_stop_uart(void);
bool blinker_wio_rpc_caller_allowed(void);
int blinker_wio_rpc_faulted(void);
int blinker_wio_rpc_finalize(uint32_t sequence, int status);
void blinker_wio_rpc_invalid_output(void);
#ifdef __cplusplus
}

namespace blinker {
namespace wio_rpc {

// A software observation budget, not an IRQ/allocator/scheduler latency SLA.
// It covers one generated RPC, not a multi-RPC TCP/HTTP operation.
static const uint32_t kBudgetMillis = 1000U;
enum class Fault { None, Deadline, Transport, Unsupported, Sequence, Codec, Ownership };
enum class Direction { Rx, Tx };
enum class Outcome { None, NotSent, Uncertain };

class Guard {
public:
    Guard() : saved_(blinker_wio_rpc_enter_critical()) {}
    ~Guard() { blinker_wio_rpc_leave_critical(saved_); }
private:
    uint32_t saved_;
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
};

class Boundary {
public:
    Boundary() : fault_(Fault::None), outcome_(Outcome::None), sequence_(0U),
        lastSequence_(0U), started_(0U), active_(false), finalized_(false),
        sent_(false), rx_(false), rxStarted_(false), tx_(false), rxAt_(0U), txAt_(0U) {}

    bool begin(uint32_t sequence, bool permittedCaller) {
        Guard guard;
        if (!readyLocked()) return false;
        if (!permittedCaller) return tripLocked(Fault::Unsupported);
        if (active_) return tripLocked(Fault::Ownership);
        if (!sequence || sequence <= lastSequence_) return tripLocked(Fault::Sequence);
        sequence_ = lastSequence_ = sequence;
        started_ = blinker_wio_rpc_now();
        active_ = true; finalized_ = false; sent_ = false; outcome_ = Outcome::None;
        return true;
    }

    bool ready() { Guard guard; return readyLocked(); }
    bool activeFor(uint32_t sequence) {
        Guard guard;
        return readyLocked() && active_ && sequence == sequence_;
    }
    bool faulted() const { Guard guard; return fault_ != Fault::None; }
    Fault reason() const { Guard guard; return fault_; }
    Outcome outcome() const { Guard guard; return outcome_; }
    void trip(Fault reason) { Guard guard; (void)tripLocked(reason); }

    // Called by generated shims AFTER decoding, BEFORE capturing err/releasing
    // the codec. release alone is too late to change the shim's return status.
    int finalize(uint32_t sequence, int status) {
        Guard guard;
        if (!active_ || sequence != sequence_) {
            (void)tripLocked(Fault::Ownership);
            return status ? status : 1;
        }
        if (status) (void)tripLocked(Fault::Codec);
        const bool valid = readyLocked();
        finalized_ = true;
        return valid ? status : (fault_ == Fault::Deadline ? 5 : (status ? status : 1));
    }

    void release(uint32_t sequence) {
        Guard guard;
        if (!sequence) return; // A denied call never owned the active request.
        if (!active_ || sequence != sequence_) { (void)tripLocked(Fault::Ownership); return; }
        if (!finalized_ && fault_ == Fault::None) (void)tripLocked(Fault::Codec);
        active_ = false;
        sequence_ = 0U;
        // Fault and lastSequence never reset; no live-runtime recovery API.
    }

    bool beginFrame(Direction direction) {
        Guard guard;
        if (!readyLocked()) return false;
        if (direction == Direction::Rx) {
            if (rx_) return tripLocked(Fault::Ownership);
            rx_ = true; rxStarted_ = false;
        } else {
            if (tx_) return tripLocked(Fault::Ownership);
            tx_ = true; txAt_ = blinker_wio_rpc_now();
        }
        return true;
    }
    bool checkFrame(Direction direction) {
        Guard guard;
        if (!readyLocked()) return false;
        if (direction == Direction::Rx) {
            if (!rx_) return tripLocked(Fault::Ownership);
            return !rxStarted_ || !expired(rxAt_) || tripLocked(Fault::Deadline);
        }
        if (!tx_) return tripLocked(Fault::Ownership);
        return !expired(txAt_) || tripLocked(Fault::Deadline);
    }
    bool progressRx() {
        Guard guard;
        if (!readyLocked() || !rx_) return false;
        if (!rxStarted_) { rxStarted_ = true; rxAt_ = blinker_wio_rpc_now(); }
        return !expired(rxAt_) || tripLocked(Fault::Deadline);
    }
    bool markSend() {
        Guard guard;
        if (!readyLocked()) return false;
        // Intent is recorded BEFORE DATA/ring writes. It is diagnostic only;
        // it must never be used to rewrite a persistent UNKNOWN or authorize replay.
        sent_ = true;
        return true;
    }
    void endFrame(Direction direction) {
        Guard guard;
        if (direction == Direction::Rx) rx_ = rxStarted_ = false;
        else tx_ = false;
    }
    bool checkWait(uint32_t started) {
        Guard guard;
        return readyLocked() && (!expired(started) || tripLocked(Fault::Deadline));
    }

private:
    bool expired(uint32_t started) const {
        return uint32_t(blinker_wio_rpc_now() - started) >= kBudgetMillis;
    }
    bool readyLocked() {
        if (fault_ != Fault::None) return false;
        return !active_ || !expired(started_) || tripLocked(Fault::Deadline);
    }
    bool tripLocked(Fault reason) {
        if (fault_ == Fault::None) {
            fault_ = reason == Fault::None ? Fault::Transport : reason;
            outcome_ = sent_ ? Outcome::Uncertain : Outcome::NotSent;
            // Native port is a fixed, no-wait GPIO/IRQ isolation primitive.
            // Never call reset/flush, wait for locks, allocate or release a
            // RequestContext here. Existing owners unwind their own buffers.
            blinker_wio_rpc_stop_uart();
        }
        return false;
    }
    Fault fault_;
    Outcome outcome_;
    uint32_t sequence_, lastSequence_, started_;
    bool active_, finalized_, sent_, rx_, rxStarted_, tx_;
    uint32_t rxAt_, txAt_;
    Boundary(const Boundary&) = delete;
    Boundary& operator=(const Boundary&) = delete;
};

extern Boundary boundary;

class FrameScope {
public:
    explicit FrameScope(Direction direction) : direction_(direction), valid_(boundary.beginFrame(direction)) {}
    ~FrameScope() { if (valid_) boundary.endFrame(direction_); }
    explicit operator bool() const { return valid_; }
private:
    Direction direction_;
    bool valid_;
    FrameScope(const FrameScope&) = delete;
    FrameScope& operator=(const FrameScope&) = delete;
};

template <typename Mutex>
class BoundedLock {
public:
    explicit BoundedLock(Mutex& mutex) : mutex_(mutex), locked_(false) {
        const uint32_t started = blinker_wio_rpc_now();
        while (boundary.checkWait(started)) {
            if (mutex_.tryLock()) {
                locked_ = true;
                if (!boundary.ready()) { mutex_.unlock(); locked_ = false; }
                return;
            }
            blinker_wio_rpc_pause();
        }
    }
    ~BoundedLock() { if (locked_) mutex_.unlock(); }
    explicit operator bool() const { return locked_; }
private:
    Mutex& mutex_;
    bool locked_;
    BoundedLock(const BoundedLock&) = delete;
    BoundedLock& operator=(const BoundedLock&) = delete;
};

} // namespace wio_rpc
} // namespace blinker
#endif // __cplusplus
#endif
