#ifndef BLINKER_RUNTIME_GATEWAYACCESSDELIVERYPROCESSOR_H
#define BLINKER_RUNTIME_GATEWAYACCESSDELIVERYPROCESSOR_H

#include "../identity/GatewayAccessStore.h"
#include "../interface/IClock.h"
#include "../interface/IDeviceKeySource.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"

namespace blinker {

struct GatewayAccessDeliveryContext {
    // Optional defense-in-depth expectations. Empty views mean the product
    // relies on the unique DeviceKey/current version AEAD binding and the
    // authenticated Edge Hub Broker ACL; envelope identifiers remain covered
    // by the exact KDF/AAD either way.
    StringView environmentId;
    StringView tenantId;
    StringView edgeHubLogicalDeviceId;
    uint32_t deviceKeyVersion;

    GatewayAccessDeliveryContext()
        : environmentId(), tenantId(), edgeHubLogicalDeviceId(),
          deviceKeyVersion(0U) {}
};

// Device-side durable half of the Edge Hub management protocol. The caller
// owns MQTTS/topic/QoS. This processor never emits an ACK before authenticated
// plaintext has been committed and verified in protected storage.
class GatewayAccessDeliveryProcessor {
public:
    GatewayAccessDeliveryProcessor(
        GatewayAccessStore& access,
        IDeviceKeySource& deviceKey,
        IX25519AesGcmCryptoProvider& crypto,
        IClock& clock)
        : access_(access), deviceKey_(deviceKey), crypto_(crypto),
          clock_(clock) {}

    Result process(
        const GatewayAccessDeliveryContext& context,
        ByteView exactEnvelope,
        MutableByteSpan ackOutput,
        size_t& ackSize);

private:
    GatewayAccessStore& access_;
    IDeviceKeySource& deviceKey_;
    IX25519AesGcmCryptoProvider& crypto_;
    IClock& clock_;

    GatewayAccessDeliveryProcessor(
        const GatewayAccessDeliveryProcessor&);
    GatewayAccessDeliveryProcessor& operator=(
        const GatewayAccessDeliveryProcessor&);
};

} // namespace blinker

#endif
