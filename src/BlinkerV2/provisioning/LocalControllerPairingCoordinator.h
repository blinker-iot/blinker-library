#ifndef BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGCOORDINATOR_H
#define BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGCOORDINATOR_H

#include "LocalControllerPairingContract.h"
#include "../interface/IClock.h"
#include "../interface/IControllerCredentialStore.h"
#include "../interface/IRandom.h"

namespace blinker {

struct LocalControllerPairingConfig {
    uint32_t permissions;
    uint32_t windowMillis;

    LocalControllerPairingConfig()
        : permissions(
              kControllerPermissionObserve |
              kControllerPermissionControl),
          windowMillis(60000U) {}
};

// LocalBLE-only bootstrap boundary. beginConfirmedPairing() may be called
// only by a product-owned physical-presence event and only after the current
// Noise handshake hash is available. BLE input must never open this window.
class LocalControllerPairingCoordinator {
public:
    LocalControllerPairingCoordinator(
        const DeviceInstanceId& deviceInstanceId,
        IControllerCredentialStore& credentials,
        IRandom& random,
        IClock& clock,
        const LocalControllerPairingConfig& config =
            LocalControllerPairingConfig());
    ~LocalControllerPairingCoordinator();
    LocalControllerPairingCoordinator(
        const LocalControllerPairingCoordinator&) = delete;
    LocalControllerPairingCoordinator& operator=(
        const LocalControllerPairingCoordinator&) = delete;

    Result beginConfirmedPairing(ByteView setupTranscriptHash);
    Result challenge(MutableByteSpan output, ByteView& encoded);
    Result apply(ByteView encodedRequest,
                 MutableByteSpan output,
                 ByteView& encodedReceipt);
    void endPairing();

    bool active() const { return active_; }
    bool applied() const { return applied_; }
    ByteView pairingNonce() const {
        return active_
                   ? ByteView(pairingNonce_, sizeof(pairingNonce_))
                   : ByteView();
    }

private:
    Result requireActive();
    uint32_t remainingMillis() const;

    DeviceInstanceId deviceInstanceId_;
    IControllerCredentialStore& credentials_;
    IRandom& random_;
    IClock& clock_;
    LocalControllerPairingConfig config_;
    uint8_t pairingNonce_[kLocalControllerPairingNonceSize];
    uint8_t setupTranscriptHash_[kLocalControllerPairingTranscriptHashSize];
    uint8_t appliedRequestDigest_[kLocalControllerPairingRequestDigestSize];
    uint32_t startedAtMillis_;
    bool active_;
    bool applied_;
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(LocalControllerPairingCoordinator) <= 128U,
    "Local controller pairing coordinator exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
