#ifndef BLINKER_WIO_TERMINAL_WIFI_TASK_EXTENSION_H
#define BLINKER_WIO_TERMINAL_WIFI_TASK_EXTENSION_H

#include "../TaskModule.h"
#include "../../ports/wio_terminal/WioTerminalTaskStorage.h"
#include "../../ports/arduino/PortableEd25519.h"
#include "../../config/PortableServerKeys.h"
#include "../../internal/WiFiTaskExtension.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Optional task storage borrows the platform's QSPI owner through its own lease.
// It never opens/closes the credential bank or changes its snapshot format.
template <size_t Capacity>
class WioTerminalTaskModule {
public:
    WioTerminalTaskModule(ITaskEnvironment& environment, ITaskTarget& target,
        const TaskWritePolicy& policy, WioTerminalFlashBlobBank& bank)
        : blob_(bank, BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED == 1,
                policy.maximumCommitMillis),
          tasks_(blob_, environment, target, policy), begun_(false) {
        static_assert(Capacity == 4U, "Wio task candidate admits four slots only");
    }
    ~WioTerminalTaskModule() { blob_.end(); }

    Result begin() {
        if (begun_) return Result::failure(ErrorCode::AlreadyExists);
        begun_ = true; // Never reopen a potentially uncertain task store.
        Result result = blob_.begin();
        return result ? tasks_.begin() : result;
    }
    TaskEngine& engine() { return tasks_.engine(); }

private:
    WioTerminalTaskSnapshotBlobStore<Capacity> blob_;
    TaskModule<Capacity> tasks_;
    bool begun_;
    WioTerminalTaskModule(const WioTerminalTaskModule&) = delete;
    WioTerminalTaskModule& operator=(const WioTerminalTaskModule&) = delete;
};

template <size_t Capacity>
struct WioTerminalWifiTaskPort {
    typedef WioTerminalTaskModule<Capacity> Module;
    typedef ArduinoEd25519DigestVerifier Crypto;
    typedef Ed25519ServerKeyRingVerifier Verifier;
    template <typename Stack>
    static WioTerminalFlashBlobBank& storage(Stack& stack) {
        return stack.platform().storage();
    }
    static const Ed25519ServerPublicKey* keys() { return official::serverSigningKeys; }
    static size_t keyCount() { return official::serverSigningKeysCount; }
};

template <typename Stack, size_t Capacity>
using WioTerminalWifiTaskExtension = WifiTaskExtension<Stack, WioTerminalWifiTaskPort<Capacity>>;

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
