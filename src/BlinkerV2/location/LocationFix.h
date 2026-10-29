#ifndef BLINKER_LOCATION_FIX_H
#define BLINKER_LOCATION_FIX_H

#include "../core/WallClock.h"

namespace blinker {

static const int32_t kLocationLatitudeLimitE7 = 900000000;
static const int32_t kLocationLongitudeLimitE7 = 1800000000;
static const uint32_t kLocationTickHalfRange = 0x80000000U;

// Provenance reported by the GNSS adapter, not authorization or a clock anchor.
enum class LocationTimeQuality : uint8_t { Unknown = 0, GnssUtc = 1 };

// One device GNSS observation in WGS84. Default construction is NOT a fix;
// (0,0) with valid=true is a legitimate point. Drivers remain Sketch-owned.
struct LocationFix {
    uint64_t capturedAtUtcMillis;
    int32_t latitudeE7;
    int32_t longitudeE7;
    uint32_t horizontalAccuracyCm;
    LocationTimeQuality timeQuality;
    bool hasHorizontalAccuracy;
    bool valid;

    LocationFix()
        : capturedAtUtcMillis(0U), latitudeE7(0), longitudeE7(0),
          horizontalAccuracyCm(0U), timeQuality(LocationTimeQuality::Unknown),
          hasHorizontalAccuracy(false), valid(false) {}
};

struct LocationSample {
    LocationFix fix;
    // Age at encoding, not a server timestamp or a cross-boot tick.
    uint32_t ageMillis;

    LocationSample() : fix(), ageMillis(0U) {}
};

inline Result validateLocationFix(const LocationFix& fix) {
    if (!fix.valid || fix.latitudeE7 < -kLocationLatitudeLimitE7 ||
        fix.latitudeE7 > kLocationLatitudeLimitE7 ||
        fix.longitudeE7 < -kLocationLongitudeLimitE7 ||
        fix.longitudeE7 > kLocationLongitudeLimitE7 ||
        (!fix.hasHorizontalAccuracy && fix.horizontalAccuracyCm != 0U)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    switch (fix.timeQuality) {
        case LocationTimeQuality::Unknown:
            if (fix.capturedAtUtcMillis != 0U) {
                return Result::failure(ErrorCode::InvalidArgument);
            }
            break;
        case LocationTimeQuality::GnssUtc:
            if (fix.capturedAtUtcMillis < kWallTimeMinimumMillis ||
                fix.capturedAtUtcMillis >= kWallTimeEndMillis) {
                return Result::failure(ErrorCode::ValueOutOfRange);
            }
            break;
        default:
            return Result::failure(ErrorCode::UnsupportedFeature);
    }
    return Result::success();
}

inline Result validateLocationSample(const LocationSample& sample) {
    const Result result = validateLocationFix(sample.fix);
    if (!result) return result;
    return sample.ageMillis < kLocationTickHalfRange
        ? Result::success() : Result::failure(ErrorCode::ValueOutOfRange);
}

} // namespace blinker

#endif
