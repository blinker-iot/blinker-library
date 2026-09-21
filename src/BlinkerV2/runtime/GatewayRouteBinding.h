#ifndef BLINKER_RUNTIME_GATEWAYROUTEBINDING_H
#define BLINKER_RUNTIME_GATEWAYROUTEBINDING_H

#include "../identity/GatewayChildRecords.h"
#include <string.h>

namespace blinker {
namespace gateway_route_binding_detail {

// Selection is not authorization. The bridge still verifies topology,
// operation, expiry, projection and the current durable credential at dispatch.
template <typename Target>
bool matches(const Target& target, const GatewayChildIdentity& record) {
    return record.state == GatewayChildRecordState::Active &&
        target.childLogicalDeviceId.size == record.logicalIdSize &&
        target.childLogicalDeviceId.data != nullptr &&
        target.childDeviceInstanceId.size == sizeof(record.instance) &&
        target.childDeviceInstanceId.data != nullptr &&
        target.accessEpoch == record.accessEpoch &&
        target.gatewayCredentialVersion == record.credentialVersion &&
        memcmp(target.childLogicalDeviceId.data, record.logicalId,
               target.childLogicalDeviceId.size) == 0 &&
        memcmp(target.childDeviceInstanceId.data, record.instance,
               sizeof(record.instance)) == 0;
}
} // namespace gateway_route_binding_detail

inline bool gatewayRouteMatchesChild(ByteView bytes, GatewayChildRecords& records) {
    if (bytes.data == nullptr || bytes.size < 3U) return false;
    const GatewayChildIdentity* identity = nullptr;
    if (!records.identity(identity)) return false;
    using namespace gateway_route_binding_detail;
    // Reuse canonical decoders; no partial CBOR scan or MAC-based identity.
    if ((bytes.data[0] == 0x89U || bytes.data[0] == 0x8aU) && bytes.data[2] == 1U) {
        gateway::GatewayRouteAdmissionView value;
        return gateway::decodeGatewayRouteAdmission(bytes, value) && matches(value, *identity);
    }
    if (bytes.data[0] == 0x89U) {
        gateway::GatewayRouteClosureView value;
        return gateway::decodeGatewayRouteClosure(bytes, value) && matches(value, *identity);
    }
    if (bytes.data[0] == 0x8aU) {
        gateway::GatewayRouteLeaseView value;
        return gateway::decodeGatewayRouteLease(bytes, value) && matches(value.route, *identity);
    }
    if (bytes.data[0] == 0x8bU) {
        gateway::GatewayExecutionDemandView value;
        return gateway::decodeGatewayExecutionDemand(bytes, value) && matches(value, *identity);
    }
    if (bytes.data[0] == 0x8cU) {
        gateway::GatewayDirectYieldView value;
        return gateway::decodeGatewayDirectYield(bytes, value) && matches(value.demand, *identity);
    }
    gateway::ChildRouteEnvelopeView value;
    return gateway::decodeChildRouteEnvelope(bytes, value) && matches(value, *identity);
}

} // namespace blinker
#endif
