#ifndef BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGCONTRACT_H
#define BLINKER_PROVISIONING_LOCALCONTROLLERPAIRINGCONTRACT_H

#include "../core/HmacSha256.h"
#include "../identity/ControllerCredential.h"
#include "../identity/DeviceInstanceId.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

enum : size_t {
    kLocalControllerPairingNonceSize = 16U,
    kLocalControllerPairingTranscriptHashSize = 32U,
    kLocalControllerPairingRequestDigestSize = 32U,
    kLocalControllerPairingProofSize = 32U,
    kLocalControllerPairingChallengeMaxEncodedSize = 86U,
    kLocalControllerPairingRequestMaxEncodedSize = 135U,
    kLocalControllerPairingReceiptMaxEncodedSize = 174U,
    kLocalControllerPairingReceiptTranscriptMaxEncodedSize = 172U,
    kLocalControllerPairingMaxWindowMillis = 300000U
};

static const uint8_t kLocalControllerPairingContractVersion = 1U;

// Emitted only after the product has confirmed physical presence for the
// current Noise transcript. pairingNonce is volatile, not a setup code or
// durable device secret.
struct LocalControllerPairingChallenge {
    ByteView deviceInstanceId;
    ByteView pairingNonce;
    ByteView setupTranscriptHash;
    uint32_t permissions;
    uint32_t expiresInMillis;

    LocalControllerPairingChallenge()
        : permissions(0U), expiresInMillis(0U) {}
};

// One Local-domain bootstrap carried inside the protected Noise channel. v1
// installs credentialVersion=1 only. Later mutation requires an already
// authenticated controller path.
struct LocalControllerPairingRequest {
    ByteView deviceInstanceId;
    ByteView pairingNonce;
    ByteView setupTranscriptHash;
    ByteView controllerId;
    uint32_t credentialVersion;
    uint32_t permissions;
    ByteView controllerSecret;

    LocalControllerPairingRequest()
        : credentialVersion(0U), permissions(0U) {}
};

// Durable-install acknowledgement. The HMAC proves possession of the same
// controller secret. requestDigest binds every request field without echoing
// the secret to the App.
struct LocalControllerPairingReceipt {
    ByteView deviceInstanceId;
    ByteView pairingNonce;
    ByteView requestDigest;
    ByteView controllerId;
    uint32_t credentialVersion;
    uint32_t permissions;
    ByteView secretDigest;
    ByteView proof;

    LocalControllerPairingReceipt()
        : credentialVersion(0U), permissions(0U) {}
};

Result encodeLocalControllerPairingChallenge(
    const LocalControllerPairingChallenge& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeLocalControllerPairingChallenge(
    ByteView encoded,
    LocalControllerPairingChallenge& body);

Result encodeLocalControllerPairingRequest(
    const LocalControllerPairingRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeLocalControllerPairingRequest(
    ByteView encoded,
    LocalControllerPairingRequest& body);

Result encodeLocalControllerPairingReceipt(
    const LocalControllerPairingReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeLocalControllerPairingReceipt(
    ByteView encoded,
    LocalControllerPairingReceipt& body);
Result encodeLocalControllerPairingReceiptTranscript(
    const LocalControllerPairingReceipt& body,
    MutableByteSpan output,
    ByteView& encoded);
Result computeLocalControllerPairingReceiptProof(
    ByteView controllerSecret,
    const LocalControllerPairingReceipt& body,
    MutableByteSpan proofOutput);
Result verifyLocalControllerPairingReceiptProof(
    ByteView controllerSecret,
    const LocalControllerPairingReceipt& body);

} // namespace blinker

#endif
