#ifndef BLINKER_SECURITY_NOISENN_H
#define BLINKER_SECURITY_NOISENN_H

#include "../interface/INoiseCryptoProvider.h"
#include "../interface/IRandom.h"
#include "../protocol/noise/NoiseSuite.h"

namespace blinker {
namespace security {

static const size_t kNoiseKeySize = 32U;
static const size_t kNoiseDhSize = 32U;
static const size_t kNoiseNonceSize = 12U;
static const size_t kNoiseTagSize = 16U;
static const size_t kNoiseMaxHandshakeOverhead =
    kNoiseDhSize + kNoiseTagSize;

enum class NoiseRole : uint8_t {
    Initiator = 1U,
    Responder = 2U
};

enum class NoiseSessionState : uint8_t {
    Empty = 0U,
    WriteInitiatorHandshake,
    ReadInitiatorHandshake,
    WriteResponderHandshake,
    ReadResponderHandshake,
    Transport,
    Failed
};

// One allocation-free state machine for Noise_NN and Noise_NNpsk0 with
// 25519, AESGCM and SHA256. Pattern selection is product-owned and fixed
// before a session starts. Cryptographic primitives remain platform-owned.
class NoiseNnSession {
public:
    NoiseNnSession(INoiseCryptoProvider& crypto, IRandom& random);
    ~NoiseNnSession();
    NoiseNnSession(const NoiseNnSession&) = delete;
    NoiseNnSession& operator=(const NoiseNnSession&) = delete;

    Result begin(
        NoiseRole role,
        noise::NoiseNnPattern pattern,
        ByteView psk,
        ByteView prologue);
    void clear();

    NoiseSessionState state() const { return state_; }
    noise::NoiseNnPattern pattern() const { return pattern_; }
    bool ready() const { return state_ == NoiseSessionState::Transport; }

    Result handshakeMessageSize(
        size_t payloadSize,
        size_t& messageSize) const;
    Result writeHandshake(
        ByteView payload,
        MutableByteSpan output,
        ByteView& message);
    Result readHandshake(
        ByteView message,
        MutableByteSpan payloadOutput,
        ByteView& payload);

    Result encryptTransport(
        ByteView plaintext,
        MutableByteSpan output,
        ByteView& ciphertext);
    Result decryptTransport(
        ByteView ciphertext,
        MutableByteSpan output,
        ByteView& plaintext);

    Result handshakeHash(ByteView& output) const;

private:
    Result initializeSymmetric();
    Result mixHash(ByteView input);
    Result mixKey(ByteView input);
    Result mixKeyAndHash(ByteView input);
    Result mixEphemeral(ByteView publicKey);
    Result encryptAndHash(
        ByteView plaintext,
        MutableByteSpan output,
        ByteView& ciphertext);
    Result decryptAndHash(
        ByteView ciphertext,
        MutableByteSpan output,
        ByteView& plaintext);
    Result split();
    Result generateEphemeral(MutableByteSpan publicKey);
    Result mixDh();
    Result encryptWithKey(
        ByteView key,
        uint64_t& nonce,
        ByteView associatedData,
        ByteView plaintext,
        MutableByteSpan output,
        ByteView& ciphertext);
    Result decryptWithKey(
        ByteView key,
        uint64_t& nonce,
        ByteView associatedData,
        ByteView ciphertext,
        MutableByteSpan output,
        ByteView& plaintext);
    Result fail(Result result);
    void clearSecrets();

    INoiseCryptoProvider& crypto_;
    IRandom& random_;
    NoiseRole role_;
    NoiseSessionState state_;
    noise::NoiseNnPattern pattern_;
    uint8_t chainingKey_[kNoiseKeySize];
    uint8_t handshakeHash_[kNoiseKeySize];
    uint8_t handshakeKey_[kNoiseKeySize];
    uint64_t handshakeNonce_;
    uint8_t localPrivate_[kNoiseDhSize];
    uint8_t remotePublic_[kNoiseDhSize];
    uint8_t sendKey_[kNoiseKeySize];
    uint8_t receiveKey_[kNoiseKeySize];
    uint64_t sendNonce_;
    uint64_t receiveNonce_;
    bool handshakeHasKey_;
};

} // namespace security
} // namespace blinker

#endif
