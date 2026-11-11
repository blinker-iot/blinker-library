#include <cstdlib>
#include <cstring>
#include <iostream>
#include "BlinkerV2/arduino/ports/arduino/PortableEd25519.h"
using namespace blinker;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << '\n'; std::exit(1); } } while (0)

int main() {
    ArduinoEd25519DigestVerifier verifier;
    uint8_t key[32] = {0x58U}, digest[32] = {}, signature[64] = {0x58U};
    // Canonical base point and scalar 1: test port framing with a fake primitive,
    // NOT a cryptographically valid signature or hardware verification claim.
    std::memset(key + 1U, 0x66U, 31U);
    std::memset(signature + 1U, 0x66U, 31U);
    signature[32] = 1U;
    const ByteView publicKey(key, sizeof(key)), message(digest, sizeof(digest)),
                   signedDigest(signature, sizeof(signature));
    CHECK(verifier.verifyDigest(publicKey, message, signedDigest));
    CHECK(fake_ed25519::state().calls == 1U);
    CHECK(fake_ed25519::state().key == key && fake_ed25519::state().signature == signature &&
          fake_ed25519::state().digest == digest && fake_ed25519::state().digestSize == 32U);
    fake_ed25519::state().accepted = false;
    CHECK(verifier.verifyDigest(publicKey, message, signedDigest).code() == ErrorCode::AuthenticationRequired);
    CHECK(fake_ed25519::state().calls == 2U);
    CHECK(verifier.verifyDigest(ByteView(), message, signedDigest).code() == ErrorCode::InvalidArgument);
    CHECK(verifier.verifyDigest(publicKey, ByteView(digest, 31U), signedDigest).code() == ErrorCode::InvalidArgument);
    CHECK(verifier.verifyDigest(publicKey, message, ByteView(signature, 63U)).code() == ErrorCode::InvalidArgument);
    CHECK(fake_ed25519::state().calls == 2U);
    std::memset(key, 0U, sizeof(key));
    CHECK(verifier.verifyDigest(publicKey, message, signedDigest).code() == ErrorCode::AuthenticationRequired);
    key[0] = 0x58U; std::memset(key + 1U, 0x66U, 31U);
    signature[63] = 0xFFU;
    CHECK(verifier.verifyDigest(publicKey, message, signedDigest).code() == ErrorCode::AuthenticationRequired);
    CHECK(fake_ed25519::state().calls == 2U);
    std::cout << "portable signature-only port framing and canonical rejection passed (fake primitive)\n";
}
