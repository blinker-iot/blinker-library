#ifndef BLINKER_LOCATION_H
#define BLINKER_LOCATION_H

#include "../api/Interactions.h"
#include "CloudLocationBinding.h"

namespace blinker {

// Immutable schema input. Object maxLength is its CBOR map pair count, not
// bytes. A declaration is not cloud authorization or a request to sample.
class LocationSpec {
public:
    enum : size_t { fieldCount = 1U };

    constexpr explicit LocationSpec(StringView key,
        uint32_t minimumIntervalMillis = 1000U,
        uint32_t maximumAgeMillis = 30000U)
        : constraints_(ConstraintMaxLength, 0.0, 0.0, 0.0, 8U,
              StringView(), nullptr, 0U),
          field_(key, EndpointKind::Property, ValueType::Object, AccessRead,
              &constraints_, minimumIntervalMillis),
          maximumAgeMillis_(maximumAgeMillis) {}

    const FieldSpec* fieldAt(size_t index) const {
        return index == 0U ? &field_ : nullptr;
    }
    const FieldSpec& field() const { return field_; }
    uint32_t maximumAgeMillis() const { return maximumAgeMillis_; }

private:
    bool configured() const {
        return field_.telemetryMinimumIntervalMs >= 100U &&
               field_.telemetryMinimumIntervalMs <= 60000U &&
               maximumAgeMillis_ != 0U &&
               maximumAgeMillis_ < kLocationTickHalfRange;
    }

    PropertyConstraints constraints_;
    FieldSpec field_;
    uint32_t maximumAgeMillis_;

    LocationSpec(const LocationSpec&) = delete;
    LocationSpec& operator=(const LocationSpec&) = delete;
    friend class Location;
};

// Caller-owned, one boot-local observation and the existing private Cloud
// binding. Keep this and its Spec alive for the entire Device/Sketch session.
// Driver/parsing remains user-owned. Call inputs and Blinker.run serially in
// the Sketch loop, not from an IRQ or another task. No heap, timers or sends.
class Location {
public:
    enum : size_t { fieldCount = 1U };

    explicit Location(const LocationSpec& spec)
        : snapshot_(spec.maximumAgeMillis()), binding_(snapshot_),
          spec_(&spec), client_(nullptr) {}

    Location(LocationSpec&&) = delete;
    Location(const LocationSpec&&) = delete;
    Location(const Location&) = delete;
    Location& operator=(const Location&) = delete;
    Location(Location&&) = delete;
    Location& operator=(Location&&) = delete;

    const FieldSpec* fieldAt(size_t index) const { return spec_->fieldAt(index); }

private:
    Result bind(Client& client) {
        if (client_ != nullptr) {
            return client_ == &client ? Result::success()
                                     : Result::failure(ErrorCode::AlreadyExists);
        }
        if (!spec_->configured()) return Result::failure(ErrorCode::InvalidArgument);
        Result result = interaction_detail::validateBinding(client, spec_->field(), client_);
        if (result) result = binding_.attach(client, spec_->field().key);
        if (!result) client_ = nullptr;
        return result;
    }

    bool configured() const { return client_ != nullptr && client_->clock_ != nullptr; }

    Result update(const LocationFix& observation) {
        return updateSnapshot(observation, false, 0U);
    }
    Result update(const LocationFix& observation, uint32_t sampledAtMillis) {
        return updateSnapshot(observation, true, sampledAtMillis);
    }
    Result updateSnapshot(const LocationFix& observation, bool hasSampleTime,
        uint32_t sampledAtMillis) {
        if (!configured()) return Result::failure(ErrorCode::NotConfigured);
        const uint32_t now = client_->clock_->monotonicMillis();
        return snapshot_.update(observation, hasSampleTime ? sampledAtMillis : now, now);
    }

    Result update(double latitude, double longitude) {
        return updateDegrees(latitude, longitude, false, 0U);
    }
    Result update(double latitude, double longitude, uint32_t sampledAtMillis) {
        return updateDegrees(latitude, longitude, true, sampledAtMillis);
    }
    Result updateDegrees(double latitude, double longitude, bool hasSampleTime,
        uint32_t sampledAtMillis) {
        if (!configured()) return Result::failure(ErrorCode::NotConfigured);
        // Ordered comparisons also reject NaN and infinities before conversion.
        if (!(latitude >= -90.0 && latitude <= 90.0) ||
            !(longitude >= -180.0 && longitude <= 180.0)) {
            return Result::failure(ErrorCode::InvalidArgument);
        }
        LocationFix observation;
        const double lat = latitude * 10000000.0;
        const double lon = longitude * 10000000.0;
        observation.latitudeE7 = static_cast<int32_t>(lat >= 0.0 ? lat + 0.5 : lat - 0.5);
        observation.longitudeE7 = static_cast<int32_t>(lon >= 0.0 ? lon + 0.5 : lon - 0.5);
        observation.valid = true; // Accuracy and UTC deliberately remain unknown.
        return updateSnapshot(observation, hasSampleTime, sampledAtMillis);
    }

    Result loseFix() {
        if (!configured()) return Result::failure(ErrorCode::NotConfigured);
        snapshot_.invalidate();
        return Result::success();
    }
    Result clear() {
        if (!configured()) return Result::failure(ErrorCode::NotConfigured);
        snapshot_.clear();
        return Result::success();
    }

    LocationSnapshot snapshot_;
    CloudLocationBinding binding_;
    const LocationSpec* spec_;
    Client* client_;

    friend class Device;
    friend class BlinkerClass;
};

} // namespace blinker

#define BLINKER_LOCATION(name)                                                \
    static const ::blinker::LocationSpec name##Spec(#name);                   \
    ::blinker::Location name(name##Spec)

#define BLINKER_LOCATION_KEY(name, keyLiteral)                                 \
    static const ::blinker::LocationSpec name##Spec(keyLiteral);              \
    ::blinker::Location name(name##Spec)

#endif
