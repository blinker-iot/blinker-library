#ifndef BLINKER_RUNTIME_GATEWAYMANAGEMENTDELIVERYMUX_H
#define BLINKER_RUNTIME_GATEWAYMANAGEMENTDELIVERYMUX_H

#include "../protocol/gateway/Contracts.h"
#include "IGatewayManagementDelivery.h"

namespace blinker {

// Exact wire dispatch only. Initial access and active renewal retain separate
// crypto/storage processors and therefore cannot accidentally share keys,
// records or state transitions.
class GatewayManagementDeliveryMux final
    : public IGatewayManagementDelivery {
public:
    GatewayManagementDeliveryMux(
        IGatewayManagementDelivery& initialAccess,
        IGatewayManagementDelivery& credentialRenewal)
        : initialAccess_(initialAccess),
          credentialRenewal_(credentialRenewal) {}

    Result process(
        const GatewayAccessDeliveryContext& context,
        ByteView exactEnvelope,
        MutableByteSpan ackOutput,
        size_t& ackSize) override {
        gateway::GatewayCredentialRenewalEnvelopeView renewal;
        const Result decoded =
            gateway::decodeGatewayCredentialRenewalEnvelope(
                exactEnvelope, renewal);
        return decoded
                   ? credentialRenewal_.process(
                         context, exactEnvelope, ackOutput, ackSize)
                   : initialAccess_.process(
                         context, exactEnvelope, ackOutput, ackSize);
    }

private:
    IGatewayManagementDelivery& initialAccess_;
    IGatewayManagementDelivery& credentialRenewal_;
};

} // namespace blinker

#endif
