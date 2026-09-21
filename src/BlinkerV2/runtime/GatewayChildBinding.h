#ifndef BLINKER_RUNTIME_GATEWAYCHILDBINDING_H
#define BLINKER_RUNTIME_GATEWAYCHILDBINDING_H

#include "../identity/GatewayChildRecords.h"
#include "GatewayAccessDeliveryProcessor.h"
#include "GatewayCredentialRenewalDeliveryProcessor.h"
#include "GatewayManagementDeliveryMux.h"

namespace blinker {

// Lightweight durable binding ingress. A disconnected child can receive its
// sealed credentials without allocating a GATT link/handshake/route workspace.
struct GatewayChildBinding {
    GatewayChildRecords& records;
    GatewayAccessDeliveryProcessor initial;
    GatewayCredentialRenewalDeliveryProcessor renewal;
    GatewayManagementDeliveryMux delivery;

    GatewayChildBinding(GatewayChildRecords& stores, IDeviceKeySource& deviceKey,
                        IX25519AesGcmCryptoProvider& crypto, IClock& clock)
        : records(stores), initial(stores.access, deviceKey, crypto, clock),
          renewal(stores.access, stores.renewal, deviceKey, crypto, clock), delivery(initial, renewal) {}

    GatewayChildBinding(const GatewayChildBinding&) = delete;
    GatewayChildBinding& operator=(const GatewayChildBinding&) = delete;
};

} // namespace blinker
#endif
