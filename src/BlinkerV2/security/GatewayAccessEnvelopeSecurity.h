#ifndef BLINKER_SECURITY_GATEWAYACCESSENVELOPESECURITY_H
#define BLINKER_SECURITY_GATEWAYACCESSENVELOPESECURITY_H

#include "../identity/DeviceKey.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"
#include "../protocol/gateway/Contracts.h"

namespace blinker {

struct GatewayAccessWrapContext {
    StringView environmentId;
    StringView tenantId;
    StringView edgeHubLogicalDeviceId;
    uint32_t deviceKeyVersion;

    GatewayAccessWrapContext()
        : environmentId(), tenantId(), edgeHubLogicalDeviceId(),
          deviceKeyVersion(0U) {}
};

Result deriveGatewayAccessWrapKey(
    const DeviceKey& deviceKey,
    const GatewayAccessWrapContext& context,
    MutableByteSpan output);

// Opens one exact GatewayAccessEnvelopeV2. All crypto workspaces are
// caller-owned and temporary material is cleared before return.
Result openGatewayAccessEnvelope(
    const DeviceKey& deviceKey,
    const gateway::GatewayAccessEnvelopeView& envelope,
    IX25519AesGcmCryptoProvider& crypto,
    MutableByteSpan aadScratch,
    MutableByteSpan plaintext);

} // namespace blinker

#endif
