#include "TelemetryLease.h"

#include <string.h>

namespace blinker {

namespace {

bool reached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

bool sameSource(const TelemetryLeaseSlot& lease, const RxContext& source) {
    return lease.transportId == source.transportId &&
           lease.sessionId == source.sessionId;
}

void fillStatus(
    const TelemetryLeaseSlot& lease,
    bbp2::TelemetryStatusCode code,
    uint32_t leaseMillis,
    bbp2::TelemetryStatusBody& status) {
    status.streamId = lease.streamId;
    status.streamEpoch = lease.streamEpoch;
    status.status = code;
    status.effectiveIntervalMillis = lease.intervalMillis;
    status.leaseMillis = leaseMillis;
}

} // namespace

TelemetryLeaseSlot::TelemetryLeaseSlot()
    : sessionId(0U),
      streamId(0U),
      streamEpoch(0U),
      intervalMillis(0U),
      expiresAtMillis(0U),
      nextSampleAtMillis(0U),
      sampleSequence(0U),
      fields(),
      transportId(0U),
      fieldCount(0U),
      occupied(false) {}

TelemetryLeaseTable::TelemetryLeaseTable()
    : slots_(), nextEpoch_(1U) {}

Result TelemetryLeaseTable::apply(
    const bbp2::TelemetryControlBody& control,
    const RxContext& source,
    const EndpointRegistry& endpoints,
    uint32_t nowMillis,
    bbp2::TelemetryStatusBody& status) {
    if (source.transportId >= BLINKER_MAX_TRANSPORTS ||
        control.streamId == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    TelemetryLeaseSlot* lease = find(source, control.streamId);
    if (control.operation == bbp2::TelemetryControlOperation::Renew ||
        control.operation == bbp2::TelemetryControlOperation::Close) {
        if (lease == nullptr) return Result::failure(ErrorCode::NotFound);
        if (lease->streamEpoch != control.streamEpoch) {
            return Result::failure(ErrorCode::SequenceConflict);
        }
        if (control.operation == bbp2::TelemetryControlOperation::Close) {
            fillStatus(*lease, bbp2::TelemetryStatusCode::Closed, 0U, status);
            close(*lease);
            return Result::success();
        }
        if (control.leaseMillis < kTelemetryMinimumLeaseMillis ||
            control.leaseMillis > kTelemetryMaximumLeaseMillis) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        lease->expiresAtMillis = nowMillis + control.leaseMillis;
        fillStatus(
            *lease,
            bbp2::TelemetryStatusCode::Renewed,
            control.leaseMillis,
            status);
        return Result::success();
    }

    if (control.operation != bbp2::TelemetryControlOperation::Open ||
        control.leaseMillis < kTelemetryMinimumLeaseMillis ||
        control.leaseMillis > kTelemetryMaximumLeaseMillis ||
        control.intervalMillis < 100U ||
        control.intervalMillis > kTelemetryMaximumIntervalMillis ||
        control.fieldCount == 0U ||
        control.fieldCount > bbp2::kMaxTelemetryFields) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }

    uint32_t effectiveInterval = control.intervalMillis;
    uint8_t requested[(BLINKER_DEVICE_MAX_FIELDS + 7U) / 8U] = {};
    for (uint8_t index = 0U; index < control.fieldCount; ++index) {
        const uint16_t fieldId = control.fieldIds[index];
        if (fieldId == 0U || fieldId > BLINKER_DEVICE_MAX_FIELDS) {
            return Result::failure(ErrorCode::CapacityExceeded);
        }
        const EndpointDescriptor* endpoint = endpoints.findById(fieldId);
        if (endpoint == nullptr || endpoint->kind != EndpointKind::Property ||
            (endpoint->access & AccessRead) == 0U ||
            endpoint->telemetryMinimumIntervalMs == 0U) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (endpoint->telemetryMinimumIntervalMs > effectiveInterval) {
            effectiveInterval = endpoint->telemetryMinimumIntervalMs;
        }
        uint8_t& selected = requested[(fieldId - 1U) / 8U];
        const uint8_t mask = static_cast<uint8_t>(
            1U << ((fieldId - 1U) % 8U));
        if ((selected & mask) != 0U) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        selected = static_cast<uint8_t>(selected | mask);
    }

    if (lease != nullptr) {
        if (lease->fieldCount != control.fieldCount ||
            lease->intervalMillis != effectiveInterval ||
            memcmp(lease->fields, requested, sizeof(requested)) != 0) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        // Identical Open is idempotent, including after a routed retry.
        lease->expiresAtMillis = nowMillis + control.leaseMillis;
        fillStatus(
            *lease,
            bbp2::TelemetryStatusCode::Opened,
            control.leaseMillis,
            status);
        return Result::success();
    }

    lease = allocate();
    if (lease == nullptr) return Result::failure(ErrorCode::CapacityExceeded);
    lease->transportId = source.transportId;
    lease->sessionId = source.sessionId;
    lease->streamId = control.streamId;
    lease->streamEpoch = nextEpoch();
    lease->intervalMillis = effectiveInterval;
    lease->expiresAtMillis = nowMillis + control.leaseMillis;
    lease->nextSampleAtMillis = nowMillis;
    lease->sampleSequence = 0U;
    lease->fieldCount = control.fieldCount;
    memcpy(lease->fields, requested, sizeof(requested));
    lease->occupied = true;
    fillStatus(
        *lease,
        bbp2::TelemetryStatusCode::Opened,
        control.leaseMillis,
        status);
    return Result::success();
}

TelemetryLeaseSlot* TelemetryLeaseTable::firstExpired(uint32_t nowMillis) {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied &&
            reached(nowMillis, slots_[index].expiresAtMillis)) {
            return &slots_[index];
        }
    }
    return nullptr;
}

