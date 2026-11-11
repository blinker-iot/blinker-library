#include <BlinkerV2Advanced.h>
#include <ArduinoBleLink.h>
#include <HardwareRandom.h>

blinker::ArduinoBleLink bleLink;
blinker::PlatformHardwareRandom secureRandom;
uint8_t randomProbe[1];

void setup() { secureRandom.fill(blinker::MutableByteSpan(randomProbe, 1)); }
void loop() { bleLink.poll(1000); }
