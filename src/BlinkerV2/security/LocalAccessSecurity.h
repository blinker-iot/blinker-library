#ifndef BLINKER_SECURITY_LOCALACCESSSECURITY_H
#define BLINKER_SECURITY_LOCALACCESSSECURITY_H

#include "../identity/DeviceKey.h"
#include "../protocol/local/Access.h"
#include "../protocol/local/Control.h"
#include "../protocol/local/Progress.h"

namespace blinker {
namespace security {

Result computeLocalAccessProgressAuthenticator(const DeviceKey& key, const local_access::ProgressView& value,
    ByteView proofSessionId, MutableByteSpan output);
Result verifyLocalAccessProgressAuthenticator(const DeviceKey& key, const local_access::ProgressView& value,
    ByteView proofSessionId);

Result computeLocalAccessControlAuthenticator(
    const DeviceKey& deviceKey, const local_access::ControlView& control,
    MutableByteSpan output, ByteView proofSessionId = ByteView());
Result verifyLocalAccessControlAuthenticator(
    const DeviceKey& deviceKey, const local_access::ControlView& control,
    ByteView proofSessionId = ByteView());

Result computeLocalAccessGrantAuthenticator(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant,
    MutableByteSpan output);

Result verifyLocalAccessGrantAuthenticator(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant);

// Profile-specific material never appears in the LAN grant: Service and MCU
// derive it independently, App receives it over HTTPS. Profile 1 is the old
// Noise PSK; profile 2 is a domain-separated key for handshake proofs only.
Result deriveLocalAccessSessionKey(
    const DeviceKey& deviceKey,
    const local_access::GrantView& grant,
    MutableByteSpan output);

Result computeLocalAccessPlainProof(ByteView sessionKey, ByteView grantAuthenticator,
    uint8_t kind, MutableByteSpan output);

} // namespace security
} // namespace blinker

#endif
