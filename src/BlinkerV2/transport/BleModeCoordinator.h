#ifndef BLINKER_TRANSPORT_BLEMODECOORDINATOR_H
#define BLINKER_TRANSPORT_BLEMODECOORDINATOR_H

#include "../interface/ITransportLifecycle.h"
#include "../interface/IBleModePreparation.h"
#include "../interface/IBleModeLink.h"

namespace blinker {

enum class BleModeCoordinatorState : uint8_t {
    Stopped = 0U,
    Starting,
    Provisioning,
    Direct,
    Switching,
    RollingBack,
    Error
};

// Owns the transition between two logical protocols on one physical BLE link.
// switchTo() is synchronous and must be called from the application loop, not
// from a BLE/session callback. A switch always disconnects the old session.
class BleModeCoordinator {
public:
    BleModeCoordinator(
        IBleModeLink& link,
        ITransportLifecycle& provisioning,
        ITransportLifecycle& direct,
        IBleModePreparation* preparation = nullptr);

    Result start(const ble::ModeProfile& profile);
    Result switchTo(const ble::ModeProfile& profile);
    // Stops, re-prepares and starts the exact active profile. Product code
    // calls this from loop() after a provisioning disconnect/fault so a new
    // Noise and application session exists before advertising resumes.
    Result restartActive();
    void stop();
    void poll(uint32_t budgetMicros);

    BleModeCoordinatorState state() const { return state_; }
    TransportState activeTransportState() const {
        return activeEndpoint_ != nullptr
                   ? activeEndpoint_->state()
                   : TransportState::Stopped;
    }
    ble::ApplicationMode activeMode() const { return activeProfile_.mode; }
    ble::ModeProfile activeProfile() const { return activeProfile_; }
    size_t sessionCount() const { return link_.sessionCount(); }
    ErrorCode lastTransitionError() const { return lastTransitionError_; }
    uint32_t rollbackCount() const { return rollbackCount_; }

private:
    ITransportLifecycle* endpointFor(ble::ApplicationMode mode);
    static BleModeCoordinatorState activeState(ble::ApplicationMode mode);
    bool transitioning() const;
    Result startStopped(const ble::ModeProfile& profile);
    Result prepareAndStart(
        const ble::ModeProfile& profile,
        ITransportLifecycle& endpoint,
        bool rollback);
    Result restore(
        const ble::ModeProfile& profile,
        ITransportLifecycle& endpoint,
        ErrorCode targetError);

    IBleModeLink& link_;
    ITransportLifecycle& provisioning_;
    ITransportLifecycle& direct_;
    IBleModePreparation* preparation_;
    ITransportLifecycle* activeEndpoint_;
    ble::ModeProfile activeProfile_;
    BleModeCoordinatorState state_;
    ErrorCode lastTransitionError_;
    uint32_t rollbackCount_;
};

} // namespace blinker

#endif
