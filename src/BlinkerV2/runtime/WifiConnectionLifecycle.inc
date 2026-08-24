#include "WifiConnectionLifecycle.h"

namespace blinker {

namespace {

bool timeReached(uint32_t now, uint32_t target) {
    return static_cast<int32_t>(now - target) >= 0;
}

} // namespace

WifiConnectionLifecycle::WifiConnectionLifecycle(
    WifiCredentialStore& credentials,
    IWifiStation& station,
    IClock& clock,
    WifiCredentialProfile& profileWorkspace,
    const WifiConnectionLifecycleConfig& config)
    : credentials_(credentials),
      station_(station),
      clock_(clock),
      profile_(profileWorkspace),
      config_(config),
      loadedGeneration_(0U),
      connectionStartedAt_(0U),
      retryAt_(0U),
      state_(WifiConnectionState::Stopped),
      lastError_(ErrorCode::Ok) {
    clearWifiCredentialProfile(profile_);
}

WifiConnectionLifecycle::~WifiConnectionLifecycle() {
    stop();
}

void WifiConnectionLifecycle::clearProfile() {
    clearWifiCredentialProfile(profile_);
    loadedGeneration_ = 0U;
}

Result WifiConnectionLifecycle::start() {
    if (state_ != WifiConnectionState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (config_.connectionTimeoutMs == 0U ||
        config_.reconnectBackoffMs == 0U ||
        config_.persistenceRetryMs == 0U) {
        enterFault(ErrorCode::InvalidArgument);
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = credentials_.ready()
                        ? Result::success()
                        : credentials_.begin();
    if (!result) {
        enterFault(result.code());
        return result;
    }
    lastError_ = ErrorCode::Ok;
    return loadAndConnect();
}

Result WifiConnectionLifecycle::reload() {
    if (state_ == WifiConnectionState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    station_.stop();
    clearProfile();
    lastError_ = ErrorCode::Ok;
    return loadAndConnect();
}

Result WifiConnectionLifecycle::loadAndConnect() {
    if (!credentials_.hasActiveProfile()) {
        clearProfile();
        state_ = WifiConnectionState::WaitingForCredentials;
        return Result::success();
    }

    Result result = credentials_.load(profile_);
    if (!result) {
        clearProfile();
        enterFault(result.code());
        return result;
    }
    loadedGeneration_ = profile_.generation;
    return startStation();
}

Result WifiConnectionLifecycle::startStation() {
    const Result result = station_.start(profile_.networkConfig());
    if (!result) return handleConnectionFailure(result.code());

    connectionStartedAt_ = clock_.monotonicMillis();
    state_ = profile_.confirmed()
                 ? WifiConnectionState::ConnectingConfirmed
                 : WifiConnectionState::ConnectingCandidate;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

Result WifiConnectionLifecycle::retryConfirmedLater(ErrorCode error) {
    station_.stop();
    lastError_ = error;
    retryAt_ = clock_.monotonicMillis() + config_.reconnectBackoffMs;
    state_ = WifiConnectionState::RetryBackoff;
    return Result::failure(error);
}

Result WifiConnectionLifecycle::handleConnectionFailure(ErrorCode error) {
    station_.stop();
    lastError_ = error;
    if (profile_.confirmed()) return retryConfirmedLater(error);

    Result result = credentials_.rollbackToPrevious();
    if (!result) {
        const uint32_t rejectedGeneration = loadedGeneration_;
        clearProfile();
        if (result.code() == ErrorCode::NotFound ||
            result.code() == ErrorCode::StateConflict) {
            // Remember which candidate was rejected without retaining its
            // secret. Only a new durable generation (or explicit reload())
            // may cause another attempt.
            loadedGeneration_ = rejectedGeneration;
            state_ = WifiConnectionState::CandidateRejected;
            return Result::failure(error);
        }
        enterFault(result.code());
        return result;
    }

    clearProfile();
    result = credentials_.load(profile_);
    if (!result || !profile_.confirmed()) {
        const ErrorCode failure = result ? ErrorCode::StateConflict
                                         : result.code();
        clearProfile();
        enterFault(failure);
        return Result::failure(failure);
    }
    loadedGeneration_ = profile_.generation;
    return startStation();
}

Result WifiConnectionLifecycle::confirmCandidate() {
    const Result result = credentials_.confirmActive();
    if (result) {
        profile_.setConfirmed(true);
        lastError_ = ErrorCode::Ok;
        state_ = WifiConnectionState::Online;
        return result;
    }
    lastError_ = result.code();
    retryAt_ = clock_.monotonicMillis() + config_.persistenceRetryMs;
    state_ = WifiConnectionState::ConfirmingCandidate;
    return result;
}

bool WifiConnectionLifecycle::connectionTimedOut(uint32_t now) const {
    return static_cast<uint32_t>(now - connectionStartedAt_) >=
           config_.connectionTimeoutMs;
}

bool WifiConnectionLifecycle::retryDue(uint32_t now) const {
    return timeReached(now, retryAt_);
}

void WifiConnectionLifecycle::enterFault(ErrorCode error) {
    station_.stop();
    lastError_ = error;
    state_ = WifiConnectionState::Fault;
}

void WifiConnectionLifecycle::poll() {
    if (state_ == WifiConnectionState::Stopped ||
        state_ == WifiConnectionState::Fault) {
        return;
    }

    const uint32_t storedGeneration = credentials_.activeGeneration();
    if (storedGeneration != loadedGeneration_) {
        (void)reload();
        return;
    }
    if (state_ == WifiConnectionState::WaitingForCredentials ||
        state_ == WifiConnectionState::CandidateRejected) {
        return;
    }

    const uint32_t now = clock_.monotonicMillis();

    if (state_ == WifiConnectionState::RetryBackoff) {
        if (retryDue(now)) (void)startStation();
        return;
    }

    station_.poll();
    const WifiStationState stationState = station_.state();

    if (state_ == WifiConnectionState::ConfirmingCandidate) {
        if (stationState != WifiStationState::Connected) {
            const ErrorCode error = stationState == WifiStationState::Failed
                                        ? station_.lastError()
                                        : ErrorCode::NotConnected;
            (void)handleConnectionFailure(error);
        } else if (retryDue(now)) {
            (void)confirmCandidate();
        }
        return;
    }

    if (state_ == WifiConnectionState::Online) {
        if (stationState != WifiStationState::Connected) {
            const ErrorCode error = stationState == WifiStationState::Failed
                                        ? station_.lastError()
                                        : ErrorCode::NotConnected;
            (void)retryConfirmedLater(error);
        }
        return;
    }

    if (stationState == WifiStationState::Connected) {
        if (profile_.confirmed()) {
            lastError_ = ErrorCode::Ok;
            state_ = WifiConnectionState::Online;
        } else {
            (void)confirmCandidate();
        }
        return;
    }

    if (stationState == WifiStationState::Failed ||
        connectionTimedOut(now)) {
        const ErrorCode error = stationState == WifiStationState::Failed
                                    ? station_.lastError()
                                    : ErrorCode::NotConnected;
        (void)handleConnectionFailure(error);
    }
}

void WifiConnectionLifecycle::stop() {
    if (state_ == WifiConnectionState::Stopped) return;
    station_.stop();
    clearProfile();
    connectionStartedAt_ = 0U;
    retryAt_ = 0U;
    lastError_ = ErrorCode::Ok;
    state_ = WifiConnectionState::Stopped;
}

} // namespace blinker
