#ifndef BLINKER_PROVISIONING_BLENOISEMODEPREPARATION_H
#define BLINKER_PROVISIONING_BLENOISEMODEPREPARATION_H

#include "../interface/IBleModePreparation.h"
#include "../interface/ILocalProvisioningSession.h"
#include "../interface/ISetupPskProvider.h"
#include "BleNoiseProvisioningChannel.h"

namespace blinker {

// Product composition seam between BLE mode ownership and the protected
// setup security profile. Ordinary NN does not require a secret provider;
// enhanced NNpsk0 loads one PSK into a short-lived stack buffer. Direct mode
// always cancels any secure provisioning state.
class BleNoiseModePreparation : public IBleModePreparation {
public:
    BleNoiseModePreparation(
        BleNoiseProvisioningChannel& channel,
        ILocalProvisioningSession& session)
        : channel_(channel), provider_(nullptr), session_(session) {}
    BleNoiseModePreparation(
        BleNoiseProvisioningChannel& channel,
        ISetupPskProvider& provider,
        ILocalProvisioningSession& session)
        : channel_(channel), provider_(&provider), session_(session) {}

    Result prepareBleMode(
        const ble::ModeProfile& profile,
        bool rollback) override;

private:
    BleNoiseProvisioningChannel& channel_;
    ISetupPskProvider* provider_;
    ILocalProvisioningSession& session_;

    Result armPsk(const ble::ModeProfile& profile);

    BleNoiseModePreparation(const BleNoiseModePreparation&);
    BleNoiseModePreparation& operator=(const BleNoiseModePreparation&);
};

} // namespace blinker

#endif