TelemetryLeaseSlot* TelemetryLeaseTable::firstDue(uint32_t nowMillis) {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied &&
            !reached(nowMillis, slots_[index].expiresAtMillis) &&
            reached(nowMillis, slots_[index].nextSampleAtMillis)) {
            return &slots_[index];
        }
    }
    return nullptr;
}

void TelemetryLeaseTable::deferSource(
    const RxContext& source,
    uint32_t nowMillis,
    uint32_t durationMillis) {
    if (durationMillis == 0U) return;
    const uint32_t deadline = nowMillis + durationMillis;
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        TelemetryLeaseSlot& lease = slots_[index];
        if (lease.occupied && sameSource(lease, source) &&
            !reached(lease.nextSampleAtMillis, deadline)) {
            lease.nextSampleAtMillis = deadline;
        }
    }
}

void TelemetryLeaseTable::sampled(
    TelemetryLeaseSlot& lease,
    uint32_t nowMillis) {
    if (!lease.occupied) return;
    ++lease.sampleSequence;
    if (lease.sampleSequence == 0U) ++lease.sampleSequence;
    lease.nextSampleAtMillis = nowMillis + lease.intervalMillis;
}

void TelemetryLeaseTable::close(TelemetryLeaseSlot& lease) {
    lease = TelemetryLeaseSlot();
}

void TelemetryLeaseTable::sessionClosed(const RxContext& source) {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied && sameSource(slots_[index], source)) {
            close(slots_[index]);
        }
    }
}

void TelemetryLeaseTable::transportClosed(uint8_t transportId) {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied &&
            slots_[index].transportId == transportId) {
            close(slots_[index]);
        }
    }
}

void TelemetryLeaseTable::reset() {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        slots_[index] = TelemetryLeaseSlot();
    }
    nextEpoch_ = 1U;
}

size_t TelemetryLeaseTable::activeCount() const {
    size_t count = 0U;
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied) ++count;
    }
    return count;
}

bool TelemetryLeaseTable::selected(
    const TelemetryLeaseSlot& lease,
    uint16_t fieldId) {
    return fieldId != 0U && fieldId <= BLINKER_DEVICE_MAX_FIELDS &&
           (lease.fields[(fieldId - 1U) / 8U] &
            static_cast<uint8_t>(1U << ((fieldId - 1U) % 8U))) != 0U;
}

TelemetryLeaseSlot* TelemetryLeaseTable::find(
    const RxContext& source,
    uint32_t streamId) {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (slots_[index].occupied && slots_[index].streamId == streamId &&
            sameSource(slots_[index], source)) {
            return &slots_[index];
        }
    }
    return nullptr;
}

TelemetryLeaseSlot* TelemetryLeaseTable::allocate() {
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        if (!slots_[index].occupied) return &slots_[index];
    }
    return nullptr;
}

uint32_t TelemetryLeaseTable::nextEpoch() {
    uint32_t epoch = nextEpoch_++;
    if (epoch == 0U) epoch = nextEpoch_++;
    if (nextEpoch_ == 0U) nextEpoch_ = 1U;
    return epoch;
}

} // namespace blinker
