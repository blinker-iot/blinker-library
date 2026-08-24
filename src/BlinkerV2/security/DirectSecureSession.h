#ifndef BLINKER_SECURITY_DIRECTSECURESESSION_H
#define BLINKER_SECURITY_DIRECTSECURESESSION_H

#include "../identity/ControllerCredential.h"
#include "../interface/IX25519AesGcmCryptoProvider.h"

namespace blinker {
namespace security {

enum : size_t {
    kDirectSecureHeaderSize = 8U,
    kDirectSecureTagSize = kAesGcmTagSize,
    kDirectSecureOverhead = kDirectSecureHeaderSize + kDirectSecureTagSize,
    kDirectSecureMaximumPlaintextSize = UINT16_MAX - kDirectSecureOverhead,
    kDirectSecureNoncePrefixSize = 8U,
    kDirectSecureBindingSize = 16U,
    kDirectSecureAuthNonceSize = 16U
};

static const uint8_t kDirectSecureMagic = 0xD3U;
static const uint8_t kDirectSecureVersion = 1U;
static const uint8_t kDirectSecureVersionShift = 4U;
static const uint8_t kDirectSecureVersionMask = 0xF0U;
static const uint8_t kDirectSecureFlagsMask = 0x0FU;

enum class DirectSecureRole : uint8_t {
    Initiator = 1U,
    Responder = 2U
};

enum class DirectSecureDirection : uint8_t {
    InitiatorToResponder = 1U,
    ResponderToInitiator = 2U
};

struct DirectSecureContext {
    ByteView controllerId;
    ControllerCredentialDomain domain;
    uint32_t ownershipGeneration;
    uint32_t credentialVersion;
    uint32_t permissions;
    ByteView clientNonce;
    ByteView deviceNonce;

    DirectSecureContext()
        : controllerId(),
          domain(ControllerCredentialDomain::Local),
          ownershipGeneration(0U),
          credentialVersion(0U),
          permissions(0U),
          clientNonce(),
          deviceNonce() {}
};

struct DirectSecureRecordView {
    uint32_t sequence;
    ByteView ciphertextAndTag;

    DirectSecureRecordView() : sequence(0U), ciphertextAndTag() {}
};

// Authentication prepares key material while the Method 2 credential and
// transcript are still available. Runtime commits only after the final raw
// AuthResult has been queued, so no peer can observe a half-switched session.
class IDirectSecureSessionController {
public:
    virtual ~IDirectSecureSessionController() {}

    virtual Result prepare(
        uint32_t sessionId,
        ByteView controllerSecret,
        const DirectSecureContext& context) = 0;
    virtual void commit(uint32_t sessionId) = 0;
    virtual void cancel(uint32_t sessionId) = 0;
};

struct DirectSecureKeyMaterial {
    uint8_t initiatorKey[kAes256KeySize];
    uint8_t responderKey[kAes256KeySize];
    uint8_t initiatorNoncePrefix[kDirectSecureNoncePrefixSize];
    uint8_t responderNoncePrefix[kDirectSecureNoncePrefixSize];
    uint8_t sessionBinding[kDirectSecureBindingSize];

    DirectSecureKeyMaterial();
    void clear();
};

size_t directSecureRecordSize(size_t plaintextSize);
Result decodeDirectSecureRecordSize(ByteView prefix, size_t& recordSize);
Result parseDirectSecureRecord(
    ByteView encoded,
    DirectSecureRecordView& record);
Result deriveDirectSecureKeyMaterial(
    ByteView controllerSecret,
    const DirectSecureContext& context,
    DirectSecureKeyMaterial& output);

// Allocation-free daily Direct data-plane protection. The App/Gateway is the
// initiator and the device is the responder. Authentication frames remain
// outside this session; after Method 2 succeeds, every BBP/2 frame is sealed.
class DirectSecureSession {
public:
    DirectSecureSession();
    ~DirectSecureSession();
    DirectSecureSession(const DirectSecureSession&) = delete;
    DirectSecureSession& operator=(const DirectSecureSession&) = delete;

    Result begin(
        DirectSecureRole role,
        ByteView controllerSecret,
        const DirectSecureContext& context);
    void clear();

    bool ready() const { return ready_; }
    uint32_t nextSendSequence() const { return sendSequence_; }
    uint32_t nextReceiveSequence() const { return receiveSequence_; }
    ByteView sessionBinding() const {
        return ByteView(sessionBinding_, sizeof(sessionBinding_));
    }

    Result seal(
        IX25519AesGcmCryptoProvider& crypto,
        ByteView plaintext,
        MutableByteSpan output,
        ByteView& encoded);
    Result open(
        IX25519AesGcmCryptoProvider& crypto,
        ByteView encoded,
        MutableByteSpan output,
        ByteView& plaintext);

private:
    DirectSecureDirection sendDirection() const;
    DirectSecureDirection receiveDirection() const;
    Result buildNonce(
        ByteView prefix,
        uint32_t sequence,
        MutableByteSpan output) const;
    Result buildAad(
        DirectSecureDirection direction,
        ByteView header,
        MutableByteSpan output,
        ByteView& aad) const;

    DirectSecureRole role_;
    uint8_t sendKey_[kAes256KeySize];
    uint8_t receiveKey_[kAes256KeySize];
    uint8_t sendNoncePrefix_[kDirectSecureNoncePrefixSize];
    uint8_t receiveNoncePrefix_[kDirectSecureNoncePrefixSize];
    uint8_t sessionBinding_[kDirectSecureBindingSize];
    uint32_t sendSequence_;
    uint32_t receiveSequence_;
    bool sendExhausted_;
    bool receiveExhausted_;
    bool ready_;
};

} // namespace security
} // namespace blinker

#endif
