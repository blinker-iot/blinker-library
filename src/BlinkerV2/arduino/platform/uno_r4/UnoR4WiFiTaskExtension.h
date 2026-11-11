#ifndef BLINKER_UNO_R4_WIFI_TASK_EXTENSION_H
#define BLINKER_UNO_R4_WIFI_TASK_EXTENSION_H

#include "../TaskModule.h"
#include "../../ports/uno_r4/UnoR4TaskStorage.h"
#include "../../ports/arduino/PortableEd25519.h"
#include "../../config/PortableServerKeys.h"
#include "../../internal/WiFiTaskExtension.h"

namespace blinker {
namespace integration {
namespace official_detail {

template <size_t Capacity>
class RenesasUnoTaskModule {
public:
    RenesasUnoTaskModule(ITaskEnvironment& environment, ITaskTarget& target,
        const TaskWritePolicy& policy, RenesasUnoPreferencesBlobBank& bank)
        : blob_(bank, policy.maximumCommitMillis), tasks_(blob_, environment, target, policy) {}
    Result begin() { return tasks_.begin(); }
    TaskEngine& engine() { return tasks_.engine(); }

private:
    RenesasUnoTaskSnapshotBlobStore<Capacity> blob_;
    TaskModule<Capacity> tasks_;
    RenesasUnoTaskModule(const RenesasUnoTaskModule&) = delete;
    RenesasUnoTaskModule& operator=(const RenesasUnoTaskModule&) = delete;
};

template <size_t Capacity>
struct RenesasUnoWifiTaskPort {
    typedef RenesasUnoTaskModule<Capacity> Module;
    typedef ArduinoEd25519DigestVerifier Crypto;
    typedef Ed25519ServerKeyRingVerifier Verifier;
    template <typename Stack>
    static RenesasUnoPreferencesBlobBank& storage(Stack& stack) { return stack.platform().storage(); }
    static const Ed25519ServerPublicKey* keys() { return official::serverSigningKeys; }
    static size_t keyCount() { return official::serverSigningKeysCount; }
};

template <typename Stack, size_t Capacity>
using RenesasUnoWifiTaskExtension = WifiTaskExtension<Stack, RenesasUnoWifiTaskPort<Capacity>>;

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
