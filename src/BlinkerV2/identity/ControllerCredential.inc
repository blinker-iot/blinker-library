#include "ControllerCredential.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool controllerCredentialAllZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

bool validDomainGeneration(const ControllerCredential& credential) {
    if (credential.domain == ControllerCredentialDomain::Local) {
        return credential.ownershipGeneration == 0U;
    }
    return credential.domain == ControllerCredentialDomain::PlatformAccess &&
           credential.ownershipGeneration != 0U;
}

} // namespace

ControllerCredential::ControllerCredential()
    : ownershipGeneration(0U),
      credentialVersion(0U),
      permissions(0U),
      suite(ControllerCredentialSuite::HmacSha256_32),
      domain(ControllerCredentialDomain::Local),
      controllerId(),
      secret() {}

Result validateControllerCredential(
    const ControllerCredential& credential) {
    if (credential.suite != ControllerCredentialSuite::HmacSha256_32 ||
        !validDomainGeneration(credential) ||
        credential.credentialVersion == 0U ||
        credential.permissions == 0U ||
        (credential.permissions & ~kControllerPermissionAll) != 0U ||
        controllerCredentialAllZero(credential.id()) ||
        controllerCredentialAllZero(credential.authenticationSecret())) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

bool sameControllerCredential(
    const ControllerCredential& first,
    const ControllerCredential& second) {
    return first.ownershipGeneration == second.ownershipGeneration &&
           first.credentialVersion == second.credentialVersion &&
           first.permissions == second.permissions &&
           first.suite == second.suite &&
           first.domain == second.domain &&
           constantTimeEqual(first.id(), second.id()) &&
           constantTimeEqual(
               first.authenticationSecret(),
               second.authenticationSecret());
}

void clearControllerCredential(ControllerCredential& credential) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&credential),
        sizeof(credential)));
    credential.suite = ControllerCredentialSuite::HmacSha256_32;
    credential.domain = ControllerCredentialDomain::Local;
}

} // namespace blinker
