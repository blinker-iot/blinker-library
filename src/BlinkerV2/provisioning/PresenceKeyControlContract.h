#ifndef BLINKER_PROVISIONING_PRESENCEKEYCONTROLCONTRACT_H
#define BLINKER_PROVISIONING_PRESENCEKEYCONTROLCONTRACT_H

#include "../core/HmacSha256.h"
#include "../identity/DevicePresenceKey.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum : size_t {
    kPresenceKeyDigestSize = 32U,
    kPresenceKeyReceiptProofSize = 32U,
    kPresenceKeyMutationMaxEncodedSize = 39U,
    kPresenceKeyReceiptMaxEncodedSize = 93U
};

static const uint8_t kPresenceKeyControlVersion = 1U;

struct PresenceKeyMutation {
    uint32_t accessEpoch;
    uint32_t expectedVersion;
    DevicePresenceKey presence;

    PresenceKeyMutation()
        : accessEpoch(0U), expectedVersion(0U), presence() {}
};

struct PresenceKeyReceipt {
    uint32_t accessEpoch;
    uint32_t expectedVersion;
    uint32_t presenceKeyVersion;
    ByteView keyDigest;
    uint8_t proofKind;
    ByteView proof;

    PresenceKeyReceipt()
        : accessEpoch(0U), expectedVersion(0U), presenceKeyVersion(0U),
          keyDigest(), proofKind(1U), proof() {}
};

Result encodePresenceKeyMutation(
    const PresenceKeyMutation& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodePresenceKeyMutation(
    ByteView encoded,
    PresenceKeyMutation& body);
Result encodePresenceKeyReceipt(
    const PresenceKeyReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodePresenceKeyReceipt(
    ByteView encoded,
    PresenceKeyReceipt& body);
Result computePresenceKeyReceiptProof(
    const DevicePresenceKey& presence,
    const PresenceKeyReceipt& body,
    MutableByteSpan digestOutput,
    MutableByteSpan proofOutput);
Result verifyPresenceKeyReceiptProof(
    const DevicePresenceKey& presence,
    const PresenceKeyReceipt& body);

} // namespace blinker

#endif
