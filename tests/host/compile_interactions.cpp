#include "Blinker.h"

namespace {

BLINKER_PROPERTY(
    power,
    bool,
    blinker::readWrite());
BLINKER_PROPERTY(
    temperature,
    float,
    blinker::rangedProperty(
        blinker::PropertyMode::ReadOnly,
        -40.0,
        125.0,
        0.1,
        blinker::StringView("C")));
BLINKER_PROPERTY(
    liveTemperature,
    float,
    blinker::realtime(200U));
BLINKER_ACTION(restart);
BLINKER_TYPED_ACTION_KEY(setLevel, "set-level", uint32_t);
BLINKER_EVENT(overTemperature, float);
BLINKER_VOID_EVENT_KEY(buttonPressed, "button-pressed");

const auto schema = blinker::makeDeviceSchema(
    power,
    temperature,
    liveTemperature,
    restart,
    setLevel,
    overTemperature,
    buttonPressed);

static_assert(decltype(schema)::fieldCount == 7U, "semantic schema count");
static_assert(
    sizeof(blinker::Property<bool>) <= sizeof(void*) * 5U,
    "Property runtime view must stay compact");

float sampleTemperature() {
    return 21.5f;
}

void compileTelemetrySampler() {
    (void)liveTemperature.onSample(&sampleTemperature);
}

} // namespace
