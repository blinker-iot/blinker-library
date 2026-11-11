#include <BlinkerV2Advanced.h>
#include <Esp32Crypto.h>
#include <HardwareRandom.h>

#include <string.h>

namespace {

class FixedRandom : public blinker::IRandom {
public:
    explicit FixedRandom(uint8_t seed) : seed_(seed), used_(false) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (used_ || output.data == nullptr || output.size != 32U) {
            return blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = static_cast<uint8_t>(seed_ + index);
        }
        used_ = true;
        return blinker::Result::success();
    }

private:
    uint8_t seed_;
    bool used_;
};

bool equals(const uint8_t* first, const uint8_t* second, size_t size) {
    return memcmp(first, second, size) == 0;
}

blinker::Esp32MbedTlsCryptoProvider crypto;
blinker::PlatformHardwareRandom hardwareRandom;
volatile bool noiseCryptoProbePassed = false;

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
    static const uint8_t serverPublicKey[65] = {
        0x04U, 0x6BU, 0x17U, 0xD1U, 0xF2U, 0xE1U, 0x2CU, 0x42U,
        0x47U, 0xF8U, 0xBCU, 0xE6U, 0xE5U, 0x63U, 0xA4U, 0x40U,
        0xF2U, 0x77U, 0x03U, 0x7DU, 0x81U, 0x2DU, 0xEBU, 0x33U,
        0xA0U, 0xF4U, 0xA1U, 0x39U, 0x45U, 0xD8U, 0x98U, 0xC2U,
        0x96U, 0x4FU, 0xE3U, 0x42U, 0xE2U, 0xFEU, 0x1AU, 0x7FU,
        0x9BU, 0x8EU, 0xE7U, 0xEBU, 0x4AU, 0x7CU, 0x0FU, 0x9EU,
        0x16U, 0x2BU, 0xCEU, 0x33U, 0x57U, 0x6BU, 0x31U, 0x5EU,
        0xCEU, 0xCBU, 0xB6U, 0x40U, 0x68U, 0x37U, 0xBFU, 0x51U,
        0xF5U};
    static const uint8_t serverSignature[64] = {
        0x70U, 0xF9U, 0x22U, 0x84U, 0x84U, 0xAAU, 0x23U, 0xBDU,
        0x81U, 0x61U, 0xBCU, 0xB2U, 0x1CU, 0x8EU, 0xD0U, 0x0DU,
        0x24U, 0x76U, 0x0BU, 0x00U, 0xF8U, 0xB3U, 0xA6U, 0x35U,
        0xC7U, 0x19U, 0x99U, 0x48U, 0xD5U, 0x98U, 0x61U, 0xFAU,
        0x73U, 0xD6U, 0x9CU, 0x52U, 0x5BU, 0xB0U, 0x6EU, 0x0CU,
        0xCCU, 0xA9U, 0x21U, 0x45U, 0xDDU, 0x0FU, 0x22U, 0x5AU,
        0x0CU, 0x34U, 0x06U, 0xA1U, 0x87U, 0x4EU, 0x3AU, 0x42U,
        0x08U, 0x0FU, 0xA3U, 0x38U, 0x4DU, 0x46U, 0xBBU, 0x32U};
    uint8_t serverDigest[32];
    for (size_t index = 0U; index < sizeof(serverDigest); ++index) {
        serverDigest[index] = static_cast<uint8_t>(index);
    }

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

    uint8_t aesKey[32] = {};
    uint8_t nonce[12] = {};
    uint8_t plaintext[16] = {};
    uint8_t ciphertext[32] = {};
    size_t written = 0U;
    if (result) {
        result = crypto.aes256GcmEncrypt(
            blinker::ByteView(aesKey, sizeof(aesKey)),
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
            blinker::ByteView(aesKey, sizeof(aesKey)),
            blinker::ByteView(nonce, sizeof(nonce)),
            blinker::ByteView(),
            blinker::ByteView(ciphertext, sizeof(ciphertext)),
            blinker::MutableByteSpan(decrypted, sizeof(decrypted)),
            written);
    }
    if (result &&
        (written != sizeof(plaintext) ||
         !equals(decrypted, plaintext, sizeof(plaintext)))) {
        result = blinker::Result::failure(blinker::ErrorCode::InternalError);
    }
    if (result) {
        ciphertext[sizeof(ciphertext) - 1U] ^= 0x01U;
        memset(decrypted, 0xA5, sizeof(decrypted));
        const blinker::Result tampered = crypto.aes256GcmDecrypt(
            blinker::ByteView(aesKey, sizeof(aesKey)),
            blinker::ByteView(nonce, sizeof(nonce)),
            blinker::ByteView(),
            blinker::ByteView(ciphertext, sizeof(ciphertext)),
            blinker::MutableByteSpan(decrypted, sizeof(decrypted)),
            written);
        uint8_t zeroes[sizeof(decrypted)] = {};
        if (tampered.code() != blinker::ErrorCode::AuthenticationRequired ||
            written != 0U ||
            !equals(decrypted, zeroes, sizeof(decrypted))) {
            result = blinker::Result::failure(
                blinker::ErrorCode::InternalError);
        }
    }
    const blinker::P256ServerPublicKey serverKeys[] = {
        blinker::P256ServerPublicKey(
            1U,
            blinker::ByteView(serverPublicKey, sizeof(serverPublicKey)))};
    blinker::P256ServerKeyRingVerifier serverVerifier(
        crypto,
        serverKeys,
        1U);
    if (result) {
        result = serverVerifier.verifyDigest(
            1U,
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
            blinker::ByteView(serverDigest, sizeof(serverDigest)),
            blinker::ByteView(serverSignature, sizeof(serverSignature)));
    }

    FixedRandom initiatorRandom(0x11U);
    FixedRandom responderRandom(0x51U);
    blinker::security::NoiseNnSession initiator(
        crypto,
        initiatorRandom);
    blinker::security::NoiseNnSession responder(
        crypto,
        responderRandom);
    if (result) {
        result = initiator.begin(
            blinker::security::NoiseRole::Initiator,
            blinker::noise::NoiseNnPattern::Nn,
            blinker::ByteView(),
            blinker::noise::localSecureNoisePrologue(
                blinker::noise::NoiseNnPattern::Nn));
    }
    if (result) {
        result = responder.begin(
            blinker::security::NoiseRole::Responder,
            blinker::noise::NoiseNnPattern::Nn,
            blinker::ByteView(),
            blinker::noise::localSecureNoisePrologue(
                blinker::noise::NoiseNnPattern::Nn));
    }
    uint8_t message1Bytes[64] = {};
    uint8_t message2Bytes[64] = {};
    uint8_t payloadBytes[8] = {};
    blinker::ByteView message1;
    blinker::ByteView message2;
    blinker::ByteView payload;
    if (result) {
        result = initiator.writeHandshake(
            blinker::ByteView(),
            blinker::MutableByteSpan(message1Bytes, sizeof(message1Bytes)),
            message1);
    }
    if (result) {
        result = responder.readHandshake(
            message1,
            blinker::MutableByteSpan(payloadBytes, sizeof(payloadBytes)),
            payload);
    }
    if (result) {
        result = responder.writeHandshake(
            blinker::ByteView(),
            blinker::MutableByteSpan(message2Bytes, sizeof(message2Bytes)),
            message2);
    }
    if (result) {
        result = initiator.readHandshake(
            message2,
            blinker::MutableByteSpan(payloadBytes, sizeof(payloadBytes)),
            payload);
    }
    uint8_t randomProbe[8] = {};
    if (result) {
        result = hardwareRandom.fill(
            blinker::MutableByteSpan(randomProbe, sizeof(randomProbe)));
    }
    noiseCryptoProbePassed = result.ok() && initiator.ready() && responder.ready();
}

void loop() {}
