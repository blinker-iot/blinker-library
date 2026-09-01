#ifndef BLINKER_V2_ARDUINO_PORTS_ESP32_SNTPCLOCK_H
#define BLINKER_V2_ARDUINO_PORTS_ESP32_SNTPCLOCK_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32SntpClock requires an ESP32 Arduino target"
#endif

#include <BlinkerV2/interface/IClock.h>

namespace blinker {

struct Esp32SntpClockConfig {
    const char* primaryServer;
    const char* secondaryServer;
    uint64_t minimumUnixSeconds;
    uint32_t retryMillis;

    Esp32SntpClockConfig()
        : primaryServer("pool.ntp.org"),
          secondaryServer("time.cloudflare.com"),
          minimumUnixSeconds(1704067200ULL),
          retryMillis(60000U) {}
};

// Non-blocking trusted wall clock for TLS and expiring management envelopes.
// A plausible RTC value alone is insufficient: unixTime() stays unavailable
// until this boot observes an ESP-IDF SNTP completion event.
class Esp32SntpClock : public IClock {
public:
    explicit Esp32SntpClock(
        const Esp32SntpClockConfig& config = Esp32SntpClockConfig());

    Result start();
    void poll();
    void stop();

    uint32_t monotonicMillis() const override;
    bool unixTime(uint64_t& output) const override;

    bool started() const { return started_; }
    bool trusted() const { return trusted_; }

private:
    void requestSync();
    bool currentUnixTime(uint64_t& output) const;

    Esp32SntpClockConfig config_;
    uint32_t requestedAt_;
    bool started_;
    bool trusted_;
};

} // namespace blinker

#endif
