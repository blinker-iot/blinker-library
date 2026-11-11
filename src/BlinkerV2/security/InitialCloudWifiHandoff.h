#ifndef BLINKER_SECURITY_INITIALCLOUDWIFIHANDOFF_H
#define BLINKER_SECURITY_INITIALCLOUDWIFIHANDOFF_H

#include "InitialCloudKeyRegistration.h"
#include "../protocol/bbp2/InitialCloudKeyInstall.h"

namespace blinker {
namespace security {

enum class InitialCloudWifiHandoffPurpose : uint8_t {
    KeyInstallation = 0U,
    InstalledKeyNetwork = 1U
};

// Non-secret, single-owner handoff coordinates, not a fabricated installation
// receipt. For InstalledKeyNetwork, intent holds the NEW network operation ID.
struct InitialCloudKeyHandoff {
    uint8_t intent[bbp2::kInitialCloudKeyInstallIntentSize];
    DeviceKeyLocator locator;
    InitialCloudKeyHandoffAuthority authority;
    uint8_t sessionBinding[kDirectSecureBindingSize];
    InitialCloudWifiHandoffPurpose purpose = InitialCloudWifiHandoffPurpose::KeyInstallation;
};

class IInitialCloudWifiHandoffSource {
public:
    virtual ~IInitialCloudWifiHandoffSource() {}
    virtual bool takeAuthorizedHandoff(InitialCloudKeyHandoff& output) = 0;
    virtual void retire() = 0;
};

} // namespace security
} // namespace blinker
#endif
