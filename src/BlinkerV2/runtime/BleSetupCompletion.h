#ifndef BLINKER_RUNTIME_BLESETUPCOMPLETION_H
#define BLINKER_RUNTIME_BLESETUPCOMPLETION_H

#include "../identity/ControllerCredentialStore.h"
#include "../provisioning/LocalSetupSagaStore.h"

namespace blinker {

// Small read-only seam used by the shared BLE mode lifecycle. It answers only
// whether provisioning is durable; authorization details remain in the
// platform Ticket saga or the Local-domain controller store.
class IBleSetupCompletion {
public:
    virtual ~IBleSetupCompletion() {}
    virtual Result load(bool& complete) = 0;
};

class LocalSetupSagaCompletion final : public IBleSetupCompletion {
public:
    explicit LocalSetupSagaCompletion(LocalSetupSagaStore& store)
        : store_(store) {}
    Result load(bool& complete) override;

private:
    LocalSetupSagaStore& store_;
};

class LocalControllerCredentialCompletion final
    : public IBleSetupCompletion {
public:
    explicit LocalControllerCredentialCompletion(
        ControllerCredentialStore& store)
        : store_(store) {}
    Result load(bool& complete) override;

private:
    ControllerCredentialStore& store_;
};

} // namespace blinker

#endif
