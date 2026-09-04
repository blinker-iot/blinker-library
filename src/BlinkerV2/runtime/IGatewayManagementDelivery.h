#ifndef BLINKER_RUNTIME_IGATEWAYMANAGEMENTDELIVERY_H
#define BLINKER_RUNTIME_IGATEWAYMANAGEMENTDELIVERY_H

#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

struct GatewayAccessDeliveryContext {
    // Optional defense-in-depth expectations. Empty views mean the product
    // relies on DeviceKey/current-version AEAD and the authenticated Hub ACL.
    StringView environmentId;
    StringView tenantId;
    StringView edgeHubLogicalDeviceId;
    uint32_t deviceKeyVersion;

    GatewayAccessDeliveryContext()
        : environmentId(), tenantId(), edgeHubLogicalDeviceId(),
          deviceKeyVersion(0U) {}
};

// One authenticated management-topic delivery handler. MQTT/QoS buffering is
// owned by GatewayManagementClient; processors own only validation, crypto,
// durable commit and the exact application ACK.
class IGatewayManagementDelivery {
public:
    virtual ~IGatewayManagementDelivery() {}
    virtual Result process(
        const GatewayAccessDeliveryContext& context,
        ByteView exactEnvelope,
        MutableByteSpan ackOutput,
        size_t& ackSize) = 0;
};

} // namespace blinker

#endif
