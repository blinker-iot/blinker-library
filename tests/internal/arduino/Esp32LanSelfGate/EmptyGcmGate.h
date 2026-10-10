#pragma once
#include <BlinkerV2/arduino/ports/esp32/Esp32Crypto.h>
#include <cstring>

// Public zero-key/zero-nonce vectors, never used for a live session. Generated
// independently with AES-256-GCM (Node/OpenSSL), empty plaintext, AAD [] / [1,2,3].
inline bool emptyGcmGate() {
    using namespace blinker;
    const uint8_t expected[2][16] = {
        {0x53,0x0f,0x8a,0xfb,0xc7,0x45,0x36,0xb9,0xa9,0x63,0xb4,0xf1,0xc4,0xcb,0x73,0x8b},
        {0xe9,0x76,0xd1,0x7c,0x1a,0xb4,0x23,0xca,0x0a,0xb4,0x9b,0x39,0x93,0x4a,0x56,0x09}};
    uint8_t key[32] = {}, nonce[12] = {}, tag[16] = {};
    const uint8_t aad[] = {1,2,3};
    Esp32MbedTlsCryptoProvider crypto;
    for (unsigned index = 0; index < 2; ++index) {
        const ByteView associated = index ? ByteView(aad, sizeof(aad)) : ByteView();
        size_t written = 0U;
        if (!crypto.aes256GcmEncrypt(ByteView(key, sizeof(key)), ByteView(nonce, sizeof(nonce)),
            associated, ByteView(), MutableByteSpan(tag, sizeof(tag)), written) ||
            written != sizeof(tag) || std::memcmp(tag, expected[index], sizeof(tag)) != 0) return false;
        if (!crypto.aes256GcmDecrypt(ByteView(key, sizeof(key)), ByteView(nonce, sizeof(nonce)),
            associated, ByteView(tag, sizeof(tag)), MutableByteSpan(), written) || written != 0U) return false;
        tag[0] ^= 1U;
        if (crypto.aes256GcmDecrypt(ByteView(key, sizeof(key)), ByteView(nonce, sizeof(nonce)),
            associated, ByteView(tag, sizeof(tag)), MutableByteSpan(), written).code() !=
            ErrorCode::AuthenticationRequired || written != 0U) return false;
    }
    return true;
}
