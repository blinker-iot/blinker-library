#ifndef BLINKER_V2_ARDUINO_PORTABLE_SERVER_KEYS_H
#define BLINKER_V2_ARDUINO_PORTABLE_SERVER_KEYS_H

#include <BlinkerV2/security/Ed25519ServerKeyRingVerifier.h>

#define BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE 1

namespace blinker {
namespace official {

// Build/test key only. Release engineering replaces this key ring before a
// production package is published.
static const uint8_t serverSigningKeyBytes[] = {
    0xD7U, 0x5AU, 0x98U, 0x01U, 0x82U, 0xB1U, 0x0AU, 0xB7U,
    0xD5U, 0x4BU, 0xFEU, 0xD3U, 0xC9U, 0x64U, 0x07U, 0x3AU,
    0x0EU, 0xE1U, 0x72U, 0xF3U, 0xDAU, 0xA6U, 0x23U, 0x25U,
    0xAFU, 0x02U, 0x1AU, 0x68U, 0xF7U, 0x07U, 0x51U, 0x1AU
};
static const Ed25519ServerPublicKey serverSigningKeys[] = {
    Ed25519ServerPublicKey(
        1U,
        ByteView(serverSigningKeyBytes, sizeof(serverSigningKeyBytes)))
};
static const size_t serverSigningKeysCount =
    sizeof(serverSigningKeys) / sizeof(serverSigningKeys[0]);

} // namespace official
} // namespace blinker

#endif
