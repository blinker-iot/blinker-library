#ifndef BLINKER_PROVISIONING_WIFIPROVREKEYENDPOINT_H
#define BLINKER_PROVISIONING_WIFIPROVREKEYENDPOINT_H

#include <atomic>

#include "../identity/DeviceAccessStore.h"
#include "../security/WifiprovRekeyProof.h"
#include "DeviceKeyProvisioningEndpoint.h"

namespace blinker {

enum class WifiprovRekeyOperation : uint8_t { Challenge = 4U, Install = 5U, ReadReceipt = 6U };

enum : size_t {
    kWifiprovRekeyChallengeRequestSize = 2U + security::kWifiprovRekeyOldBindingSize,
    kWifiprovRekeyReceiptRequestSize = 2U + security::kWifiprovRekeyBindingSize,
    kWifiprovRekeyInstallRequestSize = kWifiprovRekeyReceiptRequestSize + kAesGcmNonceSize + security::kWifiprovRekeyBoxSize,
    kWifiprovRekeySuccessResponseSize = 3U + security::kWifiprovRekeyOperationSize + security::kWifiprovRekeyMacSize
};

constexpr uint8_t kWifiprovRekeyOutcomeUnknown = 9U;

struct WifiprovRekeyAdmin {
    uint8_t controllerId[kControllerIdSize];
    uint32_t version;

    WifiprovRekeyAdmin() : controllerId(), version(0U) {}
    ByteView id() const { return ByteView(controllerId, sizeof(controllerId)); }
};

// Trusted product port. There is no default implementation or product route.
// Hold the actual physical WiFiProv owner through end(), pin the independently
// authenticated local Admin and exact authorized intent (not packet claims),
// and enforce product root/task admission. An unresolved attempt excludes all
// other intents, including after a power loss. Never infer authorization from
// matching selectors, a public instance, or an ordinary ready/installed bit.
class IWifiprovRekeyOwnerGuard : public IWifiprovRekeyCommitGuard {
public:
    virtual Result begin(WifiprovRekeyOperation operation,
        const security::WifiprovRekeyOldBinding& previous,
        const DeviceKeyLocator* newLocator, WifiprovRekeyAdmin& admin) = 0;

    // Install only: persist the exact active intent's one-use attempt BEFORE
    // any root write. A repeat or unresolved different intent must be rejected.
    // The attempt remains reserved regardless of the write's outcome.
    virtual Result reserveAttempt() = 0;

    // Called after reservation, under the existing access-root commit fence:
    // validateWifiprovRekeyCommit() must check live pinned ownership, with no
    // I/O, no waits and no recovery in this callback.

    // Install/ReadReceipt only: require the same prior durable attempt and
    // return a fresh read-only handle of the SAME committed root, pinned until
    // end(). A different wrapper around the failed write handle is not fresh.
    // Unavailable durable data is UNKNOWN, never a reason to write again.
    virtual Result openDurableReadback(IAtomicBlobStore*& storage) = 0;
    virtual void end() = 0;
};

// Narrow blinker-config v2 adapter. It neither registers a route nor grants
// WiFi Apply. Without a guard it stays closed. One live adapter can enter the
// write phase once; the guard is additionally responsible across lifecycles.
class WifiprovRekeyEndpoint {
public:
    WifiprovRekeyEndpoint(const DeviceInstanceId& instance, DeviceAccessStore& access,
        IX25519AesGcmCryptoProvider& crypto, IWifiprovRekeyOwnerGuard* guard = nullptr)
        : instance_(instance), access_(access), crypto_(crypto), guard_(guard), attempted_(false) {}

    Result handle(ByteView request, MutableByteSpan response, size_t& written);

private:
    Result readReceipt(const WifiprovRekeyAdmin& admin,
        const security::WifiprovRekeyBinding& binding, security::WifiprovRekeyMac& receipt);

    const DeviceInstanceId& instance_;
    DeviceAccessStore& access_;
    IX25519AesGcmCryptoProvider& crypto_;
    IWifiprovRekeyOwnerGuard* const guard_;
    std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    bool attempted_;
};

} // namespace blinker

#endif
