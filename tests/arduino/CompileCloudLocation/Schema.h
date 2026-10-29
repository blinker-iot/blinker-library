#ifndef BLINKER_TEST_CLOUD_LOCATION_SCHEMA_H
#define BLINKER_TEST_CLOUD_LOCATION_SCHEMA_H

#include <BlinkerV2/model/Endpoint.h>

// One portable source of truth for the internal candidate and its host probe.
// The eight-pair Object is private GNSS Telemetry, never ordinary State.
namespace blinker { namespace cloud_location_candidate {

static const uint32_t manifestRevision = 1U;
static const uint16_t locationEndpointId = 3U;
static const char locationKey[] = "position";
static const PropertyConstraints locationConstraints(
    ConstraintMaxLength, 0.0, 0.0, 0.0, 8U, StringView(), nullptr, 0U);
static const FieldSpec fields[] = {
    FieldSpec("power", EndpointKind::Property, ValueType::Boolean,
        AccessRead | AccessWrite | AccessNotify),
    FieldSpec("uptimeSeconds", EndpointKind::Property, ValueType::UnsignedInteger,
        AccessRead, nullptr, 1000U),
    FieldSpec(locationKey, EndpointKind::Property, ValueType::Object,
        AccessRead, &locationConstraints, 1000U)
};
static const size_t fieldCount = sizeof(fields) / sizeof(fields[0]);

} } // namespace blinker::cloud_location_candidate

#endif
