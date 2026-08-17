#ifndef BLINKER_RENESAS_UNO_CRYPTO_ADAPTER_H
#define BLINKER_RENESAS_UNO_CRYPTO_ADAPTER_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "BlinkerRenesasUnoCryptoAdapter requires an Arduino Renesas target"
#endif

#include <BlinkerV2/interface/IP256DigestVerifier.h>
#include <SoftwareATSE.h>

namespace blinker {

// Uses the UNO R4 WiFi connectivity module's SoftwareATSE verifier. The
// pinned SEC1 key remains in Flash; only its 64-byte X||Y body crosses the
// modem command boundary for a verification call.
class RenesasUnoP256Verifier final : public IP256DigestVerifier {
public:
    explicit RenesasUnoP256Verifier(SoftwareATSEClass& device = SATSE)
        : device_(device), active_(false) {}
    ~RenesasUnoP256Verifier();

    Result begin();
    void end();
    bool active() const { return active_; }

    Result verifyDigest(
        ByteView publicKey,
        ByteView digest,
        ByteView signature) override;

private:
    SoftwareATSEClass& device_;
    bool active_;

    RenesasUnoP256Verifier(const RenesasUnoP256Verifier&);
    RenesasUnoP256Verifier& operator=(const RenesasUnoP256Verifier&);
};

} // namespace blinker

#endif
