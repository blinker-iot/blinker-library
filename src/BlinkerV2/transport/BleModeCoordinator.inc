#include "BleModeCoordinator.h"

namespace blinker {

BleModeCoordinator::BleModeCoordinator(
    IBleModeLink& link,
    ITransportLifecycle& provisioning,
    ITransportLifecycle& direct,
    IBleModePreparation* preparation)
    : link_(link),
      provisioning_(provisioning),
      direct_(direct),
      preparation_(preparation),
      activeEndpoint_(nullptr),
      activeProfile_(),
      state_(BleModeCoordinatorState::Stopped),
      lastTransitionError_(ErrorCode::Ok),
      rollbackCount_(0U) {}

Result BleModeCoordinator::start(const ble::ModeProfile& profile) {
    if (state_ != BleModeCoordinatorState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    return startStopped(profile);
}

Result BleModeCoordinator::switchTo(const ble::ModeProfile& profile) {
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    if (transitioning()) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (state_ == BleModeCoordinatorState::Stopped) {
        return startStopped(profile);
    }
    if (state_ == BleModeCoordinatorState::Error ||
        activeEndpoint_ == nullptr) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (ble::modeProfilesEqual(activeProfile_, profile)) {
        return Result::success();
    }

    ITransportLifecycle* target = endpointFor(profile.mode);
    if (target == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    ITransportLifecycle* previousEndpoint = activeEndpoint_;
    const ble::ModeProfile previousProfile = activeProfile_;
    state_ = BleModeCoordinatorState::Switching;
    activeEndpoint_ = nullptr;
    activeProfile_ = ble::ModeProfile();
    previousEndpoint->stop();

    result = link_.configureBleProfile(profile);
    if (result) result = prepareAndStart(profile, *target, false);
    if (result) {
        activeEndpoint_ = target;
        activeProfile_ = profile;
        state_ = activeState(profile.mode);
        lastTransitionError_ = ErrorCode::Ok;
        return Result::success();
    }

    const ErrorCode targetError = result.code();
    target->stop();
    return restore(previousProfile, *previousEndpoint, targetError);
}

Result BleModeCoordinator::restartActive() {
    if (transitioning() || state_ == BleModeCoordinatorState::Stopped ||
        state_ == BleModeCoordinatorState::Error ||
        activeEndpoint_ == nullptr) {
        return Result::failure(ErrorCode::StateConflict);
    }

    ITransportLifecycle* endpoint = activeEndpoint_;
    const ble::ModeProfile profile = activeProfile_;
    state_ = BleModeCoordinatorState::Switching;
    activeEndpoint_ = nullptr;
    activeProfile_ = ble::ModeProfile();
    endpoint->stop();

    Result result = link_.configureBleProfile(profile);
    if (result) result = prepareAndStart(profile, *endpoint, false);
    if (result) {
        activeEndpoint_ = endpoint;
        activeProfile_ = profile;
        state_ = activeState(profile.mode);
        lastTransitionError_ = ErrorCode::Ok;
        return Result::success();
    }

    endpoint->stop();
    state_ = BleModeCoordinatorState::Error;
    lastTransitionError_ = result.code();
    return result;
}

void BleModeCoordinator::stop() {
    if (activeEndpoint_ != nullptr) {
        activeEndpoint_->stop();
    } else {
        link_.setPacketReceiver(nullptr, nullptr);
        link_.setSessionHandlers(nullptr, nullptr, nullptr);
        link_.stop();
    }
    activeEndpoint_ = nullptr;
    activeProfile_ = ble::ModeProfile();
    state_ = BleModeCoordinatorState::Stopped;
    lastTransitionError_ = ErrorCode::Ok;
}

void BleModeCoordinator::poll(uint32_t budgetMicros) {
    if (activeEndpoint_ != nullptr && !transitioning()) {
        activeEndpoint_->poll(budgetMicros);
        // Provisioning endpoint errors are recoverable and are restarted by
        // the owning setup lifecycle after the active session closes. Direct
        // BBP/2 has no such transaction recovery path, so a physical link
        // failure must become an explicit coordinator fault instead of a
        // silent local-data outage.
        if (activeProfile_.mode == ble::ApplicationMode::Direct &&
            activeEndpoint_->state() == TransportState::Error) {
            state_ = BleModeCoordinatorState::Error;
            const ErrorCode error = link_.lastError();
            lastTransitionError_ = error == ErrorCode::Ok
                                       ? ErrorCode::InternalError
                                       : error;
        }
    }
}

ITransportLifecycle* BleModeCoordinator::endpointFor(
    ble::ApplicationMode mode) {
    if (mode == ble::ApplicationMode::Provisioning) {
        return &provisioning_;
    }
    if (mode == ble::ApplicationMode::Direct) return &direct_;
    return nullptr;
}

BleModeCoordinatorState BleModeCoordinator::activeState(
    ble::ApplicationMode mode) {
    return mode == ble::ApplicationMode::Provisioning
               ? BleModeCoordinatorState::Provisioning
               : BleModeCoordinatorState::Direct;
}

bool BleModeCoordinator::transitioning() const {
    return state_ == BleModeCoordinatorState::Starting ||
           state_ == BleModeCoordinatorState::Switching ||
           state_ == BleModeCoordinatorState::RollingBack;
}

Result BleModeCoordinator::startStopped(
    const ble::ModeProfile& profile) {
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    ITransportLifecycle* endpoint = endpointFor(profile.mode);
    if (endpoint == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    state_ = BleModeCoordinatorState::Starting;
    result = link_.configureBleProfile(profile);
    if (result) result = prepareAndStart(profile, *endpoint, false);
    if (!result) {
        endpoint->stop();
        activeEndpoint_ = nullptr;
        activeProfile_ = ble::ModeProfile();
        state_ = BleModeCoordinatorState::Error;
        lastTransitionError_ = result.code();
        return result;
    }
    activeEndpoint_ = endpoint;
    activeProfile_ = profile;
    state_ = activeState(profile.mode);
    lastTransitionError_ = ErrorCode::Ok;
    return Result::success();
}

Result BleModeCoordinator::prepareAndStart(
    const ble::ModeProfile& profile,
    ITransportLifecycle& endpoint,
    bool rollback) {
    if (preparation_ != nullptr) {
        Result result = preparation_->prepareBleMode(profile, rollback);
        if (!result) return result;
    }
    return endpoint.start();
}

Result BleModeCoordinator::restore(
    const ble::ModeProfile& profile,
    ITransportLifecycle& endpoint,
    ErrorCode targetError) {
    state_ = BleModeCoordinatorState::RollingBack;
    ++rollbackCount_;
    Result result = link_.configureBleProfile(profile);
    if (result) result = prepareAndStart(profile, endpoint, true);
    if (result) {
        activeEndpoint_ = &endpoint;
        activeProfile_ = profile;
        state_ = activeState(profile.mode);
        lastTransitionError_ = targetError;
        return Result::failure(targetError);
    }
    endpoint.stop();
    activeEndpoint_ = nullptr;
    activeProfile_ = ble::ModeProfile();
    state_ = BleModeCoordinatorState::Error;
    lastTransitionError_ = result.code();
    return result;
}

} // namespace blinker
