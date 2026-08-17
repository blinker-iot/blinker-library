#ifndef BLINKER_INTERFACE_IBLEMODEPREPARATION_H
#define BLINKER_INTERFACE_IBLEMODEPREPARATION_H

#include "../core/Result.h"
#include "../protocol/ble/Mode.h"

namespace blinker {

// Product-owned preparation which runs while the physical BLE link is
// stopped, immediately before the selected logical endpoint starts. It lets
// provisioning re-arm short-lived secure state without putting setup secrets
// in the mode coordinator or advertising profile.
class IBleModePreparation {
public:
    virtual ~IBleModePreparation() {}

    virtual Result prepareBleMode(
        const ble::ModeProfile& profile,
        bool rollback) = 0;
};

} // namespace blinker

#endif
