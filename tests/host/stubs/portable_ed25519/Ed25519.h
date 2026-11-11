#ifndef TEST_PORTABLE_ED25519_H
#define TEST_PORTABLE_ED25519_H
#include <stddef.h>
#include <stdint.h>

namespace fake_ed25519 {
struct State {
    unsigned calls;
    bool accepted;
    const uint8_t* signature;
    const uint8_t* key;
    const uint8_t* digest;
    size_t digestSize;
    State() : calls(0U), accepted(true), signature(nullptr), key(nullptr),
              digest(nullptr), digestSize(0U) {}
};
inline State& state() { static State value; return value; }
}

class Ed25519 {
public:
    static bool verify(const uint8_t* signature, const uint8_t* key,
                       const void* digest, size_t size) {
        fake_ed25519::State& value = fake_ed25519::state();
        ++value.calls;
        value.signature = signature; value.key = key;
        value.digest = static_cast<const uint8_t*>(digest); value.digestSize = size;
        return value.accepted;
    }
};
#endif
