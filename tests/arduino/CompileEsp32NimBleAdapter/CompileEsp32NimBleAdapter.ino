#include <BlinkerV2Advanced.h>
#include <BlinkerV2/arduino/ports/esp32/Esp32NimBle.h>
#include <BlinkerV2/arduino/ports/arduino/HardwareRandom.h>

blinker::Esp32NimBleLink bleLink;
blinker::PlatformHardwareRandom secureRandom;
uint8_t randomProbe[1];

void setup() {
    secureRandom.fill(blinker::MutableByteSpan(randomProbe, 1));
}

void loop() {
    bleLink.poll(1000);
}
