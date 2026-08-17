#ifndef BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGAPPLICATION_H
#define BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGAPPLICATION_H

#include "LocalControllerPairingApplicationContract.h"
#include "LocalControllerPairingCoordinator.h"
#include "../identity/DeviceInstanceId.h"
#include "../interface/ILocalProvisioningApplication.h"
#include "../protocol/SetupSession.h"

namespace blinker {

enum class LocalControllerPairingApplicationState : uint8_t {
    Idle = 0U,
    AwaitingHello,
    AwaitingSecureSession,
    Ready
};

// LocalBLE application above one Noise session. BLE messages can read a
// challenge or submit a request, but only product-owned physical input may
// call confirmPhysicalPresence() and open the pairing window.
class LocalControllerPairingApplication final
    : public ILocalProvisioningApplication {
public:
    LocalControllerPairingApplication(
        const DeviceInstanceId& deviceInstanceId,
        LocalControllerPairingCoordinator& coordinator);
    ~LocalControllerPairingApplication() override;
    LocalControllerPairingApplication(
        const LocalControllerPairingApplication&) = delete;
    LocalControllerPairingApplication& operator=(
        const LocalControllerPairingApplication&) = delete;

    Result beginSession(ByteView setupSessionLocator) override;
    Result secureSessionReady(ByteView setupTranscriptHash) override;
    void endSession() override;
    Result handle(ByteView request,
                  MutableByteSpan operationWorkspace,
                  MutableByteSpan output,
                  ByteView& response) override;

    Result confirmPhysicalPresence();
    LocalControllerPairingApplicationState state() const { return state_; }

private:
    Result encodeError(uint32_t requestId,
                       ErrorCode error,
                       MutableByteSpan output,
                       ByteView& response) const;
    void clearSession();

    const DeviceInstanceId& deviceInstanceId_;
    LocalControllerPairingCoordinator& coordinator_;
    uint8_t setupSessionLocator_[kSetupSessionLocatorSize];
    uint8_t setupTranscriptHash_[
        kLocalControllerPairingTranscriptHashSize];
    LocalControllerPairingApplicationState state_;
};

#if UINTPTR_MAX <= UINT32_MAX
static_assert(
    sizeof(LocalControllerPairingApplication) <= 64U,
    "Local controller pairing application exceeds its 32-bit ABI gate");
#endif

} // namespace blinker

#endif
