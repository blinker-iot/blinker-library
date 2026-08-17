#ifndef BLINKER_INTERFACE_IBLEMODELINK_H
#define BLINKER_INTERFACE_IBLEMODELINK_H

#include "IBleLink.h"
#include "../protocol/ble/Mode.h"

namespace blinker {

// A platform link with an explicit, versioned advertising profile. Profile
// changes are accepted only while the link is stopped and are copied before
// return. No setup secret is carried by this interface.
class IBleModeLink : public IBleLink {
public:
    virtual Result configureBleProfile(
        const ble::ModeProfile& profile) = 0;
    virtual ble::ModeProfile bleProfile() const = 0;
};

} // namespace blinker

#endif
