#ifndef BLINKER_RUNTIME_TELEMETRY_LEASE_H
#define BLINKER_RUNTIME_TELEMETRY_LEASE_H

#include "../core/ResourceProfile.h"
#include "../model/EndpointRegistry.h"
#include "../protocol/bbp2/Messages.h"
#include "../transport/IFrameTransport.h"

namespace blinker {

static const uint32_t kTelemetryMinimumLeaseMillis = 5000U;
static const uint32_t kTelemetryMaximumLeaseMillis = 60000U;
static const uint32_t kTelemetryMaximumIntervalMillis = 60000U;

struct TelemetryLeaseSlot {
    uint32_t sessionId;
    uint32_t streamId;
    uint32_t streamEpoch;
    uint32_t intervalMillis;
    uint32_t expiresAtMillis;
    uint32_t nextSampleAtMillis;
    uint32_t sampleSequence;
    uint8_t fields[(BLINKER_DEVICE_MAX_FIELDS + 7U) / 8U];
    uint8_t transportId;
    uint8_t fieldCount;
    bool occupied;

    TelemetryLeaseSlot();
};

struct TelemetryCounters {
    // Success is full-width. Drop/expiry are exceptional diagnostics and
    // saturate at UINT16_MAX so the 32-bit Device ABI does not grow.
    uint32_t samplesSent;
    uint16_t samplesDropped;
    uint16_t leasesExpired;

    TelemetryCounters()
        : samplesSent(0U), samplesDropped(0U), leasesExpired(0U) {}
};

static_assert(
    sizeof(TelemetryCounters) == 8U,
    "TelemetryCounters must remain an 8-byte fixed-resource record");

// Fixed-capacity session-local lease table. It contains no transport or clock
// implementation and therefore behaves identically on WiFi, BLE and mixed
// products.
class TelemetryLeaseTable {
public:
    TelemetryLeaseTable();

    Result apply(
        const bbp2::TelemetryControlBody& control,
        const RxContext& source,
        const EndpointRegistry& endpoints,
        uint32_t nowMillis,
        bbp2::TelemetryStatusBody& status);

    TelemetryLeaseSlot* firstExpired(uint32_t nowMillis);
    TelemetryLeaseSlot* firstDue(uint32_t nowMillis);
    void deferSource(
        const RxContext& source,
        uint32_t nowMillis,
        uint32_t durationMillis);
    void sampled(TelemetryLeaseSlot& lease, uint32_t nowMillis);
    void close(TelemetryLeaseSlot& lease);
    void sessionClosed(const RxContext& source);
    void transportClosed(uint8_t transportId);
    void reset();

    size_t activeCount() const;
    static bool selected(const TelemetryLeaseSlot& lease, uint16_t fieldId);

private:
    TelemetryLeaseSlot* find(const RxContext& source, uint32_t streamId);
    TelemetryLeaseSlot* allocate();
    uint32_t nextEpoch();

    TelemetryLeaseSlot slots_[BLINKER_TELEMETRY_LEASE_SLOTS];
    uint32_t nextEpoch_;
};

} // namespace blinker

#endif
