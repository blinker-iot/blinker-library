#ifndef BLINKER_RUNTIME_GATEWAYCREDENTIALRENEWALDELIVERYPROCESSOR_H
#define BLINKER_RUNTIME_GATEWAYCREDENTIALRENEWALDELIVERYPROCESSOR_H

#include "../identity/GatewayAccessStore.h"
#include "../identity/GatewayCredentialRenewalStore.h"
#include "../interface/IClock.h"
#include "../interface/IDeviceKeySource.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"
#include "IGatewayManagementDelivery.h"

namespace blinker {

// Receives one sealed N->N+1 candidate into the Edge-Hub-only protected
// journal. It never mutates the active access record and never emits an ACK
// before reopening the exact durable pending record succeeds.
class GatewayCredentialRenewalDeliveryProcessor final
    : public IGatewayManagementDelivery {
public:
    GatewayCredentialRenewalDeliveryProcessor(
        GatewayAccessStore& active,
        GatewayCredentialRenewalStore& pending,
        IDeviceKeySource& deviceKey,
        IX25519AesGcmCryptoProvider& crypto,
        IClock& clock)
        : active_(active), pending_(pending), deviceKey_(deviceKey),
          crypto_(crypto), clock_(clock) {}

    Result process(
        const GatewayAccessDeliveryContext& context,
        ByteView exactEnvelope,
        MutableByteSpan ackOutput,
        size_t& ackSize) override;

private:
    GatewayAccessStore& active_;
    GatewayCredentialRenewalStore& pending_;
    IDeviceKeySource& deviceKey_;
    IX25519AesGcmCryptoProvider& crypto_;
    IClock& clock_;

    GatewayCredentialRenewalDeliveryProcessor(
        const GatewayCredentialRenewalDeliveryProcessor&);
    GatewayCredentialRenewalDeliveryProcessor& operator=(
        const GatewayCredentialRenewalDeliveryProcessor&);
};

} // namespace blinker

#endif
