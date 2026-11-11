#include "BlinkerV2/core/SketchTimer.h"
#include <cstdlib>
#include <iostream>

using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

namespace {
uint32_t now = 0U;
unsigned calls = 0U;
uint32_t clockNow() { return now; }
void count() { ++calls; }
void contextCount(void* value) { ++*static_cast<unsigned*>(value); }

void testBasics() {
    now = 100U; calls = 0U;
    SketchTimer<2U> timer(clockNow);
    const TimerHandle interval = timer.setInterval(10U, count);
    const TimerHandle timeout = timer.setTimeout(25U, count);
    CHECK(interval.valid() && timeout.valid() && timer.size() == 2U);
    CHECK(!timer.setTimeout(1U, count).valid());
    now = 109U; CHECK(timer.run() == 0U);
    now = 110U; CHECK(timer.run() == 1U && calls == 1U);
    now = 110U; CHECK(timer.run() == 0U);
    now = 125U; CHECK(timer.run() == 2U && calls == 3U);
    CHECK(timer.active(interval) && !timer.active(timeout) && timer.size() == 1U);
    now = 169U; CHECK(timer.run() == 1U && calls == 4U); // No catch-up callbacks.
    now = 170U; CHECK(timer.run() == 1U && calls == 5U); // Original phase retained.
    CHECK(timer.cancel(interval) && !timer.cancel(interval));
    CHECK(timer.size() == 0U && timer.run() == 0U);
}

void testArgumentsAndTokens() {
    now = 0U; calls = 0U;
    SketchTimer<1U> timer(clockNow), other(clockNow), disabled(nullptr);
    CHECK(!disabled.setInterval(1U, count).valid() && disabled.run() == 0U);
    CHECK(!timer.setInterval(0U, count).valid());
    CHECK(!timer.setTimeout(0x80000000U, count).valid());
    CHECK(!timer.setTimeout(1U, static_cast<SketchTimer<1U>::Callback>(nullptr)).valid());
    CHECK(!timer.setInterval(1U, static_cast<SketchTimer<1U>::ContextCallback>(nullptr), nullptr).valid());
    CHECK(!timer.active(TimerHandle()) && !timer.cancel(TimerHandle()));
    const TimerHandle first = timer.setTimeout(1U, count);
    const TimerHandle foreign = other.setTimeout(1U, count);
    CHECK(!timer.cancel(foreign) && !other.cancel(first));
    CHECK(timer.cancel(first));
    const TimerHandle replacement = timer.setTimeout(1U, count);
    CHECK(!timer.cancel(first) && timer.active(replacement));
    CHECK(!timer.setTimeout(0U, count).valid() && timer.active(replacement));
    timer.clear();
    const TimerHandle afterClear = timer.setTimeout(1U, count);
    CHECK(!timer.cancel(replacement) && timer.active(afterClear));
    now = 1U; CHECK(timer.run() == 1U && calls == 1U);
    CHECK(!timer.active(afterClear));
    unsigned contextual = 0U;
    CHECK(timer.setInterval(1U, contextCount, &contextual).valid());
    now = 2U; CHECK(timer.run() == 1U && contextual == 1U);
}

void testWrapAndMaximumDelay() {
    now = UINT32_MAX - 4U; calls = 0U;
    SketchTimer<1U> timer(clockNow);
    CHECK(timer.setInterval(10U, count).valid());
    now = 4U; CHECK(timer.run() == 0U);
    now = 5U; CHECK(timer.run() == 1U && calls == 1U);
    now = 14U; CHECK(timer.run() == 0U);
    now = 15U; CHECK(timer.run() == 1U && calls == 2U);
    timer.clear();
    now = 500U;
    CHECK(timer.setTimeout(0x7FFFFFFFU, count).valid());
    now = 500U + 0x7FFFFFFEU; CHECK(timer.run() == 0U);
    now = 500U + 0x7FFFFFFFU; CHECK(timer.run() == 1U && calls == 3U);
    timer.clear(); now = 0U;
    CHECK(timer.setInterval(1U, count).valid());
    now = 0x7FFFFFFFU; CHECK(timer.run() == 1U && calls == 4U);
    CHECK(timer.run() == 0U);
    ++now; CHECK(timer.run() == 1U && calls == 5U);
}

struct Mutation {
    SketchTimer<2U>* timer;
    TimerHandle self;
    TimerHandle other;
    TimerHandle added;
    unsigned invoked;
    Mutation() : timer(nullptr), self(), other(), added(), invoked(0U) {}
};
void cancelDuringCallback(void* value) {
    Mutation& state = *static_cast<Mutation*>(value);
    ++state.invoked;
    CHECK(state.timer->run() == 0U); // Reentry cannot execute other due callbacks.
    CHECK(state.timer->cancel(state.self));
    CHECK(state.timer->cancel(state.other));
    state.added = state.timer->setTimeout(1U, count);
    CHECK(state.added.valid());
    now += 10U; // A newly due callback still waits for the next outer run.
}
void clearDuringCallback(void* value) {
    Mutation& state = *static_cast<Mutation*>(value);
    ++state.invoked;
    state.timer->clear();
    state.added = state.timer->setTimeout(1U, count);
    CHECK(state.added.valid());
    now += 10U;
}
void testCallbackMutation() {
    now = 0U; calls = 0U;
    SketchTimer<2U> timer(clockNow);
    Mutation state; state.timer = &timer;
    state.self = timer.setInterval(1U, cancelDuringCallback, &state);
    state.other = timer.setTimeout(1U, count);
    now = 1U;
    CHECK(timer.run() == 1U && state.invoked == 1U && calls == 0U);
    CHECK(!timer.active(state.self) && !timer.active(state.other) && timer.active(state.added));
    CHECK(timer.run() == 1U && calls == 1U);
    state.self = timer.setTimeout(1U, clearDuringCallback, &state);
    state.other = timer.setTimeout(1U, count);
    ++now;
    CHECK(timer.run() == 1U && state.invoked == 2U && calls == 1U);
    CHECK(timer.run() == 1U && calls == 2U);
}

struct OneSlot {
    SketchTimer<1U>* timer;
    TimerHandle next;
    unsigned called;
    OneSlot() : timer(nullptr), next(), called(0U) {}
};
void restartTimeout(void* value) {
    OneSlot& state = *static_cast<OneSlot*>(value);
    ++state.called;
    CHECK(state.timer->size() == 0U); // One-shot slot is released before callback.
    state.next = state.timer->setTimeout(1U, count);
    CHECK(state.next.valid());
    ++now;
}
void testOneShotReuse() {
    now = 0U; calls = 0U;
    SketchTimer<1U> timer(clockNow);
    OneSlot state; state.timer = &timer;
    const TimerHandle old = timer.setTimeout(1U, restartTimeout, &state);
    now = 1U; CHECK(timer.run() == 1U && state.called == 1U && calls == 0U);
    CHECK(!timer.cancel(old) && timer.active(state.next));
    CHECK(timer.run() == 1U && calls == 1U);
}

void addIntoLaterSlot(void* value) {
    Mutation& state = *static_cast<Mutation*>(value);
    ++state.invoked;
    state.added = state.timer->setTimeout(1U, count);
    CHECK(state.added.valid());
    now += 10U;
}
void testNewLaterSlot() {
    now = 0U; calls = 0U;
    SketchTimer<2U> timer(clockNow);
    Mutation state; state.timer = &timer;
    state.self = timer.setInterval(1U, addIntoLaterSlot, &state);
    now = 1U; CHECK(timer.run() == 1U && state.invoked == 1U && calls == 0U);
    CHECK(timer.cancel(state.self));
    CHECK(timer.run() == 1U && calls == 1U);
}
}

int main() {
    testBasics();
    testArgumentsAndTokens();
    testWrapAndMaximumDelay();
    testCallbackMutation();
    testOneShotReuse();
    testNewLaterSlot();
    std::cout << "Sketch timers: bounds, phase, wrap, stale tokens and callback mutation passed\n";
}
