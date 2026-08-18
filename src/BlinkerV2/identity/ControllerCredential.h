#ifndef BLINKER_IDENTITY_CONTROLLERCREDENTIAL_H
#define BLINKER_IDENTITY_CONTROLLERCREDENTIAL_H

#include "../core/Authorization.h"
#include "../core/Result.h"
#include "../core/Span.h"

namespace blinker {

enum class ControllerCredentialSuite : uint8_t {
    HmacSha256_32 = 1U
};

enum class ControllerCredentialDomain : uint8_t {
    Local = 1U,
    PlatformAccess = 2U,
    // Source compatibility for the unpublished v2 experiments. Value 2 on
    // persistent/wire records means platform direct access, not ownership.
    Ownership = PlatformAccess
};

enum : size_t {
    kControllerIdSize = 16U,
    kControllerCredentialSecretSize = 32U
};

enum : uint32_t {
    kControllerPermissionObserve = kAuthorizationPermissionObserve,
    kControllerPermissionControl = kAuthorizationPermissionControl,
    kControllerPermissionProvisionNetwork =
        kAuthorizationPermissionProvisionNetwork,
    kControllerPermissionManageControllers =
        kAuthorizationPermissionManageControllers,
    kControllerPermissionAll = kAuthorizationPermissionAll
};

// One installed direct-access authentication root. App account and sharing
// records are not persisted on the device. Local credentials use epoch 0;
// server-managed Admin/Shared groups bind to one non-zero access epoch.
struct ControllerCredential {
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    uint32_t permissions;
    ControllerCredentialSuite suite;
    ControllerCredentialDomain domain;
    uint8_t controllerId[kControllerIdSize];
    uint8_t secret[kControllerCredentialSecretSize];

    ControllerCredential();

    ByteView id() const {
        return ByteView(controllerId, sizeof(controllerId));
    }
    ByteView authenticationSecret() const {
        return ByteView(secret, sizeof(secret));
    }
};

Result validateControllerCredential(
    const ControllerCredential& credential);
bool sameControllerCredential(
    const ControllerCredential& first,
    const ControllerCredential& second);
void clearControllerCredential(ControllerCredential& credential);

} // namespace blinker

#endif
