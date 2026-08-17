#ifndef BLINKER_RUNTIME_BLESETUPLIFECYCLE_H
#define BLINKER_RUNTIME_BLESETUPLIFECYCLE_H

#include "BleSetupCompletion.h"
#include "../core/SecureMemory.h"
#include "../interface/IRandom.h"
#include "../transport/BleModeCoordinator.h"

namespace blinker {

enum class BleSetupContract : uint8_t {
    PlatformEnrollment = 1U,
    LocalControllerPairing = 2U
};

struct BleSetupLifecycleConfig {
    BleSetupContract contract;
    bool acceptsWifiConfig;
    bool useProvisioningPsk;

    BleSetupLifecycleConfig()
        : contract(BleSetupContract::PlatformEnrollment),
          acceptsWifiConfig(false), useProvisioningPsk(false) {}
};

// Shared radio/mode state machine for BLE-only and WiFi+BLE products. It does
// not interpret Ticket, pairing or controller messages; it only observes the
// durable completion source after an application request finishes.
template <typename ProvisioningEndpoint>
class BasicBleSetupLifecycle {
public:
    BasicBleSetupLifecycle(
        BleModeCoordinator& ble,
        ProvisioningEndpoint& provisioning,
        IBleSetupCompletion& completion,
        IRandom& random,
        const BleSetupLifecycleConfig& config = BleSetupLifecycleConfig())
        : ble_(ble), provisioning_(provisioning), completion_(completion),
          random_(random), observedRequests_(0U), contract_(config.contract),
          acceptsWifiConfig_(config.acceptsWifiConfig),
          useProvisioningPsk_(config.useProvisioningPsk),
          setupComplete_(false), forcedProvisioning_(false),
          forcedSessionSeen_(false) {}

    Result start(bool allowDirect) {
        Result result = validateConfig();
        if (result) result = refresh();
        ble::ModeProfile profile;
        if (result && setupComplete_ && allowDirect) {
            profile = ble::makeDirectModeProfile();
        } else if (result) {
            result = makeProvisioningProfile(profile);
        }
        if (result) result = ble_.start(profile);
        if (!result) {
            ble_.stop();
            setupComplete_ = false;
            observedRequests_ = 0U;
            return result;
        }
        observedRequests_ = provisioning_.metrics().requests;
        return Result::success();
    }

    Result poll(bool allowDirect, uint32_t budgetMicros) {
        ble_.poll(budgetMicros);
        if (ble_.state() == BleModeCoordinatorState::Error) {
            const ErrorCode error = ble_.lastTransitionError();
            return Result::failure(
                error == ErrorCode::Ok ? ErrorCode::InternalError : error);
        }

        if (ble_.activeMode() == ble::ApplicationMode::Provisioning) {
            if (forcedProvisioning_ &&
                provisioning_.activeSessionId() != 0U) {
                forcedSessionSeen_ = true;
            }
            const uint32_t requests = provisioning_.metrics().requests;
            if (requests != observedRequests_) {
                observedRequests_ = requests;
                Result result = refresh();
                if (!result) return result;
            }

            if (forcedProvisioning_ && forcedSessionSeen_ &&
                provisioning_.activeSessionId() == 0U &&
                provisioning_.recoveryRequired()) {
                forcedProvisioning_ = false;
                forcedSessionSeen_ = false;
            }
            if (!forcedProvisioning_ && setupComplete_ && allowDirect &&
                provisioning_.activeSessionId() == 0U) {
                return ble_.switchTo(ble::makeDirectModeProfile());
            }
            if (provisioning_.recoveryRequired()) {
                return ble_.restartActive();
            }
        }
        return Result::success();
    }

    // Product-owned recovery entry for LocalBLE. This changes only the radio
    // application mode; it does not open a pairing window. After Noise is
    // ready, a separate physical event must still confirm the exact session.
    Result openLocalPairingMode() {
        if (contract_ !=
            BleSetupContract::LocalControllerPairing) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        if (ble_.activeMode() == ble::ApplicationMode::Provisioning) {
            return Result::success();
        }
        ble::ModeProfile profile;
        Result result = makeProvisioningProfile(profile);
        if (result) result = ble_.switchTo(profile);
        if (result) {
            forcedProvisioning_ = true;
            forcedSessionSeen_ = false;
            observedRequests_ = provisioning_.metrics().requests;
        }
        return result;
    }

    Result cancelLocalPairingMode() {
        if (!forcedProvisioning_) return Result::success();
        if (!setupComplete_) {
            return Result::failure(ErrorCode::StateConflict);
        }
        Result result = ble_.switchTo(ble::makeDirectModeProfile());
        if (result) {
            forcedProvisioning_ = false;
            forcedSessionSeen_ = false;
        }
        return result;
    }

    void stop() {
        ble_.stop();
        observedRequests_ = 0U;
        setupComplete_ = false;
        forcedProvisioning_ = false;
        forcedSessionSeen_ = false;
    }

    bool setupComplete() const { return setupComplete_; }
    ble::ApplicationMode activeMode() const { return ble_.activeMode(); }

private:
    Result validateConfig() const {
        if (contract_ == BleSetupContract::PlatformEnrollment) {
            return Result::success();
        }
        return contract_ ==
                       BleSetupContract::LocalControllerPairing &&
                   !acceptsWifiConfig_
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }

    Result refresh() {
        bool complete = false;
        Result result = completion_.load(complete);
        if (result) setupComplete_ = complete;
        return result;
    }

    Result makeProvisioningProfile(ble::ModeProfile& profile) {
        uint8_t locator[kSetupSessionLocatorSize] = {};
        Result result = random_.fill(
            MutableByteSpan(locator, sizeof(locator)));
        uint8_t combined = 0U;
        for (size_t index = 0U; index < sizeof(locator); ++index) {
            combined = static_cast<uint8_t>(combined | locator[index]);
        }
        if (result && combined == 0U) {
            result = Result::failure(ErrorCode::InternalError);
        }
        if (result) {
            const ByteView value(locator, sizeof(locator));
            if (contract_ ==
                BleSetupContract::LocalControllerPairing) {
                profile = useProvisioningPsk_
                              ? ble::makePskLocalControllerPairingModeProfile(
                                    value)
                              : ble::makeLocalControllerPairingModeProfile(
                                    value);
            } else {
                profile = useProvisioningPsk_
                              ? ble::makePskProvisioningModeProfile(
                                    value, acceptsWifiConfig_)
                              : ble::makeProvisioningModeProfile(
                                    value, acceptsWifiConfig_);
            }
            result = ble::validateModeProfile(profile);
        }
        secureZero(MutableByteSpan(locator, sizeof(locator)));
        return result;
    }

    BleModeCoordinator& ble_;
    ProvisioningEndpoint& provisioning_;
    IBleSetupCompletion& completion_;
    IRandom& random_;
    uint32_t observedRequests_;
    BleSetupContract contract_;
    uint8_t acceptsWifiConfig_ : 1;
    uint8_t useProvisioningPsk_ : 1;
    uint8_t setupComplete_ : 1;
    uint8_t forcedProvisioning_ : 1;
    uint8_t forcedSessionSeen_ : 1;
};

} // namespace blinker

#endif
