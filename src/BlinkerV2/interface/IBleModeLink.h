#ifndef BLINKER_INTERFACE_IBLEMODELINK_H
#define BLINKER_INTERFACE_IBLEMODELINK_H

#include "IBleLink.h"
#include "../protocol/ble/Mode.h"

namespace blinker {

// A platform link with an explicit, versioned advertising profile. Full mode
// changes are configured while stopped. Platforms which can safely replace an
// idle advertising payload may override refreshBleProfile(); the default keeps
// that optimization optional. No setup secret is carried by this interface.
class IBleModeLink : public IBleLink {
public:
    virtual Result configureBleProfile(
        const ble::ModeProfile& profile) = 0;
    virtual Result refreshBleProfile(
        const ble::ModeProfile&) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    virtual ble::ModeProfile bleProfile() const = 0;
};

} // namespace blinker

#endif
