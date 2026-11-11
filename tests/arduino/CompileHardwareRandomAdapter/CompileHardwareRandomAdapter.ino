#include <BlinkerV2Advanced.h>
#include <HardwareRandom.h>

blinker::PlatformHardwareRandom hardwareRandom;
uint8_t randomBytes[16];

void setup()
{
    hardwareRandom.fill(
        blinker::MutableByteSpan(randomBytes, sizeof(randomBytes)));
}

void loop() {}
