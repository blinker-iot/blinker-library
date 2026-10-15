// Internal, finite R4/Wio hardware probe. Never arms timers at boot.
// 's': one six-second window per boot; 'j': one observation. Keep the host
// reading Serial while requesting output. Reset externally for the next boot.
// No product, GPIO action, UTC synchronization, storage or bridge/RTL call.
#include <Arduino.h>
#include <BlinkerTimer.h>
#include <BlinkerV2/api/BlinkerFacade.h>

static_assert(sizeof(blinker::Timer<1>) == 36U, "1-slot timer ABI changed");
static_assert(sizeof(blinker::Timer<4>) == 108U, "4-slot timer ABI changed");
static_assert(sizeof(blinker::Timer<8>) == 204U, "8-slot timer ABI changed");
static_assert(sizeof(blinker::TimerHandle) == 12U, "Timer token ABI changed");
static_assert(sizeof(blinker::BlinkerClass) == 12U, "Default facade ABI changed");

namespace {
blinker::Timer<4> timers;
blinker::TimerHandle periodic;
blinker::TimerHandle selfPeriodic;
blinker::BlinkerClass timeObserver; // Deliberately never bound/enabled.
typedef bool (blinker::BlinkerClass::*TimeStatus)(blinker::WallClockSnapshot&);
TimeStatus volatile timeStatusEntry = &blinker::BlinkerClass::timeStatus;

bool started = false, armed = false, finished = false, cancelled = false;
bool handlesValid = false, capacityRejected = false, opaqueHandles = false;
bool earlyCancelWorked = false, explicitCancelWorked = false, selfCancelWorked = false;
bool pauseObserved = false, resumeObserved = false, bootTimeChecksPassed = false;
uint32_t bootActiveTimers = 0U, startedMillis = 0U, loops = 0U, windowLoops = 0U;
uint32_t periodicCalls = 0U, onceCalls = 0U, forbiddenCalls = 0U, selfCalls = 0U;
uint32_t callsAtCancel = 0U, maxPeriodicPerRun = 0U, firstOnceMillis = 0U;
uint32_t periodicAtPause = 0U, periodicResumeCalls = 0U;

bool observeTime(blinker::WallClockSnapshot& snapshot, bool& valid) {
    // Non-default input proves that the real API initializes unavailable output.
    snapshot.utcMillis = 1U;
    snapshot.ageMillis = snapshot.uncertaintyMillis = snapshot.sampleRevision = 1U;
    snapshot.state = blinker::WallClockState::Synchronized;
    snapshot.source = blinker::WallTimeSource::Controller;
    valid = (timeObserver.*timeStatusEntry)(snapshot);
    return !valid && snapshot.state == blinker::WallClockState::Unsynchronized &&
        snapshot.source == blinker::WallTimeSource::None && snapshot.utcMillis == 0U &&
        snapshot.ageMillis == 0U && snapshot.uncertaintyMillis == 0U &&
        snapshot.sampleRevision == 0U;
}

void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}

void observations(const char* event) {
    blinker::WallClockSnapshot time;
    bool timeValid = false;
    const bool timeChecksPassed = observeTime(time, timeValid);
    const bool timerChecksPassed = finished && handlesValid && capacityRejected &&
        opaqueHandles && earlyCancelWorked && explicitCancelWorked && selfCancelWorked &&
        onceCalls == 1U && forbiddenCalls == 0U && selfCalls == 3U &&
        firstOnceMillis >= 1001U && periodicCalls > 0U && periodicCalls == callsAtCancel &&
        maxPeriodicPerRun == 1U && pauseObserved && resumeObserved &&
        periodicResumeCalls == 1U && timers.size() == 0U;
    Serial.print("{\"fixture\":\"sketch-timer-live\",\"event\":\"");
    Serial.print(event); Serial.print('"');
    number("started", started); number("armed", armed); number("finished", finished);
    number("loops", loops); number("windowLoops", windowLoops);
    number("elapsedMillis", started ? static_cast<uint32_t>(millis()) - startedMillis : 0U);
    number("periodic", periodicCalls); number("once", onceCalls);
    number("forbidden", forbiddenCalls); number("self", selfCalls);
    number("cancelBaseline", callsAtCancel); number("handlesValid", handlesValid);
    number("capacityRejected", capacityRejected); number("opaqueHandles", opaqueHandles);
    number("earlyCancel", earlyCancelWorked); number("explicitCancel", explicitCancelWorked);
    number("selfCancel", selfCancelWorked); number("maxPeriodicPerRun", maxPeriodicPerRun);
    number("firstOnceMillis", firstOnceMillis); number("periodicAtPause", periodicAtPause);
    number("periodicResumeCalls", periodicResumeCalls);
    number("pauseObserved", pauseObserved); number("resumeObserved", resumeObserved);
    number("timerChecksPassed", timerChecksPassed); number("activeTimers", timers.size());
    number("bootActiveTimers", bootActiveTimers); number("bootTimeChecksPassed", bootTimeChecksPassed);
    number("timeValid", timeValid); number("timeState", static_cast<unsigned>(time.state));
    number("timeSource", static_cast<unsigned>(time.source)); number("timeAgeMillis", time.ageMillis);
    number("timeChecksPassed", timeChecksPassed); Serial.println('}');
}

