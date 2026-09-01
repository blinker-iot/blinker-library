#ifndef BLINKER_RUNTIME_BLEDIRECTPROFILE_H
#define BLINKER_RUNTIME_BLEDIRECTPROFILE_H

#include "../identity/DeviceInstanceId.h"
#include "../interface/IClock.h"
#include "../interface/IDeviceAccessStore.h"
#include "../interface/IRandom.h"
#include "../protocol/ble/Mode.h"
#include "../protocol/ble/Presence.h"

namespace blinker {

static const uint32_t kBlePresenceRotationIntervalMillis =
    15UL * 60UL * 1000UL;

// Builds Direct advertising profiles from durable access state. The provider
// owns only the previous 4-byte nonce; raw PresenceKey material is loaded into
// a bounded stack object, used once and cleared before return.
class BleDirectProfileProvider {
public:
    BleDirectProfileProvider(
        const DeviceInstanceId& deviceInstance,
        IDeviceAccessStore& access,
        IRandom& random,
        IClock& clock,
        uint32_t rotationIntervalMillis =
            kBlePresenceRotationIntervalMillis);

    Result make(uint32_t sessionRevision, ble::ModeProfile& profile);
    bool refreshDue(
        size_t sessionCount,
        uint32_t sessionRevision) const;
    void reset();

private:
    Result tryMakeAuthorized(ble::ModeProfile& profile);

    const DeviceInstanceId& deviceInstance_;
    IDeviceAccessStore& access_;
    IRandom& random_;
    IClock& clock_;
    uint32_t rotationIntervalMillis_;
    uint32_t generatedAtMillis_;
    uint32_t observedSessionRevision_;
    uint8_t previousNonce_[ble::kPresenceNonceSize];
    uint8_t authorizedProfile_ : 1;
    uint8_t profileReady_ : 1;
    uint8_t hasPreviousNonce_ : 1;
};

} // namespace blinker

#endif
