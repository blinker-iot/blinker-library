#ifndef BLINKER_SECURITY_WIFIPROVREKEYPROOF_H
#define BLINKER_SECURITY_WIFIPROVREKEYPROOF_H

#include "../identity/DeviceInstanceId.h"
#include "../identity/DeviceKey.h"
#include "../identity/ControllerCredential.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"

namespace blinker {
namespace security {

enum : size_t {
    kWifiprovRekeyOperationSize = 16U,
    kWifiprovRekeyChallengeSize = 16U,
    kWifiprovRekeyOwnerNonceSize = 16U,
    kWifiprovRekeyMacSize = 32U,
    kWifiprovRekeyBoxSize = static_cast<size_t>(kDeviceKeySize) + kAesGcmTagSize,
    kWifiprovRekeyOldBindingSize = 72U,
    kWifiprovRekeyBindingSize = 88U
};

struct WifiprovRekeyOldBinding {
    uint8_t operationId[kWifiprovRekeyOperationSize];
    uint8_t challenge[kWifiprovRekeyChallengeSize];
    DeviceInstanceId instance;
    uint32_t accessEpoch;
    uint32_t fromVersion;
    DeviceKeyLocator oldLocator;

    WifiprovRekeyOldBinding()
        : operationId(), challenge(), instance(), accessEpoch(0U), fromVersion(0U), oldLocator() {}
};

struct WifiprovRekeyBinding {
    WifiprovRekeyOldBinding previous;
    DeviceKeyLocator newLocator;
};

struct WifiprovRekeyMac {
    uint8_t bytes[kWifiprovRekeyMacSize];

    WifiprovRekeyMac() : bytes() {}
};

Result decodeWifiprovRekeyOldBinding(ByteView bytes, WifiprovRekeyOldBinding& binding);
Result decodeWifiprovRekeyBinding(ByteView bytes, WifiprovRekeyBinding& binding);
Result encodeWifiprovRekeyBinding(const WifiprovRekeyBinding& binding, MutableByteSpan bytes);

// Pure cryptographic contract. The caller owns physical authorization,
// one-use intent and equality to its actual local instance/epoch.
Result createWifiprovRekeyChallengeProof(const DeviceKey& oldKey,
    const WifiprovRekeyOldBinding& binding, WifiprovRekeyMac& proof);

// The output must be distinct from oldKey. Output is zero on failure, except
// invalid aliasing leaves oldKey intact. Clear the returned Key after use.
Result openWifiprovRekeyCandidate(const DeviceKey& oldKey,
    const WifiprovRekeyBinding& binding, ByteView iv, ByteView box,
    IX25519AesGcmCryptoProvider& crypto, DeviceKey& candidate);

// Only call with the Key obtained by durable readback of the installed root.
// Producing a receipt does not itself prove persistence or network success.
Result createWifiprovRekeyInstalledReceipt(const DeviceKey& readbackKey,
    const WifiprovRekeyBinding& binding, WifiprovRekeyMac& receipt);

// Verify the Gateway's one-time permit against the exact installed Key and
// intent. The native owner/one-use fence is outside this cryptographic check.
Result verifyWifiprovRekeyWifiPermit(const DeviceKey& readbackKey,
    const WifiprovRekeyBinding& binding, ByteView permit);

// Admin must be freshly loaded from the current access root; the nonce must
// come from this actual Security1 setup owner, not from the request alone.
Result verifyWifiprovRekeySetupAdminProof(const ControllerCredential& admin,
    const WifiprovRekeyOldBinding& binding, ByteView ownerNonce, ByteView proof);

} // namespace security
} // namespace blinker

#endif
