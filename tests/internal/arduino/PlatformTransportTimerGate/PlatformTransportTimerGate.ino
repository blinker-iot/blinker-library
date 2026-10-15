// Finite ordinary-product/Timer coexistence probe. No timer starts at boot.
// 's' starts one short window per boot; 'j' prints one bounded observation.
// WiFi uses a private GateConfig.h; no credentials belong in this fixture.
#include <Arduino.h>
#if defined(GATE_TRANSPORT_WIFI)
#include "GateConfig.h"
#include <BlinkerWiFi.h>
#else
#include <BlinkerBLE.h>
#endif
#include <BlinkerTimer.h>

static_assert(sizeof(blinker::Timer<4>) == 108U, "Timer ABI changed");
static_assert(sizeof(blinker::BlinkerClass) == 12U, "Default facade ABI changed");

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_PROPERTY(buttonPresses, uint32_t, blinker::readOnly());
BLINKER_PROPERTY(uptimeSeconds, uint32_t, blinker::realtime(1000U));
BLINKER_ACTION(button);

namespace {
blinker::Timer<4> timers;
blinker::TimerHandle periodic;
bool begun = false, timeEnabled = false, currentPower = false;
bool started = false, finished = false, registered = false, cancelled = false;
uint32_t loops = 0U, presses = 0U, samples = 0U, startedMillis = 0U;
uint32_t periodicCalls = 0U, onceCalls = 0U, forbiddenCalls = 0U;
uint32_t onceMillis = 0U, callsAtCancel = 0U;
typedef bool (blinker::BlinkerClass::*TimeStatus)(blinker::WallClockSnapshot&);
TimeStatus volatile timeStatusEntry = &blinker::BlinkerClass::timeStatus;

void onPower(bool value) {
    currentPower = value;
    power.report(value);
}

void onButton() { buttonPresses.report(++presses); }
uint32_t sampleUptimeSeconds() { ++samples; return millis() / 1000U; }
void forbidden() { ++forbiddenCalls; }

void number(const char* key, unsigned long value) {
    Serial.print(",\""); Serial.print(key); Serial.print("\":"); Serial.print(value);
}

void observe() {
    blinker::WallClockSnapshot time;
    const bool valid = (Blinker.*timeStatusEntry)(time);
    Serial.print("{\"fixture\":\"platform-transport-timer\"");
    number("begun", begun); number("timeEnabled", timeEnabled); number("loops", loops);
    number("started", started); number("finished", finished); number("registered", registered);
    number("cancelled", cancelled); number("activeTimers", timers.size());
    number("periodic", periodicCalls); number("once", onceCalls); number("forbidden", forbiddenCalls);
    number("onceMillis", onceMillis); number("cancelBaseline", callsAtCancel);
    number("power", currentPower); number("buttonPresses", presses);
    number("samples", samples);
    number("timeValid", valid); number("timeState", static_cast<unsigned>(time.state));
    number("timeSource", static_cast<unsigned>(time.source)); number("timeAgeMillis", time.ageMillis);
    number("timeUncertaintyMillis", time.uncertaintyMillis); number("timeRevision", time.sampleRevision);
    Serial.println('}');
}
} // namespace

// Real callbacks retained for installed-link verification, not a second runtime.
void sample() { ++periodicCalls; }
void delayed(void* context) {
    ++*static_cast<uint32_t*>(context);
    onceMillis = static_cast<uint32_t>(millis()) - startedMillis;
}

namespace {
void start() {
    if (started) return;
    started = true;
    startedMillis = static_cast<uint32_t>(millis());
    periodic = timers.setInterval(250U, sample);
    const blinker::TimerHandle once = timers.setTimeout(1001U, delayed, &onceCalls);
    const blinker::TimerHandle suppressed = timers.setTimeout(1500U, forbidden);
    registered = periodic.valid() && once.valid() && suppressed.valid() && timers.cancel(suppressed);
    if (!registered) { timers.clear(); finished = true; }
}

void runTimers() {
    if (!started || finished) return;
    if (static_cast<uint32_t>(millis()) - startedMillis >= 2500U) {
        cancelled = timers.cancel(periodic);
        callsAtCancel = periodicCalls;
        // Keep polling an overdue one-shot instead of masking it with clear().
        timers.run();
        finished = timers.size() == 0U;
        return;
    }
    timers.run();
}
} // namespace

void setup() {
    Serial.begin(115200);
    power.onWrite(onPower);
    button.onInvoke(onButton);
    uptimeSeconds.onSample(sampleUptimeSeconds);
    timeEnabled = Blinker.enableTime(); // The one existing business UTC service.
#if defined(GATE_TRANSPORT_WIFI)
    begun = timeEnabled && Blinker.begin(GATE_DEVICE_KEY, GATE_WIFI_SSID, GATE_WIFI_PASSWORD,
                                        power, buttonPresses, button, uptimeSeconds);
#else
    begun = timeEnabled && Blinker.begin(power, buttonPresses, button, uptimeSeconds);
#endif
    if (begun) Blinker.report(power, currentPower, buttonPresses, presses);
}

void loop() {
    ++loops;
    Blinker.run();
    runTimers();
    if (Serial.available() <= 0) return;
    const int command = Serial.read();
    if (command == 's') start();
    else if (command == 'j') observe();
}
