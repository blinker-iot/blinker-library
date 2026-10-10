#include <BlinkerV2/arduino/ports/esp32/Esp32NimBleCentralPort.h>
#include <cstdlib>
#include <iostream>

FakeHostConfig ble_hs_cfg = {};
fake_nimble::State fake_nimble::state;

int main() {
    static_assert(blinker::Esp32NimBleCentralHost::capacity == 1U, "honor the SDK pool");
    blinker::Esp32NimBleCentralHost host;
    blinker::Esp32NimBleCentralPort a(host), b(host);
    if (!a.start() || b.start().code() != blinker::ErrorCode::CapacityExceeded ||
        fake_nimble::state.initCalls != 1U) return EXIT_FAILURE;
    b.stop();
    if (a.state() != blinker::BleCentralPortState::Idle || fake_nimble::state.stopCalls != 0U) return EXIT_FAILURE;
    a.stop();
    if (!b.start() || fake_nimble::state.initCalls != 1U) return EXIT_FAILURE;
    b.stop();
    if (!host.end()) return EXIT_FAILURE;
    std::cout << "ESP32 SDK one-slot admission passed" << std::endl;
}
