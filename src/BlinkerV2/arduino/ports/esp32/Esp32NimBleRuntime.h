#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLERUNTIME_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_NIMBLERUNTIME_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32NimBleRuntime requires an ESP32 Arduino target"
#endif

namespace blinker {
namespace esp32_nimble_detail {

// ESP-IDF exposes one process-wide NimBLE Host. Blinker Peripheral and Central
// ports therefore share one owner instead of keeping unrelated guards.
// ESP WiFiProv is owned by its SDK; product composition must end it completely
// before either Blinker port is started.
inline void*& activeOwner() {
    static void* owner = nullptr;
    return owner;
}

inline bool claim(void* owner) {
    if (owner == nullptr || activeOwner() != nullptr) return false;
    activeOwner() = owner;
    return true;
}

inline void release(void* owner) {
    if (activeOwner() == owner) activeOwner() = nullptr;
}

} // namespace esp32_nimble_detail
} // namespace blinker

#endif