void forbidden() { ++forbiddenCalls; }
void selfCancelling() {
    ++selfCalls;
    if (selfCalls == 3U) selfCancelWorked = timers.cancel(selfPeriodic);
}
} // namespace

// Keep these real callbacks externally named for the installed-link symbol gate.
void sample() { ++periodicCalls; }
void delayed(void* context) {
    ++*static_cast<uint32_t*>(context);
    firstOnceMillis = static_cast<uint32_t>(millis()) - startedMillis;
}

namespace {
void startWindow() {
    if (started) { observations("start-rejected"); return; }
    started = true;
    startedMillis = static_cast<uint32_t>(millis());
    periodic = timers.setInterval(250U, sample);
    const blinker::TimerHandle once = timers.setTimeout(1001U, delayed, &onceCalls);
    const blinker::TimerHandle cancelledTimeout = timers.setTimeout(1500U, forbidden);
    selfPeriodic = timers.setInterval(400U, selfCancelling);
    handlesValid = periodic.valid() && once.valid() && cancelledTimeout.valid() && selfPeriodic.valid();
    capacityRejected = !timers.setTimeout(1U, forbidden).valid(); // All four slots are occupied.
    earlyCancelWorked = timers.cancel(cancelledTimeout);
    // Reuse only the vacated slot, immediately cancel it, and prove the opaque
    // issued-but-inactive old token cannot cancel its replacement.
    const blinker::TimerHandle replacement = timers.setTimeout(1U, forbidden);
    opaqueHandles = cancelledTimeout.valid() && !timers.active(cancelledTimeout) &&
        replacement.valid() && !timers.cancel(cancelledTimeout) && timers.active(replacement) &&
        timers.cancel(replacement) && !timers.active(blinker::TimerHandle());
    armed = handlesValid && capacityRejected && earlyCancelWorked && opaqueHandles;
    if (!armed) { timers.clear(); finished = true; }
    observations(armed ? "start" : "registration-rejected");
}

void advanceWindow() {
    if (!armed) return;
    ++windowLoops;
    const uint32_t elapsed = static_cast<uint32_t>(millis()) - startedMillis;
    if (!pauseObserved && elapsed >= 1500U && elapsed < 2250U) {
        pauseObserved = true;
        periodicAtPause = periodicCalls;
    }
    if (!cancelled && elapsed >= 2500U) {
        explicitCancelWorked = timers.cancel(periodic);
        callsAtCancel = periodicCalls;
        cancelled = true;
        observations("cancel");
    }
    // Pause only this scheduler; loop() and the serial command path continue.
    if (elapsed < 1500U || elapsed >= 2250U) {
        const uint32_t before = periodicCalls;
        timers.run();
        const uint32_t advanced = periodicCalls - before;
        if (advanced > maxPeriodicPerRun) maxPeriodicPerRun = advanced;
        if (pauseObserved && !resumeObserved) {
            resumeObserved = true;
            periodicResumeCalls = periodicCalls - periodicAtPause;
        }
    }
    if (elapsed >= 6000U) {
        armed = false;
        finished = true;
        observations("finish"); // Do not clear slots to manufacture a successful final size.
    }
}
} // namespace

void setup() {
    Serial.begin(115200);
    bootActiveTimers = static_cast<uint32_t>(timers.size());
    blinker::WallClockSnapshot time;
    bool timeValid = false;
    bootTimeChecksPassed = observeTime(time, timeValid);
    observations("boot"); // No wait for host, auto-arm or unattended recurring log.
}

void loop() {
    ++loops;
    advanceWindow();
    if (Serial.available() <= 0) return;
    const int command = Serial.read(); // At most one input byte per loop.
    if (command == 's') startWindow();
    else if (command == 'j') observations("requested");
}
