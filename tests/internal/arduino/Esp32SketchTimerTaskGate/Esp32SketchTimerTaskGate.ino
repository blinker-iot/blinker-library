// Internal finite hardware gate, not a public example. Copy to an ignored
// build directory and supply matching GateConfig.h / TaskPilotPolicy.h there.
// GateConfig defines GATE_DEVICE_KEY, GATE_WIFI_SSID and GATE_WIFI_PASSWORD;
// credentials, network keys and maintainer policy never belong in this file.
// 's': one 6 s window; 'j': observations; 'd': existing product diagnostics.
// Never clears access, starts a persistent task or arms timers at boot.
#include "GateConfig.h"
#define BLINKER_ENABLE_TASKS 1
#include "TaskPilotPolicy.h"
#include <BlinkerWiFi.h> // Internal probe selects WiFi explicitly.
#include <BlinkerTimer.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(duty, uint32_t,
    blinker::rangedProperty(blinker::PropertyMode::ReadWrite, 0.0, 100.0, 5.0));

namespace {
blinker::Timer<4> timers;
blinker::TimerHandle periodic;
blinker::TimerHandle selfPeriodic;

bool begun = false;
bool armed = false;
bool finished = false;
bool cancelled = false;
bool handlesValid = false;
bool earlyCancelWorked = false;
bool explicitCancelWorked = false;
bool selfCancelWorked = false;
bool pauseObserved = false;
bool resumeObserved = false;
bool currentPower = false;
uint32_t currentDuty = 0U;
uint32_t startedMillis = 0U;
uint32_t loops = 0U;
uint32_t periodicCalls = 0U;
uint32_t onceCalls = 0U;
uint32_t forbiddenCalls = 0U;
uint32_t selfCalls = 0U;
uint32_t callsAtCancel = 0U;
uint32_t maxPeriodicPerRun = 0U;
uint32_t firstOnceMillis = 0U;
uint32_t periodicAtPause = 0U;
uint32_t periodicResumeCalls = 0U;
uint32_t powerWrites = 0U;
uint32_t dutyWrites = 0U;

void onPower(bool requested) {
    currentPower = requested;
    ++powerWrites;
    power.report(currentPower);
}
void onDuty(uint32_t requested) {
    currentDuty = requested;
    ++dutyWrites;
    duty.report(currentDuty);
}
void sampleLocalCache() { ++periodicCalls; }
void delayed(void* context) {
    ++*static_cast<uint32_t*>(context);
    firstOnceMillis = static_cast<uint32_t>(millis()) - startedMillis;
}
void forbidden() { ++forbiddenCalls; }
void selfCancelling() {
    ++selfCalls;
    if (selfCalls == 3U) selfCancelWorked = timers.cancel(selfPeriodic);
}

void observations(const char* event) {
    blinker::WallClockSnapshot time;
    const bool timeValid = Blinker.timeStatus(time);
    const bool timerChecksPassed = finished && handlesValid && earlyCancelWorked &&
        explicitCancelWorked && selfCancelWorked && onceCalls == 1U &&
        forbiddenCalls == 0U && selfCalls == 3U && firstOnceMillis >= 1001U &&
        periodicCalls > 0U && periodicCalls == callsAtCancel &&
        maxPeriodicPerRun == 1U && pauseObserved && resumeObserved &&
        periodicResumeCalls == 1U && timers.size() == 0U;
    Serial.printf(
        "{\"fixture\":\"sketch-timer-task\",\"event\":\"%s\","
        "\"begun\":%u,\"armed\":%u,\"finished\":%u,\"handlesValid\":%u,"
        "\"loops\":%lu,\"periodic\":%lu,\"once\":%lu,\"forbidden\":%lu,"
        "\"self\":%lu,\"cancelBaseline\":%lu,\"earlyCancel\":%u,"
        "\"explicitCancel\":%u,\"selfCancel\":%u,\"maxPeriodicPerRun\":%lu,"
        "\"firstOnceMillis\":%lu,\"periodicAtPause\":%lu,\"periodicResumeCalls\":%lu,"
        "\"pauseObserved\":%u,\"resumeObserved\":%u,\"timerChecksPassed\":%u,"
        "\"activeTimers\":%u,\"power\":%u,"
        "\"duty\":%lu,\"powerWrites\":%lu,\"dutyWrites\":%lu,"
        "\"timeValid\":%u,\"timeState\":%u,\"timeSource\":%u,\"timeAgeMillis\":%lu}\n",
        event, begun, armed, finished, handlesValid,
        static_cast<unsigned long>(loops), static_cast<unsigned long>(periodicCalls),
        static_cast<unsigned long>(onceCalls), static_cast<unsigned long>(forbiddenCalls),
        static_cast<unsigned long>(selfCalls), static_cast<unsigned long>(callsAtCancel),
        earlyCancelWorked, explicitCancelWorked, selfCancelWorked,
        static_cast<unsigned long>(maxPeriodicPerRun), static_cast<unsigned long>(firstOnceMillis),
        static_cast<unsigned long>(periodicAtPause), static_cast<unsigned long>(periodicResumeCalls),
        pauseObserved, resumeObserved, timerChecksPassed,
        static_cast<unsigned>(timers.size()), currentPower, static_cast<unsigned long>(currentDuty),
        static_cast<unsigned long>(powerWrites), static_cast<unsigned long>(dutyWrites),
        timeValid, static_cast<unsigned>(time.state), static_cast<unsigned>(time.source),
        static_cast<unsigned long>(time.ageMillis));
}

void startWindow() {
    if (!begun || armed) { observations("start-rejected"); return; }
    timers.clear();
    armed = true;
    finished = cancelled = handlesValid = false;
    earlyCancelWorked = explicitCancelWorked = selfCancelWorked = false;
    pauseObserved = resumeObserved = false;
    loops = periodicCalls = onceCalls = forbiddenCalls = selfCalls = 0U;
    callsAtCancel = maxPeriodicPerRun = firstOnceMillis = 0U;
    periodicAtPause = periodicResumeCalls = 0U;
    startedMillis = static_cast<uint32_t>(millis());
    periodic = timers.setInterval(250U, sampleLocalCache);
    const blinker::TimerHandle once = timers.setTimeout(1001U, delayed, &onceCalls);
    const blinker::TimerHandle cancelledTimeout = timers.setTimeout(1500U, forbidden);
    selfPeriodic = timers.setInterval(400U, selfCancelling);
    handlesValid = periodic.valid() && once.valid() && cancelledTimeout.valid() && selfPeriodic.valid();
    earlyCancelWorked = timers.cancel(cancelledTimeout);
    if (!handlesValid || !earlyCancelWorked) { armed = false; timers.clear(); }
    observations("start");
}

void advanceWindow() {
    if (!armed) return;
    ++loops;
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
    // Intentionally defer only scheduler polls, not communication or loop().
    // A resumed interval must skip missed periods, not emit a catch-up burst.
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
        observations("finish");
        timers.clear();
    }
}
} // namespace

void setup() {
    Serial.begin(115200);
    power.onWrite(onPower);
    duty.onWrite(onDuty);
    begun = Blinker.enableTasks(power, duty) &&
        Blinker.begin(GATE_DEVICE_KEY, GATE_WIFI_SSID, GATE_WIFI_PASSWORD, power, duty);
    if (begun) Blinker.report(power, currentPower, duty, currentDuty);
    observations("boot");
}

void loop() {
    if (begun) Blinker.run();
    advanceWindow();
    // Bound serial work to one byte per loop. Leave the monitor consuming data
    // while requesting a dump; no unattended periodic debug output exists.
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 's') startWindow();
    else if (command == 'j') observations("requested");
    else if (command == 'd') Blinker.printDiagnostics(Serial);
}
