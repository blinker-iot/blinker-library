#ifndef BLINKER_ESP32_WIFI_TASK_EXTENSION_H
#define BLINKER_ESP32_WIFI_TASK_EXTENSION_H

#include "Esp32TaskModule.h"
#include "../../ports/esp32/Esp32Crypto.h"
#include "../../config/Esp32ServerKeys.h"
#include "../../internal/WiFiTaskExtension.h"

namespace blinker {
namespace integration {
namespace official_detail {

template <size_t Capacity>
struct Esp32WifiTaskPort {
    typedef Esp32TaskModule<Capacity> Module;
    typedef Esp32MbedTlsCryptoProvider Crypto;
    typedef P256ServerKeyRingVerifier Verifier;
    template <typename Stack>
    static Esp32NvsBlobStoreConfig storage(Stack&) { return Esp32NvsBlobStoreConfig(); }
    static const P256ServerPublicKey* keys() { return official::serverSigningKeys; }
    static size_t keyCount() { return official::serverSigningKeysCount; }
};

template <typename Stack, size_t Capacity>
using Esp32WifiTaskExtension = WifiTaskExtension<Stack, Esp32WifiTaskPort<Capacity>>;

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
