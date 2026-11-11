#ifndef BLINKER_TEST_DETERMINISTIC_NOISE_CRYPTO_H
#define BLINKER_TEST_DETERMINISTIC_NOISE_CRYPTO_H

#include <BlinkerV2Advanced.h>
#include <cstring>

// Test-only deterministic primitive provider. It deliberately is not AES-GCM
// or X25519 and must never be shipped. Shared state-machine/transport tests
// use it for fault injection; independent golden/provider gates verify crypto.
class DeterministicTestCrypto : public blinker::INoiseCryptoProvider {
public:
    blinker::Result x25519PublicKey(
        blinker::ByteView privateKey,
        blinker::MutableByteSpan publicKey) override {
        if (privateKey.data == nullptr || privateKey.size != 32U ||
            publicKey.data == nullptr || publicKey.size < 32U) {
            return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
        }
        std::memcpy(publicKey.data, privateKey.data, 32U);
        return blinker::Result::success();
    }

    blinker::Result x25519(
        blinker::ByteView privateKey,
        blinker::ByteView remotePublicKey,
        blinker::MutableByteSpan sharedSecret) override {
        if (privateKey.data == nullptr || privateKey.size != 32U ||
            remotePublicKey.data == nullptr || remotePublicKey.size != 32U ||
            sharedSecret.data == nullptr || sharedSecret.size < 32U) {
            return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
        }
        const bool localFirst =
            std::memcmp(privateKey.data, remotePublicKey.data, 32U) <= 0;
        blinker::Sha256 hash;
        blinker::Result result = hash.update(
            localFirst ? privateKey : remotePublicKey);
        if (result) {
            result = hash.update(localFirst ? remotePublicKey : privateKey);
        }
        if (result) result = hash.finish(sharedSecret);
        return result;
    }

    blinker::Result aes256GcmEncrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView plaintext,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!validInputs(key, nonce, associatedData) ||
            (plaintext.data == nullptr && !plaintext.empty()) ||
            output.data == nullptr ||
            output.size < plaintext.size + 16U) {
            return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
        }
        blinker::Result result = xorStream(
            key,
            nonce,
            associatedData,
            plaintext,
            blinker::MutableByteSpan(output.data, plaintext.size));
        uint8_t tag[32] = {};
        if (result) {
            result = computeTag(
                key,
                nonce,
                associatedData,
                blinker::ByteView(output.data, plaintext.size),
                tag);
        }
        if (result) {
            std::memcpy(output.data + plaintext.size, tag, 16U);
            written = plaintext.size + 16U;
        }
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!validInputs(key, nonce, associatedData) ||
            ciphertextAndTag.data == nullptr || ciphertextAndTag.size < 16U) {
            return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
        }
        const size_t ciphertextSize = ciphertextAndTag.size - 16U;
        if ((ciphertextSize != 0U && output.data == nullptr) ||
            output.size < ciphertextSize) {
            return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
        }
        uint8_t tag[32] = {};
        blinker::Result result = computeTag(
            key,
            nonce,
            associatedData,
            blinker::ByteView(ciphertextAndTag.data, ciphertextSize),
            tag);
        if (result && !blinker::constantTimeEqual(
                          blinker::ByteView(tag, 16U),
                          blinker::ByteView(
                              ciphertextAndTag.data + ciphertextSize,
                              16U))) {
            result = blinker::Result::failure(
                blinker::ErrorCode::AuthenticationRequired);
        }
        if (result) {
            result = xorStream(
                key,
                nonce,
                associatedData,
                blinker::ByteView(ciphertextAndTag.data, ciphertextSize),
                output);
        }
        if (result) written = ciphertextSize;
        blinker::secureZero(blinker::MutableByteSpan(tag, sizeof(tag)));
        return result;
    }

private:
    static bool validInputs(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData) {
        return key.data != nullptr && key.size == 32U &&
               nonce.data != nullptr && nonce.size == 12U &&
               (associatedData.data != nullptr || associatedData.empty());
    }

    static blinker::Result beginMac(
        blinker::HmacSha256& hmac,
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData) {
        blinker::Result result = hmac.begin(key);
        if (result) result = hmac.update(nonce);
        if (result) result = hmac.update(associatedData);
        return result;
    }

    static blinker::Result computeTag(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertext,
        uint8_t output[32]) {
        blinker::HmacSha256 hmac;
        blinker::Result result = beginMac(
            hmac,
            key,
            nonce,
            associatedData);
        if (result) result = hmac.update(ciphertext);
        const uint8_t marker = 0xA5U;
        if (result) result = hmac.update(blinker::ByteView(&marker, 1U));
        if (result) {
            result = hmac.finish(blinker::MutableByteSpan(output, 32U));
        }
        return result;
    }

    static blinker::Result xorStream(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView input,
        blinker::MutableByteSpan output) {
        size_t offset = 0U;
        uint32_t counter = 0U;
        while (offset < input.size) {
            const uint8_t encodedCounter[] = {
                static_cast<uint8_t>(counter >> 24U),
                static_cast<uint8_t>(counter >> 16U),
                static_cast<uint8_t>(counter >> 8U),
                static_cast<uint8_t>(counter)
            };
            uint8_t block[32] = {};
            blinker::HmacSha256 hmac;
            blinker::Result result = beginMac(
                hmac,
                key,
                nonce,
                associatedData);
            if (result) {
                result = hmac.update(blinker::ByteView(
                    encodedCounter,
                    sizeof(encodedCounter)));
            }
            if (result) {
                result = hmac.finish(blinker::MutableByteSpan(
                    block,
                    sizeof(block)));
            }
            if (!result) {
                blinker::secureZero(blinker::MutableByteSpan(
                    block,
                    sizeof(block)));
                return result;
            }
            const size_t remaining = input.size - offset;
            const size_t count = remaining < sizeof(block)
                                     ? remaining
                                     : sizeof(block);
            for (size_t index = 0U; index < count; ++index) {
                output.data[offset + index] = static_cast<uint8_t>(
                    input.data[offset + index] ^ block[index]);
            }
            blinker::secureZero(blinker::MutableByteSpan(
                block,
                sizeof(block)));
            offset += count;
            ++counter;
        }
        return blinker::Result::success();
    }
};

#endif
