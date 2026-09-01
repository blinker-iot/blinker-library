#ifndef BLINKER_RUNTIME_BLESETUPLIFECYCLE_H
#define BLINKER_RUNTIME_BLESETUPLIFECYCLE_H

#include "BleSetupCompletion.h"
#include "BleDirectProfile.h"
#include "../core/SecureMemory.h"
#include "../interface/IRandom.h"
#include "../transport/BleModeCoordinator.h"

namespace blinker {

struct BleSetupLifecycleConfig {
    bool acceptsWifiConfig;
    bool useProvisioningPsk;

    BleSetupLifecycleConfig()
        : acceptsWifiConfig(false), useProvisioningPsk(false) {}
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
        const BleSetupLifecycleConfig& config = BleSetupLifecycleConfig(),
        BleDirectProfileProvider* directProfiles = nullptr)
        : ble_(ble), provisioning_(provisioning), completion_(completion),
          random_(random), observedRequests_(0U),
          acceptsWifiConfig_(config.acceptsWifiConfig),
          useProvisioningPsk_(config.useProvisioningPsk),
          setupComplete_(false), directProfiles_(directProfiles) {}

    Result start(bool allowDirect, uint32_t directSessionRevision = 0U) {
        Result result = refresh();
        ble::ModeProfile profile;
        if (result && setupComplete_ && allowDirect) {
            result = makeDirectProfile(directSessionRevision, profile);
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

    Result poll(
        bool allowDirect,
        uint32_t budgetMicros,
        uint32_t directSessionRevision = 0U) {
        ble_.poll(budgetMicros);
        if (ble_.state() == BleModeCoordinatorState::Error) {
            const ErrorCode error = ble_.lastTransitionError();
            return Result::failure(
                error == ErrorCode::Ok ? ErrorCode::InternalError : error);
        }

        if (ble_.activeMode() == ble::ApplicationMode::Provisioning) {
            const uint32_t requests = provisioning_.metrics().requests;
            if (requests != observedRequests_) {
                observedRequests_ = requests;
                Result result = refresh();
                if (!result) return result;
            }

            if (setupComplete_ && allowDirect &&
                provisioning_.activeSessionId() == 0U) {
                ble::ModeProfile profile;
                Result result = makeDirectProfile(
                    directSessionRevision, profile);
                return result ? ble_.switchTo(profile) : result;
            }
            if (provisioning_.recoveryRequired()) {
                return ble_.restartActive();
            }
        }
        if (ble_.activeMode() == ble::ApplicationMode::Direct &&
            directProfiles_ != nullptr &&
            directProfiles_->refreshDue(
                ble_.sessionCount(), directSessionRevision)) {
            ble::ModeProfile profile;
            Result result = directProfiles_->make(
                directSessionRevision, profile);
            return result ? ble_.switchTo(profile) : result;
        }
        return Result::success();
    }

    void stop() {
        ble_.stop();
        observedRequests_ = 0U;
        setupComplete_ = false;
        if (directProfiles_ != nullptr) directProfiles_->reset();
    }

    bool setupComplete() const { return setupComplete_; }
    ble::ApplicationMode activeMode() const { return ble_.activeMode(); }

private:
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
            profile = useProvisioningPsk_
                          ? ble::makePskProvisioningModeProfile(
                                value, acceptsWifiConfig_)
                          : ble::makeProvisioningModeProfile(
                                value, acceptsWifiConfig_);
            result = ble::validateModeProfile(profile);
        }
        secureZero(MutableByteSpan(locator, sizeof(locator)));
        return result;
    }

    Result makeDirectProfile(
        uint32_t sessionRevision,
        ble::ModeProfile& profile) {
        if (directProfiles_ == nullptr) {
            profile = ble::makeDirectModeProfile();
            return Result::success();
        }
        return directProfiles_->make(sessionRevision, profile);
    }

    BleModeCoordinator& ble_;
    ProvisioningEndpoint& provisioning_;
    IBleSetupCompletion& completion_;
    IRandom& random_;
    uint32_t observedRequests_;
    uint8_t acceptsWifiConfig_ : 1;
    uint8_t useProvisioningPsk_ : 1;
    uint8_t setupComplete_ : 1;
    BleDirectProfileProvider* directProfiles_;
};

} // namespace blinker

#endif
