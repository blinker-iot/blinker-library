#ifndef BLINKER_LOCATION_SNAPSHOT_H
#define BLINKER_LOCATION_SNAPSHOT_H

#include "LocationFix.h"

namespace blinker {

// Boot-local, caller-owned single snapshot; no driver, timers, storage or sends.
// All observations must be <2^31 ms apart. Clear after reboot/tick reset or
// unmeasured sleep; a 32-bit tick cannot detect an unobserved full wrap.
class LocationSnapshot {
public:
    explicit LocationSnapshot(uint32_t maximumAgeMillis = 30000U)
        : fix_(), maximumAgeMillis_(maximumAgeMillis), sampledAtMillis_(0U),
          hasFix_(false), available_(false) {}

    Result update(const LocationFix& fix, uint32_t sampledAtMillis, uint32_t nowMillis) {
        if (!configured()) return Result::failure(ErrorCode::InvalidArgument);
        const Result valid = validateLocationFix(fix);
        if (!valid) return valid;
        const uint32_t age = nowMillis - sampledAtMillis;
        if (age >= kLocationTickHalfRange || age > maximumAgeMillis_) {
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (hasFix_) {
            const uint32_t elapsed = sampledAtMillis - sampledAtMillis_;
            if (elapsed == 0U || elapsed >= kLocationTickHalfRange) {
                return Result::failure(ErrorCode::SequenceConflict);
            }
        }
        fix_ = fix;
        sampledAtMillis_ = sampledAtMillis;
        hasFix_ = available_ = true;
        return Result::success();
    }

    Result read(uint32_t nowMillis, LocationSample& sample) {
        if (!configured()) return Result::failure(ErrorCode::InvalidArgument);
        if (!hasFix_) return Result::failure(ErrorCode::NotFound);
        if (!available_) return Result::failure(ErrorCode::WouldBlock);
        const uint32_t age = nowMillis - sampledAtMillis_;
        if (age >= kLocationTickHalfRange) {
            available_ = false;
            return Result::failure(ErrorCode::ValueOutOfRange);
        }
        if (age > maximumAgeMillis_) {
            available_ = false; // Never revive an expired fix after a later tick wrap.
            return Result::failure(ErrorCode::WouldBlock);
        }
        LocationSample current;
        current.fix = fix_;
        current.ageMillis = age;
        sample = current;
        return Result::success();
    }

    // Loss of satellite fix does not turn the previous coordinates into (0,0).
    void invalidate() { available_ = false; }
    // Borrowed until update/clear; copy before handing it to asynchronous code.
    const LocationFix* lastFix() const { return hasFix_ ? &fix_ : nullptr; }
    void clear() {
        fix_ = LocationFix();
        sampledAtMillis_ = 0U;
        hasFix_ = available_ = false;
    }

private:
    bool configured() const {
        return maximumAgeMillis_ != 0U && maximumAgeMillis_ < kLocationTickHalfRange;
    }

    LocationFix fix_;
    uint32_t maximumAgeMillis_;
    uint32_t sampledAtMillis_;
    bool hasFix_;
    bool available_;
};

} // namespace blinker

#endif
