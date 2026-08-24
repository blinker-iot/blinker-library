#ifndef BLINKER_RUNTIME_BLESETUPCOMPLETION_H
#define BLINKER_RUNTIME_BLESETUPCOMPLETION_H

#include "../interface/IAccessEpochSource.h"

namespace blinker {

// Small read-only seam used by the shared BLE mode lifecycle. It answers only
// whether the atomic access root is durable.
class IBleSetupCompletion {
public:
    virtual ~IBleSetupCompletion() {}
    virtual Result load(bool& complete) = 0;
};

class DeviceAccessSetupCompletion final : public IBleSetupCompletion {
public:
    explicit DeviceAccessSetupCompletion(IAccessEpochSource& access)
        : access_(access) {}
    Result load(bool& complete) override;

private:
    IAccessEpochSource& access_;
};

} // namespace blinker

#endif
