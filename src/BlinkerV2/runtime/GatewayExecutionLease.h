#ifndef BLINKER_RUNTIME_GATEWAYEXECUTIONLEASE_H
#define BLINKER_RUNTIME_GATEWAYEXECUTIONLEASE_H

#include <string.h>
#include "../core/SecureMemory.h"
#include "../interface/IRandom.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

// Volatile receiver-owned freshness only: no BLE, MQTT, wall clock, authority,
// callbacks or queues. Adapter must validate exact admission/access/session
// before apply, clear on retirement, and poll while the MCU is running.
class GatewayExecutionLease {
public:
    GatewayExecutionLease()
        : incarnation_(), challengeId_(0U), issuedAt_(0U), activeIssuedAt_(0U),
          activeMillis_(0U), activeId_(0U), initialized_(false), pending_(false) {}

    Result begin(IRandom& random) {
        clear();
        Result result = random.fill(MutableByteSpan(incarnation_, sizeof(incarnation_)));
        uint8_t bits = 0U;
        for (size_t i = 0U; i < sizeof(incarnation_); ++i) bits |= incarnation_[i];
        if (!result || bits == 0U) {
            clear();
            return result ? Result::failure(ErrorCode::NotConfigured) : result;
        }
        initialized_ = true;
        return Result::success();
    }

    void clear() {
        secureZero(MutableByteSpan(incarnation_, sizeof(incarnation_)));
        challengeId_ = issuedAt_ = activeIssuedAt_ = activeMillis_ = activeId_ = 0U;
        initialized_ = pending_ = false;
    }

    // Separate control exchanges may share a current route incarnation while
    // retaining independent challenge counters. No authority is implied by it.
    Result begin(ByteView incarnation) {
        uint8_t copy[gateway::kGatewayExecutionIncarnationSize];
        if (incarnation.data == nullptr || incarnation.size != sizeof(copy)) {
            clear(); return Result::failure(ErrorCode::InvalidArgument);
        }
        memcpy(copy, incarnation.data, sizeof(copy)); clear();
        uint8_t bits = 0U;
        for (size_t i = 0U; i < sizeof(copy); ++i) bits |= copy[i];
        if (bits != 0U) { memcpy(incarnation_, copy, sizeof(copy)); initialized_ = true; }
        secureZero(MutableByteSpan(copy, sizeof(copy)));
        return initialized_ ? Result::success() : Result::failure(ErrorCode::InvalidArgument);
    }

    // An unexpired request is reused byte-for-byte, never given a later anchor.
    // Probe invokes only this method; it cannot set wanted().
    Result request(uint32_t now) {
        poll(now);
        if (!initialized_) return Result::failure(ErrorCode::NotConfigured);
        if (pending_) return Result::failure(ErrorCode::AlreadyExists);
        if (challengeId_ == UINT32_MAX) return Result::failure(ErrorCode::CapacityExceeded);
        // Also bounds successive denied/probe exchanges; no hot-loop storm.
        if (challengeId_ != 0U && static_cast<uint32_t>(now - issuedAt_) < 1000U) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        ++challengeId_;
        issuedAt_ = now;
        pending_ = true;
        return Result::success();
    }

    Result apply(ByteView incarnation, uint32_t id, uint32_t leaseMillis, uint32_t now) {
        poll(now);
        if (!initialized_ || incarnation.size != sizeof(incarnation_) || incarnation.data == nullptr ||
            memcmp(incarnation_, incarnation.data, sizeof(incarnation_)) != 0 || id == 0U ||
            id > challengeId_ || leaseMillis > gateway::kGatewayExecutionMaximumLeaseMillis) {
            return Result::failure(ErrorCode::StateConflict);
        }
        if (leaseMillis == 0U) {
            // A zero decision may overtake its positive reply. Old releases
            // cannot cancel a newer challenge/lease; no tombstone collection.
            if (id == challengeId_) pending_ = false;
            if (id >= activeId_) activeMillis_ = 0U;
            return Result::success();
        }
        if (!pending_ || id != challengeId_) return Result::failure(ErrorCode::SequenceConflict);
        pending_ = false; // Each challenge is consumed once, including an expired grant.
        if (static_cast<uint32_t>(now - issuedAt_) >= leaseMillis) {
            return Result::failure(ErrorCode::StateConflict);
        }
        activeId_ = id;
        activeIssuedAt_ = issuedAt_;
        activeMillis_ = leaseMillis;
        return Result::success();
    }

    void poll(uint32_t now) {
        if (pending_ && static_cast<uint32_t>(now - issuedAt_) >= gateway::kGatewayExecutionChallengeMillis) {
            pending_ = false;
        }
        if (activeMillis_ != 0U && static_cast<uint32_t>(now - activeIssuedAt_) >= activeMillis_) {
            activeMillis_ = 0U;
        }
    }
    bool wanted(uint32_t now) { poll(now); return activeMillis_ != 0U; }
    uint32_t remainingMillis(uint32_t now) {
        return wanted(now) ? activeMillis_ - static_cast<uint32_t>(now - activeIssuedAt_) : 0U;
    }
    bool pending(uint32_t now) { poll(now); return pending_; }
    uint32_t challengeId() const { return challengeId_; }
    ByteView incarnation() const { return ByteView(incarnation_, sizeof(incarnation_)); }
    uint32_t activeChallengeId() const { return activeId_; }
    uint32_t activeMillis() const { return activeMillis_; }
    bool matchesIncarnation(ByteView incarnation) const {
        return initialized_ && incarnation.data != nullptr && incarnation.size == sizeof(incarnation_) &&
            memcmp(incarnation.data, incarnation_, sizeof(incarnation_)) == 0;
    }
    bool matchesActive(ByteView incarnation, uint32_t id, uint32_t millis, uint32_t now) {
        return wanted(now) && id == activeId_ && millis == activeMillis_ && matchesIncarnation(incarnation);
    }

private:
    uint8_t incarnation_[gateway::kGatewayExecutionIncarnationSize];
    uint32_t challengeId_, issuedAt_, activeIssuedAt_, activeMillis_, activeId_;
    bool initialized_, pending_;
    GatewayExecutionLease(const GatewayExecutionLease&);
    GatewayExecutionLease& operator=(const GatewayExecutionLease&);
};

static_assert(sizeof(GatewayExecutionLease) <= 40U, "Execution freshness must stay within 40 bytes");

} // namespace blinker
#endif
