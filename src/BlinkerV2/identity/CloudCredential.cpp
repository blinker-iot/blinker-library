#include "CloudCredential.h"

#include <string.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

bool allZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

} // namespace

CloudCredential::CloudCredential()
    : suite(CloudCredentialSuite::HmacSha256_32),
      logicalDeviceIdLength(0U),
      generation(0U),
      credentialVersion(0U),
      secret(),
      logicalDeviceId() {}

Result validateCloudCredential(const CloudCredential& credential) {
    if (credential.suite != CloudCredentialSuite::HmacSha256_32 ||
        credential.generation == 0U ||
        credential.credentialVersion == 0U ||
        allZero(credential.authenticationSecret()) ||
        !validateLogicalDeviceId(credential.logicalId())) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return Result::success();
}

bool sameCloudCredential(
    const CloudCredential& first,
    const CloudCredential& second) {
    return first.suite == second.suite &&
           first.generation == second.generation &&
           first.credentialVersion == second.credentialVersion &&
           first.logicalDeviceIdLength == second.logicalDeviceIdLength &&
           constantTimeEqual(
               first.authenticationSecret(),
               second.authenticationSecret()) &&
           memcmp(
               first.logicalDeviceId,
               second.logicalDeviceId,
               first.logicalDeviceIdLength) == 0;
}

void clearCloudCredential(CloudCredential& credential) {
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(&credential),
        sizeof(credential)));
    credential.suite = CloudCredentialSuite::HmacSha256_32;
}

} // namespace blinker
