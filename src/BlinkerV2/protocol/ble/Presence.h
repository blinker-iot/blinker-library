#ifndef BLINKER_PROTOCOL_BLE_PRESENCE_H
#define BLINKER_PROTOCOL_BLE_PRESENCE_H

#include "../../core/Result.h"
#include "../../core/Span.h"
#include "../../identity/DeviceInstanceId.h"
#include "../../identity/DevicePresenceKey.h"
#include "../../interface/IRandom.h"

namespace blinker {
namespace ble {

enum : size_t {
    kPresenceKeySize = kDevicePresenceKeySize,
    kPresenceNonceSize = 4U,
    kPresenceTagSize = 4U,
    kPresenceLocatorSize = kPresenceNonceSize + kPresenceTagSize
};

// Exact locator contract:
//   nonce32 || Trunc32(HMAC-SHA256(
//     PresenceKey,
//     "blinker/ble/presence/v1" || 0x00 || DeviceInstanceId ||
//     accessEpoch(be32) || presenceKeyVersion(be32) || nonce32))
//
// The locator is authorized discovery metadata only. It never authenticates a
// GATT peer and cannot replace Method 2 or DirectSecureRecord.
Result derivePresenceLocator(
    ByteView presenceKey,
    const DeviceInstanceId& deviceInstanceId,
    uint32_t accessEpoch,
    uint32_t presenceKeyVersion,
    ByteView nonce,
    MutableByteSpan output);

Result verifyPresenceLocator(
    ByteView presenceKey,
    const DeviceInstanceId& deviceInstanceId,
    uint32_t accessEpoch,
    uint32_t presenceKeyVersion,
    ByteView locator,
    bool& matches);

// Fills one 32-bit nonce from the platform CSPRNG. When previousNonce is
// supplied, four bounded attempts prevent an identical consecutive value.
Result generatePresenceNonce(
    IRandom& random,
    ByteView previousNonce,
    MutableByteSpan output);

} // namespace ble
} // namespace blinker

#endif
