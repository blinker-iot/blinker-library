#ifndef BLINKER_OFFICIAL_WIFI_BLE_COMPOSITION_H
#define BLINKER_OFFICIAL_WIFI_BLE_COMPOSITION_H

#include "BlinkerOfficialBleProduct.h"
#include "BlinkerOfficialWifiCloudStack.h"

#include <BlinkerV2/identity/ControllerCredentialStore.h>
#include <BlinkerV2/provisioning/BleLocalProvisioningEndpoint.h>
#include <BlinkerV2/provisioning/BleNoiseModePreparation.h>
#include <BlinkerV2/provisioning/EnrollmentTicketVerifier.h>
#include <BlinkerV2/provisioning/LocalSetupApplication.h>
#include <BlinkerV2/provisioning/LocalSetupSaga.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>
#include <BlinkerV2/transport/BleFrameTransport.h>
#include <BlinkerV2/transport/BleModeCoordinator.h>

namespace blinker {
namespace integration {
namespace official_detail {

// Internal ESP32 WiFi+BLE product graph. The public WiFiBLE facade remains
// deliberately unbound until R4b-7; this class is exercised through an
// official-package compile probe first.
template <typename Platform>
class WifiBleComposition final : public IProductLifecycle {
public:
    WifiBleComposition()
        : stack_(),
          controllers_(
              stack_.platform().controllerCredentialBlob()),
          setupJournal_(stack_.platform().localSetupSagaBlob()),
          setupCompletion_(setupJournal_),
          setupSaga_(
              stack_.deviceInstance(),
              stack_.platform().ownershipStore(),
              stack_.claim(),
              controllers_,
              setupJournal_,
              stack_.clock(),
              &stack_.platform().wifiCredentials(),
              &stack_.cloudEnrollment()),
          ticketVerifier_(stack_.platform().serverSignatureVerifier()),
          setupApplication_(
              stack_.deviceInstance(),
              stack_.random(),
              stack_.clock(),
              ticketVerifier_,
              setupSaga_,
              kEnrollmentTicketAuthorizeAll),
          workspace_(),
          noiseChannel_(
              stack_.platform().bleLink(),
              stack_.clock(),
              stack_.platform().noiseCrypto(),
              stack_.random(),
              workspace_.noiseRx(),
              workspace_.noiseTx(),
              workspace_.packet(),
              workspace_.plaintext(),
              BleNoiseProvisioningConfig()),
          provisioning_(
              noiseChannel_,
              setupApplication_,
              workspace_.operation(),
              workspace_.response()),
          modePreparation_(noiseChannel_, setupApplication_),
          direct_(
              stack_.platform().bleLink(),
              stack_.clock(),
              workspace_.directRx(),
              workspace_.directTx(),
              workspace_.packet(),
              directBleTransportConfig()),
          ble_(
              stack_.platform().bleLink(),
              provisioning_,
              direct_,
              &modePreparation_),
          authSessions_(),
          directAuthorization_(
              controllers_,
              stack_.random(),
              authSessions_,
              BLINKER_BLE_MAX_SESSIONS),
          lifecycle_(
              stack_.wifiLifecycle(),
              stack_.cloudTransport(),
              ble_,
              provisioning_,
              setupCompletion_,
              direct_,
              directAuthorization_,
              stack_.random()),
          configurationError_(ErrorCode::Ok) {}

    ~WifiBleComposition() override {
        lifecycle_.stop();
        stack_.end();
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = lifecycle_.attach(client);
        if (!result) configurationError_ = result.code();
        return result;
    }

    Result start() override {
        const Result result = stack_.initialized()
                                  ? lifecycle_.start()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        if (!result) configurationError_ = result.code();
        return result;
    }

    void poll(uint32_t totalBudgetMicros) override {
        lifecycle_.poll(totalBudgetMicros);
    }

    void stop() override { lifecycle_.stop(); }

    ProductLifecycleStatus status() const override {
        ProductLifecycleStatus current = lifecycle_.status();
        if (configurationError_ != ErrorCode::Ok &&
            current.state == ProductLifecycleState::Stopped) {
            current.state = ProductLifecycleState::Fault;
            current.lastError = configurationError_;
        }
        return current;
    }

    ProductCapabilities capabilities() const override {
        return lifecycle_.capabilities();
    }

private:
    Result initialize() {
        if (stack_.initialized()) return Result::success();
        if (configurationError_ != ErrorCode::Ok) {
            return Result::failure(configurationError_);
        }
        const Result result = stack_.begin();
        if (!result) {
            stack_.end();
            configurationError_ = result.code();
        }
        return result;
    }

    WifiCloudStack<Platform> stack_;
    ControllerCredentialStore controllers_;
    LocalSetupSagaStore setupJournal_;
    LocalSetupSagaCompletion setupCompletion_;
    LocalSetupSaga setupSaga_;
    EnrollmentTicketVerifier ticketVerifier_;
    LocalSetupApplication setupApplication_;

    BleWorkspace<
        Platform,
        kLocalSetupOperationWorkspaceSize,
        kLocalSetupResponseMaxEncodedSize>
        workspace_;
    BleNoiseProvisioningChannel noiseChannel_;
    BleLocalProvisioningEndpoint provisioning_;
    BleNoiseModePreparation modePreparation_;
    BleFrameTransport direct_;
    BleModeCoordinator ble_;
    security::ControllerAuthSession
        authSessions_[BLINKER_BLE_MAX_SESSIONS];
    security::ControllerHmacSha256Authorizer directAuthorization_;
    WifiBleLifecycle lifecycle_;
    ErrorCode configurationError_;

    WifiBleComposition(const WifiBleComposition&);
    WifiBleComposition& operator=(const WifiBleComposition&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
