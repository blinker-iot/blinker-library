// Driver-free optional API probe, not a new released platform or default upload.
#include <Blinker.h>

BLINKER_PROPERTY(power, bool, blinker::readWrite());
BLINKER_LOCATION(position);

// Call only when your GPS/parser/other device delivers a NEW WGS84 observation.
// This function is not called by this sketch: no simulated/default fix is sent.
bool acceptUserLocation(double latitude, double longitude, uint32_t sampledAtMillis) {
    return Blinker.location(position, latitude, longitude, sampledAtMillis);
}

// Optional metadata uses integer E7 coordinates, centimetres, and actual UTC.
// Unknown accuracy/UTC stay unknown; no separate lat/lon State fields are used.
bool acceptUserLocation(const blinker::LocationFix& observation, uint32_t sampledAtMillis) {
    return Blinker.location(position, observation, sampledAtMillis);
}

bool acceptFreshUserLocation(double latitude, double longitude) {
    return Blinker.location(position, latitude, longitude);
}

bool acceptFreshUserLocation(const blinker::LocationFix& observation) {
    return Blinker.location(position, observation);
}

void userLocationLost() { Blinker.locationLost(position); }
void userLocationClockReset() { Blinker.clearLocation(position); }

// Compile/link admission only: root all six input adapters so --gc-sections
// cannot turn this into an include-only test. loop() reads the volatile
// pointers, never calls them or manufactures a hardware observation.
namespace {
bool (*volatile retainedTimedDegrees)(double, double, uint32_t) = &acceptUserLocation;
bool (*volatile retainedTimedFix)(const blinker::LocationFix&, uint32_t) = &acceptUserLocation;
bool (*volatile retainedFreshDegrees)(double, double) = &acceptFreshUserLocation;
bool (*volatile retainedFreshFix)(const blinker::LocationFix&) = &acceptFreshUserLocation;
void (*volatile retainedLost)() = &userLocationLost;
void (*volatile retainedClear)() = &userLocationClockReset;
}

void setup() {
    if (Blinker.begin("REPLACE_WITH_DEVICE_KEY", "YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD", power, position)) {
        power.report(false);
    }
}

void loop() {
    // Your nonblocking hardware/parser work belongs here. Do not re-stamp an
    // old point each loop: pass its original same-boot millis() sample tick.
    (void)retainedTimedDegrees;
    (void)retainedTimedFix;
    (void)retainedFreshDegrees;
    (void)retainedFreshFix;
    (void)retainedLost;
    (void)retainedClear;
    Blinker.run();
}
