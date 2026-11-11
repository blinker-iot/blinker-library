#ifndef BLINKER_ESP32_PROVISIONED_TASK_EXTENSION_H
#define BLINKER_ESP32_PROVISIONED_TASK_EXTENSION_H

#include "Esp32WiFiTaskExtension.h"
#include "../../../runtime/TaskRootOwnerGate.h"
#include "../../../identity/DeviceAccessStore.h"

namespace blinker {
namespace integration {
namespace official_detail {

// Internal SoftAP/P256 four-slot composition. One original task graph and
// Cloud carrier; fixed BOTH roots, never inferred from an incoming request.
template <typename Stack, size_t Capacity>
class Esp32ProvisionedTaskExtension final :
    public WifiTaskExtension<Stack, Esp32WifiTaskPort<Capacity>, BorrowedTaskProductOwner>,
    public ITaskProductOwner,
    public IDeviceAccessChangeObserver,
    public IInitialCloudKeyInstaller {
    typedef WifiTaskExtension<Stack, Esp32WifiTaskPort<Capacity>, BorrowedTaskProductOwner> Base;
public:
    enum : bool { guardedRoots = true };
    Esp32ProvisionedTaskExtension(Stack& stack, IProductLifecycle& product)
        : Base(stack, product, BorrowedTaskProductOwner(this)),
          access_(stack.platform().deviceKeys()), selector_(), gate_(), active_(false), held_(false),
          initialKeyArmed_(false), initialKeyFailed_(false) {
        this->rootSources().controllers = &access_;
        this->rootSources().adminControllerId = ByteView(selector_.controllerId, sizeof(selector_.controllerId));
        this->setTaskReady(false);
        access_.setChangeObserver(this);
    }
    ~Esp32ProvisionedTaskExtension() {
        // Product owns native callback quiescence BEFORE reaching this point.
        access_.setChangeObserver(nullptr);
    }
    void provisioning() { active_ = false; this->setTaskReady(false); gate_.markDirty(); }
    void active() { active_ = true; gate_.markDirty(); }
    bool enterTaskOwner() override {
        // Inactive/provisioning is normally a no-op owner, but must never
        // bypass a first-key maintenance transaction or reenter dispatch.
        if (initialKeyArmed_) return false;
        held_ = false;
        if (!active_) return true; // Provisioning must not starve the native writer.
        if (!gate_.enterOwner()) return false;
        held_ = true;
        if (gate_.takeDirty()) {
            this->setTaskReady(false);
            selector_ = DeviceAccessAdminSelector();
            this->rootSources().accessEpoch = 0U;
            if (!gate_.uncertain() && !initialKeyFailed_) {
                const Result result = access_.loadAdminSelector(selector_);
                if (result) {
                    this->rootSources().accessEpoch = selector_.accessEpoch;
                    this->setTaskReady(true);
                }
            }
        }
        return true;
    }
    void leaveTaskOwner() override {
        if (held_) { held_ = false; gate_.leaveOwner(); }
    }
    Result beginDeviceAccessChange() override { return gate_.beginDeviceAccessChange(); }
    void endDeviceAccessChange(Result outcome) override { gate_.endDeviceAccessChange(outcome); }
    Result beginCooperativePresenceChange() override {
        if (!active_ || !held_) return Result::failure(ErrorCode::NotConfigured);
        const Result result = gate_.beginCooperativePresenceChange();
        // Invalidate BEFORE mutation, including later frames in this same poll.
        if (result) this->setTaskReady(false);
        return result;
    }
    void endCooperativePresenceChange(Result outcome) override {
        gate_.endCooperativePresenceChange(outcome);
    }
    Result beginCooperativeInitialCloudKeyChange() override {
        // The product's own synchronous maintenance transaction, NOT a native
        // caller, is the only owner permitted to borrow this dedicated fence.
        if (!initialKeyArmed_ || !Base::initialCloudKeyCommitActive())
            return Result::failure(ErrorCode::NotConfigured);
        const Result result = gate_.beginCooperativeInitialCloudKeyChange();
        if (result) this->setTaskReady(false);
        return result;
    }
    void endCooperativeInitialCloudKeyChange(Result outcome) override {
        gate_.endCooperativeInitialCloudKeyChange(outcome);
    }
    // Trusted internal composition seam only. A network caller still needs a
    // separately committed Method2/intent binding; this is not that permission.
    // Provisioning is deliberately allowed: tasks must remain uninitialized.
    Result installInitialCloudKeyVerified(const DeviceKey& candidate,
        uint32_t expectedEpoch, const ControllerCredential& expectedAdmin) {
        return installInitialCloudKey(candidate, expectedEpoch, expectedAdmin, nullptr);
    }
    Result installInitialCloudKeyAuthorized(const DeviceKey& candidate,
        uint32_t expectedEpoch, const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard& guard) override {
        return installInitialCloudKey(candidate, expectedEpoch, expectedAdmin, &guard);
    }
    bool enterMaintenance() { return gate_.enterOwner(); }
    void leaveMaintenance() { gate_.leaveOwner(); }
    bool quiescent() const { return gate_.idle(); }

private:
    Result installInitialCloudKey(const DeviceKey& candidate,
        uint32_t expectedEpoch, const ControllerCredential& expectedAdmin,
        IInitialCloudKeyCommitGuard* guard) {
        if (initialKeyArmed_ || held_) return Result::failure(ErrorCode::WouldBlock);
        if (initialKeyFailed_) return Result::failure(ErrorCode::StateConflict);
        if (!access_.hasChangeObserver(this)) return Result::failure(ErrorCode::NotConfigured);
        if (!enterMaintenance()) return Result::failure(ErrorCode::WouldBlock);
        initialKeyArmed_ = true; // Exclude reentry during every validation/read.
        this->setTaskReady(false); // Stops management and invalidates cached roots BEFORE any write.
        Result result = Result::failure(ErrorCode::StateConflict);
        if (!gate_.uncertain()) {
            DeviceAccessAdminSelector current;
            result = validateControllerCredential(expectedAdmin);
            if (result && (expectedEpoch == 0U || expectedAdmin.domain != ControllerCredentialDomain::PlatformAccess ||
                expectedAdmin.ownershipGeneration != expectedEpoch || expectedAdmin.permissions != kControllerPermissionAll))
                result = Result::failure(ErrorCode::AuthenticationRequired);
            if (result) result = access_.loadAdminSelector(current);
            if (result && (current.accessEpoch != expectedEpoch || !constantTimeEqual(
                ByteView(current.controllerId, sizeof(current.controllerId)), expectedAdmin.id())))
                result = Result::failure(ErrorCode::StateConflict);
            if (result) {
                result = Base::installInitialCloudKeyVerified(access_, candidate, expectedEpoch, expectedAdmin, guard);
            }
        }
        // Known no-write refusals do not retire existing task ownership. Only
        // the original root/TaskStore/port failure state poisons this instance;
        // a same-handle read or stop/start can never clear that uncertainty.
        initialKeyArmed_ = false;
        if (gate_.uncertain() || Base::taskStorageFailed()) initialKeyFailed_ = true;
        gate_.markDirty(); // Original lifecycle refresh, never a Key retry.
        leaveMaintenance();
        return result;
    }
    DeviceAccessStore& access_;
    DeviceAccessAdminSelector selector_;
    TaskRootOwnerGate gate_;
    bool active_, held_, initialKeyArmed_, initialKeyFailed_;
    Esp32ProvisionedTaskExtension(const Esp32ProvisionedTaskExtension&) = delete;
    Esp32ProvisionedTaskExtension& operator=(const Esp32ProvisionedTaskExtension&) = delete;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker
#endif
