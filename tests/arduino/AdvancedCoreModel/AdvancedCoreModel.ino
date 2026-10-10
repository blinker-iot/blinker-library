#include <BlinkerV2Advanced.h>

// Internal advanced model-only probe. Normal product sketches select
// <BlinkerWiFi.h> or <BlinkerBLE.h>, then use the same global Blinker facade.
BLINKER_PROPERTY(
    power,
    bool,
    blinker::readWrite());
BLINKER_DEVICE(device, power);
bool started = false;

void onPowerChange(bool requestedState)
{
    // Real firmware changes hardware first, then reports the confirmed fact.
    power.report(requestedState);
}

void setup()
{
    const blinker::Result callback = power.onWrite(&onPowerChange);
    blinker::Result result = callback ? device.begin() : callback;
    if (result) result = power.report(false);
    started = result.ok();
}

void loop()
{
    if (started) device.run();
}
