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
    Ownership = 2U
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

// One installed App/controller authentication root. App role names are not
// persisted on the device; the verified control plane maps them to this small
// permission mask. Local credentials use ownershipGeneration == 0. Platform
// credentials are bound to one non-zero OwnershipRecord generation.
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
