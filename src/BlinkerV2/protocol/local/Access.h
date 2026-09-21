#ifndef BLINKER_PROTOCOL_LOCAL_ACCESS_H
#define BLINKER_PROTOCOL_LOCAL_ACCESS_H

#include "../../core/Authorization.h"
#include "../../core/Result.h"
#include "../../core/Span.h"
#include "../cbor/Cbor.h"

namespace blinker {
namespace local_access {

enum : uint8_t {
    kContractVersion = 1U,
    kChallengeKind = 1U,
    kGrantKind = 2U,
    kCallerProofKind = 8U,
    kRecipientProofKind = 9U
};

enum : size_t {
    kIdSize = 16U,
    kChallengeSize = 16U,
    kAuthenticatorSize = 32U,
    kMaximumLogicalDeviceIdSize = 64U,
    kChallengeMaximumEncodedSize = 96U,
    kGrantMaximumEncodedSize = 272U,
    kPlainProofEncodedSize = 37U
};

enum : uint32_t {
    kChallengeResponseMillis = 15000U,
    kMinimumLifetimeMillis = 1000U,
    kMaximumLifetimeMillis = 300000U
};

enum class SecurityProfile : uint8_t {
    NoiseNnPsk0AesGcmSha256 = 1U,
    PlainHmacSha256 = 2U // Handshake proof only; business frames have no MAC/encryption.
};

enum class TargetKind : uint8_t {
    Self = 1U,
    Child = 2U
};

struct ChallengeView {
    uint8_t version;
    ByteView recipientDeviceInstanceId;
    uint32_t deviceKeyVersion;
    ByteView receiverSessionId;
    ByteView challenge;
    uint32_t authorityRevision;
    uint32_t responseWindowMillis;
    uint32_t maximumLifetimeMillis;
    SecurityProfile securityProfile;

    ChallengeView();
};

struct GrantView {
    uint8_t version;
    ByteView grantId;
    ByteView authorizationId;
    ByteView recipientDeviceInstanceId;
    uint32_t deviceKeyVersion;
    ByteView receiverSessionId;
    ByteView challenge;
    ByteView callerSessionId;
    uint32_t authorityRevision;
    TargetKind targetKind;
    StringView targetLogicalDeviceId;
    ByteView targetDeviceInstanceId;
    uint32_t accessEpoch;
    uint32_t topologyVersion;
    uint32_t permissions;
    uint32_t lifetimeMillis;
    SecurityProfile securityProfile;
    ByteView authenticator;

    GrantView();
};

Result validateChallenge(const ChallengeView& value);
Result encodeChallenge(
    const ChallengeView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeChallenge(ByteView encoded, ChallengeView& value);

Result validateGrantUnsigned(const GrantView& value);
Result validateGrant(const GrantView& value);
Result encodeGrant(
    const GrantView& value,
    MutableByteSpan output,
    size_t& written);
Result decodeGrant(ByteView encoded, GrantView& value);

Result encodePlainProof(uint8_t kind, ByteView authenticator, MutableByteSpan output, size_t& written);
Result decodePlainProof(ByteView encoded, uint8_t expectedKind, ByteView& authenticator);

// Streams the exact canonical CBOR grant without its authenticator. This is
// the sole MAC transcript and avoids a second grant-sized MCU scratch buffer.
Result streamGrantTranscript(
    const GrantView& value,
    cbor::WriteSink sink,
    void* context,
    size_t& written);

} // namespace local_access
} // namespace blinker

#endif
