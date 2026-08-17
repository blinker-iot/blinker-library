#ifndef BLINKER_CORE_AUTHORIZATION_H
#define BLINKER_CORE_AUTHORIZATION_H

#include <stdint.h>

namespace blinker {

enum : uint32_t {
    kAuthorizationPermissionObserve = 1UL << 0U,
    kAuthorizationPermissionControl = 1UL << 1U,
    kAuthorizationPermissionProvisionNetwork = 1UL << 2U,
    kAuthorizationPermissionManageControllers = 1UL << 3U,
    kAuthorizationPermissionAll =
        kAuthorizationPermissionObserve |
        kAuthorizationPermissionControl |
        kAuthorizationPermissionProvisionNetwork |
        kAuthorizationPermissionManageControllers
};

inline bool validAuthorizationPermissions(uint32_t permissions) {
    return permissions != 0U &&
           (permissions & ~kAuthorizationPermissionAll) == 0U;
}

} // namespace blinker

#endif
