#ifndef BLINKER_CORE_SKETCHTIMER_H
#define BLINKER_CORE_SKETCHTIMER_H

#include <stddef.h>
#include <stdint.h>

namespace blinker {

template <size_t Capacity> class SketchTimer;

// Volatile cancellation token, not a task identity or security capability.
// valid() means a token was issued; active() checks whether it is still live.
// Tokens must not outlive their timer object or be reused after its destruction.
class TimerHandle {
public:
    TimerHandle() : owner_(nullptr), generation_(0U), slot_(0U) {}
    bool valid() const { return owner_ != nullptr && generation_ != 0U; }
private:
    template <size_t Capacity> friend class SketchTimer;
    TimerHandle(const void* owner, uint32_t generation, uint8_t slot)
        : owner_(owner), generation_(generation), slot_(slot) {}
    const void* owner_;
    uint32_t generation_;
    uint8_t slot_;
};

// Cooperative, fixed-capacity Sketch callbacks. Owns no clock, heap, thread,
// network, UTC or persistence. Call run() regularly; callbacks must not block.
// Poll gaps must remain below 2^31 ms. Clear after known clock reset/unmeasured
// sleep: a uint32 tick cannot identify an unobserved full wrap.
template <size_t Capacity>
class SketchTimer {
public:
    typedef uint32_t (*Clock)();
    typedef void (*Callback)();
    typedef void (*ContextCallback)(void*);

    explicit SketchTimer(Clock clock)
        : slots_(), clock_(clock), generation_(0U), running_(false) {
        static_assert(Capacity > 0U && Capacity <= 255U, "SketchTimer capacity must be 1..255");
#if UINTPTR_MAX <= UINT32_MAX
        static_assert(sizeof(Slot) <= 32U, "SketchTimer slot exceeds its 32-bit resource gate");
#endif
    }

    TimerHandle setInterval(uint32_t milliseconds, Callback callback) {
        return schedule(milliseconds, callback, nullptr, nullptr, true);
    }
    TimerHandle setTimeout(uint32_t milliseconds, Callback callback) {
        return schedule(milliseconds, callback, nullptr, nullptr, false);
    }
    TimerHandle setInterval(uint32_t milliseconds, ContextCallback callback, void* context) {
        return schedule(milliseconds, nullptr, callback, context, true);
    }
    TimerHandle setTimeout(uint32_t milliseconds, ContextCallback callback, void* context) {
        return schedule(milliseconds, nullptr, callback, context, false);
    }

    bool active(const TimerHandle& handle) const {
        return handle.owner_ == this && handle.slot_ < Capacity &&
               handle.generation_ != 0U && slots_[handle.slot_].active &&
               slots_[handle.slot_].generation == handle.generation_;
    }
    bool cancel(const TimerHandle& handle) {
        if (!active(handle)) return false;
        slots_[handle.slot_].active = false;
        return true;
    }
    void clear() {
        for (size_t i = 0U; i < Capacity; ++i) slots_[i].active = false;
        // Do not rewind generation: old handles must never cancel replacements.
    }
    size_t size() const {
        size_t count = 0U;
        for (size_t i = 0U; i < Capacity; ++i) if (slots_[i].active) ++count;
        return count;
    }

    size_t run() {
        if (clock_ == nullptr || running_) return 0U;
        running_ = true;
        const uint32_t currentMillis = clock_();
        // Generations never wrap: one frontier defers registrations made by
        // callbacks without a per-slot snapshot or Capacity-sized stack array.
        const uint32_t eligibleGeneration = generation_;
        size_t called = 0U;
        for (size_t i = 0U; i < Capacity; ++i) {
            Slot& slot = slots_[i];
            if (!slot.active || slot.generation > eligibleGeneration ||
                currentMillis - slot.deadline > 0x7FFFFFFFU) continue;
            const Function callback = slot.callback;
            void* const context = slot.context;
            const bool contextual = slot.contextual;
            if (slot.interval == 0U) slot.active = false; // Release before callback.
            else {
                // Skip missed occurrences; retain phase, never run a catch-up burst.
                const uint32_t periods = (currentMillis - slot.deadline) / slot.interval + 1U;
                slot.deadline += periods * slot.interval;
            }
            ++called;
            if (contextual) callback.contextual(context);
            else callback.simple();
        }
        running_ = false;
        return called;
    }

private:
    union Function {
        Callback simple;
        ContextCallback contextual;
        Function() : simple(nullptr) {}
    };
    struct Slot {
        Function callback;
        void* context;
        uint32_t deadline;
        uint32_t interval;
        uint32_t generation;
        bool active;
        bool contextual;
        Slot() : callback(), context(nullptr), deadline(0U), interval(0U),
                 generation(0U), active(false), contextual(false) {}
    };
    TimerHandle schedule(uint32_t milliseconds, Callback simple,
                         ContextCallback contextual, void* context, bool repeated) {
        if (clock_ == nullptr || milliseconds == 0U || milliseconds > 0x7FFFFFFFU ||
            (simple == nullptr && contextual == nullptr) || generation_ == UINT32_MAX)
            return TimerHandle();
        for (size_t i = 0U; i < Capacity; ++i) {
            Slot& slot = slots_[i];
            if (slot.active) continue;
            slot.contextual = contextual != nullptr;
            if (slot.contextual) slot.callback.contextual = contextual;
            else slot.callback.simple = simple;
            slot.context = context;
            slot.deadline = clock_() + milliseconds;
            slot.interval = repeated ? milliseconds : 0U;
            slot.generation = ++generation_;
            slot.active = true;
            return TimerHandle(this, slot.generation, static_cast<uint8_t>(i));
        }
        return TimerHandle();
    }

    Slot slots_[Capacity];
    Clock clock_;
    // At UINT32_MAX, existing timers still run but new registrations fail.
    // Saturation, not wrap/reuse, protects old tokens for this object's lifetime.
    uint32_t generation_;
    bool running_;
    SketchTimer(const SketchTimer&) = delete;
    SketchTimer& operator=(const SketchTimer&) = delete;
};

} // namespace blinker

#endif
