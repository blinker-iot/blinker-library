#ifndef BLINKER_IDENTITY_DEVELOPERDEVICEID_H
#define BLINKER_IDENTITY_DEVELOPERDEVICEID_H

#include "../interface/IFactoryIdentity.h"

namespace blinker {

enum : size_t { kDeveloperPhysicalDeviceIdSize = 36U };

// "dev_" plus the first 16 SHA-256 bytes of the domain, algorithm and key.
Result deriveDeveloperPhysicalDeviceId(
    FactoryProofAlgorithm algorithm,
    ByteView publicKey,
    MutableCharSpan output,
    StringView& physicalDeviceId);

} // namespace blinker

#endif
