#if defined(ARDUINO_ARCH_ESP32)

#include "Esp32SntpClock.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

namespace blinker {

Esp32SntpClock::Esp32SntpClock(const Esp32SntpClockConfig& config)
    : config_(config), requestedAt_(0U), started_(false), trusted_(false) {}

Result Esp32SntpClock::start() {
    if (started_) return Result::success();
    if (config_.primaryServer == nullptr ||
        config_.primaryServer[0] == '\0' || config_.retryMillis == 0U ||
        config_.minimumUnixSeconds == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    started_ = true;
    trusted_ = false;
    requestSync();
    return Result::success();
}

void Esp32SntpClock::poll() {
    if (!started_) return;
    if (!trusted_ &&
        esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        uint64_t now = 0U;
        trusted_ = currentUnixTime(now);
    }
    if (!trusted_ &&
        static_cast<uint32_t>(millis() - requestedAt_) >=
            config_.retryMillis) {
        requestSync();
    }
}

void Esp32SntpClock::stop() {
    requestedAt_ = 0U;
    started_ = false;
    trusted_ = false;
}

uint32_t Esp32SntpClock::monotonicMillis() const {
    return millis();
}

bool Esp32SntpClock::unixTime(uint64_t& output) const {
    output = 0U;
    return trusted_ && currentUnixTime(output);
}

void Esp32SntpClock::requestSync() {
    configTime(
        0,
        0,
        config_.primaryServer,
        config_.secondaryServer != nullptr &&
                config_.secondaryServer[0] != '\0'
            ? config_.secondaryServer
            : nullptr);
    requestedAt_ = millis();
}

bool Esp32SntpClock::currentUnixTime(uint64_t& output) const {
    const time_t value = time(nullptr);
    if (value < 0 ||
        static_cast<uint64_t>(value) < config_.minimumUnixSeconds) {
        output = 0U;
        return false;
    }
    output = static_cast<uint64_t>(value);
    return true;
}

} // namespace blinker

#endif // ARDUINO_ARCH_ESP32
