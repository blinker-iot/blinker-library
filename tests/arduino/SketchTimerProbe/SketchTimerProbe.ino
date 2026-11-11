// Independent scheduler probe: no product, network, UTC or persistent tasks.
#include <BlinkerTimer.h>
#include <BlinkerV2/api/BlinkerFacade.h> // API/ABI check only, no product selected.

static_assert(sizeof(blinker::Timer<1>) == 36U, "1-slot timer ABI changed");
static_assert(sizeof(blinker::Timer<4>) == 108U, "4-slot timer ABI changed");
static_assert(sizeof(blinker::Timer<8>) == 204U, "8-slot timer ABI changed");
static_assert(sizeof(blinker::TimerHandle) == 12U, "Timer token ABI changed");
static_assert(sizeof(blinker::BlinkerClass) == 12U, "Default facade ABI changed");

blinker::Timer<4> timers;
blinker::TimerHandle periodic;
unsigned callbackCount = 0U;

// Retain a real call to the installed public method, without binding a clock
// or starting a product. A volatile member pointer prevents LTO from reducing
// the unused-clock check to a compile-time false/sizeof-only API test.
blinker::BlinkerClass timeObserver;
typedef bool (blinker::BlinkerClass::*TimeStatus)(blinker::WallClockSnapshot&);
TimeStatus volatile timeStatusEntry = &blinker::BlinkerClass::timeStatus;
volatile bool utcAvailable = false;

void sample() { ++callbackCount; }
void delayed(void* context) { ++*static_cast<unsigned*>(context); }

void setup() {
    periodic = timers.setInterval(1000U, sample);
    const auto once = timers.setTimeout(500U, delayed, &callbackCount);
    if (!periodic.valid() || !once.valid()) timers.clear();
}

void loop() {
    timers.run();
    if (callbackCount >= 10U) timers.cancel(periodic);
    blinker::WallClockSnapshot snapshot;
    utcAvailable = (timeObserver.*timeStatusEntry)(snapshot);
}
