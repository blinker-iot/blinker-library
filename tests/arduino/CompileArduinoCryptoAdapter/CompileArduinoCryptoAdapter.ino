#include <BlinkerV2Advanced.h>
#include <PortableCrypto.h>

#include <string.h>

namespace {

blinker::ArduinoCryptoProvider crypto;
volatile bool cryptoProbePassed = false;

bool equals(const uint8_t* first, const uint8_t* second, size_t size) {
    return memcmp(first, second, size) == 0;
}

} // namespace

void setup() {
    static const uint8_t alicePrivate[32] = {
        0x77U, 0x07U, 0x6DU, 0x0AU, 0x73U, 0x18U, 0xA5U, 0x7DU,
        0x3CU, 0x16U, 0xC1U, 0x72U, 0x51U, 0xB2U, 0x66U, 0x45U,
        0xDFU, 0x4CU, 0x2FU, 0x87U, 0xEBU, 0xC0U, 0x99U, 0x2AU,
        0xB1U, 0x77U, 0xFBU, 0xA5U, 0x1DU, 0xB9U, 0x2CU, 0x2AU};
    static const uint8_t alicePublic[32] = {
        0x85U, 0x20U, 0xF0U, 0x09U, 0x89U, 0x30U, 0xA7U, 0x54U,
        0x74U, 0x8BU, 0x7DU, 0xDCU, 0xB4U, 0x3EU, 0xF7U, 0x5AU,
        0x0DU, 0xBFU, 0x3AU, 0x0DU, 0x26U, 0x38U, 0x1AU, 0xF4U,
        0xEBU, 0xA4U, 0xA9U, 0x8EU, 0xAAU, 0x9BU, 0x4EU, 0x6AU};
    static const uint8_t bobPublic[32] = {
        0xDEU, 0x9EU, 0xDBU, 0x7DU, 0x7BU, 0x7DU, 0xC1U, 0xB4U,
        0xD3U, 0x5BU, 0x61U, 0xC2U, 0xECU, 0xE4U, 0x35U, 0x37U,
        0x3FU, 0x83U, 0x43U, 0xC8U, 0x5BU, 0x78U, 0x67U, 0x4DU,
        0xADU, 0xFCU, 0x7EU, 0x14U, 0x6FU, 0x88U, 0x2BU, 0x4FU};
    static const uint8_t sharedExpected[32] = {
        0x4AU, 0x5DU, 0x9DU, 0x5BU, 0xA4U, 0xCEU, 0x2DU, 0xE1U,
        0x72U, 0x8EU, 0x3BU, 0xF4U, 0x80U, 0x35U, 0x0FU, 0x25U,
        0xE0U, 0x7EU, 0x21U, 0xC9U, 0x47U, 0xD1U, 0x9EU, 0x33U,
        0x76U, 0xF0U, 0x9BU, 0x3CU, 0x1EU, 0x16U, 0x17U, 0x42U};
    static const uint8_t aesExpected[32] = {
        0xCEU, 0xA7U, 0x40U, 0x3DU, 0x4DU, 0x60U, 0x6BU, 0x6EU,
        0x07U, 0x4EU, 0xC5U, 0xD3U, 0xBAU, 0xF3U, 0x9DU, 0x18U,
        0xD0U, 0xD1U, 0xC8U, 0xA7U, 0x99U, 0x99U, 0x6BU, 0xF0U,
        0x26U, 0x5BU, 0x98U, 0xB5U, 0xD4U, 0x8AU, 0xB9U, 0x19U};
    static const uint8_t serverPublicKey[32] = {
        0x03U, 0xA1U, 0x07U, 0xBFU, 0xF3U, 0xCEU, 0x10U, 0xBEU,
        0x1DU, 0x70U, 0xDDU, 0x18U, 0xE7U, 0x4BU, 0xC0U, 0x99U,
        0x67U, 0xE4U, 0xD6U, 0x30U, 0x9BU, 0xA5U, 0x0DU, 0x5FU,
        0x1DU, 0xDCU, 0x86U, 0x64U, 0x12U, 0x55U, 0x31U, 0xB8U};
    static const uint8_t serverSignature[64] = {
        0xB3U, 0x35U, 0xDCU, 0x5EU, 0xAEU, 0x70U, 0xB4U, 0x87U,
        0x64U, 0xF1U, 0x12U, 0x07U, 0xBBU, 0xAAU, 0x62U, 0x75U,
        0x7CU, 0xBBU, 0x94U, 0xE8U, 0x20U, 0xCEU, 0xBAU, 0x37U,
        0x6AU, 0xEBU, 0x62U, 0xA5U, 0x81U, 0x5DU, 0xFAU, 0xBEU,
        0x78U, 0x42U, 0x17U, 0x75U, 0x88U, 0x36U, 0xCAU, 0x28U,
        0x50U, 0x4AU, 0x36U, 0xA2U, 0x16U, 0x0AU, 0xBBU, 0xDCU,
        0x9EU, 0x78U, 0x74U, 0xFEU, 0x1DU, 0x65U, 0x24U, 0xA5U,
        0x86U, 0x65U, 0xEFU, 0x2CU, 0x42U, 0x6DU, 0xA3U, 0x0EU};

    uint8_t publicKey[32] = {};
    uint8_t shared[32] = {};
    blinker::Result result = crypto.x25519PublicKey(
        blinker::ByteView(alicePrivate, sizeof(alicePrivate)),
        blinker::MutableByteSpan(publicKey, sizeof(publicKey)));
    if (result && !equals(publicKey, alicePublic, sizeof(publicKey))) {
        result = blinker::Result::failure(blinker::ErrorCode::InternalError);
    }
    if (result) {
        result = crypto.x25519(
            blinker::ByteView(alicePrivate, sizeof(alicePrivate)),
            blinker::ByteView(bobPublic, sizeof(bobPublic)),
            blinker::MutableByteSpan(shared, sizeof(shared)));
    }
    if (result && !equals(shared, sharedExpected, sizeof(shared))) {
        result = blinker::Result::failure(blinker::ErrorCode::InternalError);
    }

    uint8_t key[32] = {};
    uint8_t nonce[12] = {};
    uint8_t plaintext[16] = {};
    uint8_t ciphertext[32] = {};
    size_t written = 0U;
    if (result) {
        result = crypto.aes256GcmEncrypt(
            blinker::ByteView(key, sizeof(key)),
            blinker::ByteView(nonce, sizeof(nonce)),
            blinker::ByteView(),
            blinker::ByteView(plaintext, sizeof(plaintext)),
            blinker::MutableByteSpan(ciphertext, sizeof(ciphertext)),
            written);
    }
    if (result &&
        (written != sizeof(aesExpected) ||
         !equals(ciphertext, aesExpected, sizeof(aesExpected)))) {
        result = blinker::Result::failure(blinker::ErrorCode::InternalError);
    }

    uint8_t decrypted[16] = {};
    if (result) {
        result = crypto.aes256GcmDecrypt(
            blinker::ByteView(key, sizeof(key)),
            blinker::ByteView(nonce, sizeof(nonce)),
            blinker::ByteView(),
            blinker::ByteView(ciphertext, sizeof(ciphertext)),
            blinker::MutableByteSpan(decrypted, sizeof(decrypted)),
            written);
    }
    if (result && !equals(decrypted, plaintext, sizeof(plaintext))) {
        result = blinker::Result::failure(blinker::ErrorCode::InternalError);
    }
    if (result) {
        ciphertext[sizeof(ciphertext) - 1U] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        const blinker::Result tampered = crypto.aes256GcmDecrypt(
            blinker::ByteView(key, sizeof(key)),
            blinker::ByteView(nonce, sizeof(nonce)),
            blinker::ByteView(),
            blinker::ByteView(ciphertext, sizeof(ciphertext)),
            blinker::MutableByteSpan(decrypted, sizeof(decrypted)),
            written);
        uint8_t zeroes[sizeof(decrypted)] = {};
        if (tampered.code() != blinker::ErrorCode::AuthenticationRequired ||
            written != 0U || !equals(decrypted, zeroes, sizeof(decrypted))) {
            result = blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
    }
    uint8_t digest[32];
    for (size_t index = 0U; index < sizeof(digest); ++index) {
        digest[index] = static_cast<uint8_t>(index);
    }
    const blinker::Ed25519ServerPublicKey serverKeys[] = {
        blinker::Ed25519ServerPublicKey(
            1U,
            blinker::ByteView(serverPublicKey, sizeof(serverPublicKey)))};
    blinker::Ed25519ServerKeyRingVerifier serverVerifier(
        crypto,
        serverKeys,
        1U);
    if (result) {
        result = serverVerifier.verifyDigest(
            1U,
            blinker::ServerSignatureAlgorithm::Ed25519,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(serverSignature, sizeof(serverSignature)));
    }
    if (result) {
        uint8_t tampered[sizeof(serverSignature)];
        memcpy(tampered, serverSignature, sizeof(tampered));
        tampered[0] ^= 0x01U;
        const blinker::Result rejected = serverVerifier.verifyDigest(
            1U,
            blinker::ServerSignatureAlgorithm::Ed25519,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(tampered, sizeof(tampered)));
        if (rejected.code() != blinker::ErrorCode::AuthenticationRequired) {
            result = blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
    }
    cryptoProbePassed = result.ok();
}

void loop() {}
