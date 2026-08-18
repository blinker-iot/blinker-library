#ifndef BLINKER_V2_ARDUINO_BLE_PRODUCT_H
#define BLINKER_V2_ARDUINO_BLE_PRODUCT_H

#include "ArduinoRuntime.h"

#include <BlinkerHardwareRandomAdapter.h>
#include <BlinkerV2/core/ResourceProfile.h>
#include <BlinkerV2/identity/ControllerCredentialStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/identity/OwnershipRecordStore.h>
#include <BlinkerV2/interface/IProductLifecycle.h>
#include <BlinkerV2/provisioning/BleNoiseProvisioningChannel.h>
#include <BlinkerV2/provisioning/BleLocalProvisioningEndpoint.h>
#include <BlinkerV2/provisioning/BleNoiseModePreparation.h>
#include <BlinkerV2/provisioning/EnrollmentTicketVerifier.h>
#include <BlinkerV2/provisioning/LocalControllerPairingApplication.h>
#include <BlinkerV2/provisioning/LocalControllerPairingCoordinator.h>
#include <BlinkerV2/provisioning/LocalSetupApplication.h>
#include <BlinkerV2/provisioning/LocalSetupSaga.h>
#include <BlinkerV2/provisioning/OwnershipClaimCoordinator.h>
#include <BlinkerV2/provisioning/OwnershipClaimRecordStore.h>
#include <BlinkerV2/runtime/BleOnlyLifecycle.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>
#include <BlinkerV2/transport/BleFrameTransport.h>
#include <BlinkerV2/transport/BleModeCoordinator.h>

namespace blinker {
namespace integration {
namespace official_detail {

template <
    typename Platform,
    size_t OperationWorkspaceSize,
    size_t ResponseWorkspaceSize>
class BleWorkspace {
public:
    enum : size_t {
        directRxSize =
            BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_SESSIONS,
        directTxSize =
            BLINKER_DEVICE_FRAME_SIZE * BLINKER_BLE_MAX_TX_FRAMES,
        sharedRxSize = directRxSize > kBleNoiseRecordSize
                           ? directRxSize
                           : kBleNoiseRecordSize,
        sharedTxSize = directTxSize > kBleNoiseRecordSize
                           ? directTxSize
                           : kBleNoiseRecordSize,
        packetSize = Platform::maximumBlePacketSize
    };

    MutableByteSpan noiseRx() {
        return MutableByteSpan(recordRx_, kBleNoiseRecordSize);
    }
    MutableByteSpan noiseTx() {
        return MutableByteSpan(recordTx_, kBleNoiseRecordSize);
    }
    MutableByteSpan directRx() {
        return MutableByteSpan(recordRx_, sizeof(recordRx_));
    }
    MutableByteSpan directTx() {
        return MutableByteSpan(recordTx_, sizeof(recordTx_));
    }
    MutableByteSpan packet() {
        return MutableByteSpan(packet_, sizeof(packet_));
    }
    MutableByteSpan plaintext() {
        return MutableByteSpan(plaintext_, sizeof(plaintext_));
    }
    MutableByteSpan operation() {
        return MutableByteSpan(operation_, sizeof(operation_));
    }
    MutableByteSpan response() {
        return MutableByteSpan(response_, sizeof(response_));
    }

private:
    uint8_t recordRx_[sharedRxSize];
    uint8_t recordTx_[sharedTxSize];
    uint8_t packet_[packetSize];
    uint8_t plaintext_[kBleNoiseMaxTransportPayloadSize];
    uint8_t operation_[OperationWorkspaceSize];
    uint8_t response_[ResponseWorkspaceSize];
};

inline BleFrameTransportConfig directBleTransportConfig() {
    BleFrameTransportConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

class ILocalBleComposition : public IProductLifecycle {
public:
    virtual Result openPairingMode() = 0;
    virtual Result cancelPairingMode() = 0;
    virtual Result confirmPhysicalPresence() = 0;
};

ILocalBleComposition& esp32LocalBleLifecycle();
ILocalBleComposition& renesasUnoLocalBleLifecycle();

inline BleSetupLifecycleConfig platformBleSetupConfig() {
    BleSetupLifecycleConfig config;
    config.contract = BleSetupContract::PlatformEnrollment;
    return config;
}

inline BleSetupLifecycleConfig localBleSetupConfig() {
    BleSetupLifecycleConfig config;
    config.contract = BleSetupContract::LocalControllerPairing;
    return config;
}

// Shared BLE record/radio/direct-data graph. Authorization and durable setup
// state are injected by the product composition and never interpreted here.
template <
    typename Platform,
    typename Application,
    size_t OperationWorkspaceSize,
    size_t ResponseWorkspaceSize>
class BleOnlyRadio {
public:
    BleOnlyRadio(
        Platform& platform,
        ArduinoClock& clock,
        PlatformHardwareRandom& random,
        Application& application,
        ControllerCredentialStore& controllers,
        IBleSetupCompletion& completion,
        const BleSetupLifecycleConfig& config)
        : workspace_(),
          noiseChannel_(
              platform.bleLink(),
              clock,
              platform.noiseCrypto(),
              random,
              workspace_.noiseRx(),
              workspace_.noiseTx(),
              workspace_.packet(),
              workspace_.plaintext(),
              BleNoiseProvisioningConfig()),
          provisioning_(
              noiseChannel_,
              application,
              workspace_.operation(),
              workspace_.response()),
          modePreparation_(noiseChannel_, application),
          direct_(
              platform.bleLink(),
              clock,
              workspace_.directRx(),
              workspace_.directTx(),
              workspace_.packet(),
              directBleTransportConfig()),
          ble_(
              platform.bleLink(),
              provisioning_,
              direct_,
              &modePreparation_),
          authSessions_(),
          directAuthorization_(
              controllers,
              random,
              authSessions_,
              BLINKER_BLE_MAX_SESSIONS),
          lifecycle_(
              ble_,
              provisioning_,
              completion,
              direct_,
              directAuthorization_,
              random,
              config) {}

    Result attach(Client& client) { return lifecycle_.attach(client); }
    Result start() { return lifecycle_.start(); }
    void poll(uint32_t budgetMicros) { lifecycle_.poll(budgetMicros); }
    void stop() { lifecycle_.stop(); }
    ProductLifecycleStatus status() const { return lifecycle_.status(); }
    ProductCapabilities capabilities() const {
        return lifecycle_.capabilities();
    }
    Result openPairingMode() {
        return lifecycle_.openLocalPairingMode();
    }
    Result cancelPairingMode() {
        return lifecycle_.cancelLocalPairingMode();
    }

private:
    BleWorkspace<
        Platform,
        OperationWorkspaceSize,
        ResponseWorkspaceSize>
        workspace_;
    BleNoiseProvisioningChannel noiseChannel_;
    BleLocalProvisioningEndpoint provisioning_;
    BleNoiseModePreparation modePreparation_;
    BleFrameTransport direct_;
    BleModeCoordinator ble_;
    security::ControllerAuthSession
        authSessions_[BLINKER_BLE_MAX_SESSIONS];
    security::ControllerHmacSha256Authorizer directAuthorization_;
    BleOnlyLifecycle lifecycle_;
};

template <typename Platform>
class PlatformBleOnlyComposition final : public IProductLifecycle {
public:
    PlatformBleOnlyComposition()
        : platform_(),
          clock_(),
          random_(),
          deviceInstance_(),
          deviceInstanceStore_(platform_.deviceInstanceBlob()),
          claimJournal_(platform_.ownershipClaimBlob()),
          claimVerifier_(platform_.serverSignatureVerifier()),
          claim_(
              deviceInstance_,
              platform_.ownershipStore(),
              claimJournal_,
              random_,
              claimVerifier_),
          controllers_(platform_.controllerCredentialBlob()),
          setupJournal_(platform_.localSetupSagaBlob()),
          setupCompletion_(setupJournal_),
          setupSaga_(
              deviceInstance_,
              platform_.ownershipStore(),
              claim_,
              controllers_,
              setupJournal_,
              clock_),
          ticketVerifier_(platform_.serverSignatureVerifier()),
          setupApplication_(
              deviceInstance_,
              random_,
              clock_,
              ticketVerifier_,
              setupSaga_,
              0U),
          radio_(
              platform_,
              clock_,
              random_,
              setupApplication_,
              controllers_,
              setupCompletion_,
              platformBleSetupConfig()),
          initialized_(false),
          configurationError_(ErrorCode::Ok) {}

    ~PlatformBleOnlyComposition() override {
        radio_.stop();
        platform_.end();
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = radio_.attach(client);
        if (!result) configurationError_ = result.code();
        return result;
    }

    Result start() override {
        const Result result = initialized_
                                  ? radio_.start()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        if (!result) configurationError_ = result.code();
        return result;
    }

    void poll(uint32_t budgetMicros) override {
        radio_.poll(budgetMicros);
    }
    void stop() override { radio_.stop(); }
    ProductLifecycleStatus status() const override {
        ProductLifecycleStatus current = radio_.status();
        if (configurationError_ != ErrorCode::Ok &&
            current.state == ProductLifecycleState::Stopped) {
            current.state = ProductLifecycleState::Fault;
            current.lastError = configurationError_;
        }
        return current;
    }
    ProductCapabilities capabilities() const override {
        return radio_.capabilities();
    }

private:
    Result initialize() {
        if (initialized_) return Result::success();
        if (configurationError_ != ErrorCode::Ok) {
            return Result::failure(configurationError_);
        }
        Result result = platform_.begin();
        if (result) {
            result = deviceInstanceStore_.loadOrCreate(
                random_, deviceInstance_);
        }
        if (!result) {
            platform_.end();
            configurationError_ = result.code();
            return result;
        }
        initialized_ = true;
        return Result::success();
    }

    Platform platform_;
    ArduinoClock clock_;
    PlatformHardwareRandom random_;
    DeviceInstanceId deviceInstance_;
    DeviceInstanceIdStore deviceInstanceStore_;
    OwnershipClaimRecordStore claimJournal_;
    OwnershipClaimGrantVerifier claimVerifier_;
    OwnershipClaimCoordinator claim_;
    ControllerCredentialStore controllers_;
    LocalSetupSagaStore setupJournal_;
    LocalSetupSagaCompletion setupCompletion_;
    LocalSetupSaga setupSaga_;
    EnrollmentTicketVerifier ticketVerifier_;
    LocalSetupApplication setupApplication_;
    BleOnlyRadio<
        Platform,
        LocalSetupApplication,
        kLocalSetupOperationWorkspaceSize,
        kLocalSetupResponseMaxEncodedSize>
        radio_;
    bool initialized_;
    ErrorCode configurationError_;
};

template <typename Platform>
class LocalBleComposition final : public ILocalBleComposition {
public:
    LocalBleComposition()
        : platform_(),
          clock_(),
          random_(),
          deviceInstance_(),
          deviceInstanceStore_(platform_.deviceInstanceBlob()),
          controllers_(platform_.controllerCredentialBlob()),
          setupCompletion_(controllers_),
          pairing_(
              deviceInstance_,
              controllers_,
              random_,
              clock_),
          application_(deviceInstance_, pairing_),
          radio_(
              platform_,
              clock_,
              random_,
              application_,
              controllers_,
              setupCompletion_,
              localBleSetupConfig()),
          initialized_(false),
          configurationError_(ErrorCode::Ok) {}

    ~LocalBleComposition() override {
        radio_.stop();
        platform_.end();
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = radio_.attach(client);
        if (!result) configurationError_ = result.code();
        return result;
    }

    Result start() override {
        const Result result = initialized_
                                  ? radio_.start()
                                  : Result::failure(
                                        ErrorCode::NotConfigured);
        if (!result) configurationError_ = result.code();
        return result;
    }

    void poll(uint32_t budgetMicros) override {
        radio_.poll(budgetMicros);
    }
    void stop() override { radio_.stop(); }
    ProductLifecycleStatus status() const override {
        ProductLifecycleStatus current = radio_.status();
        if (configurationError_ != ErrorCode::Ok &&
            current.state == ProductLifecycleState::Stopped) {
            current.state = ProductLifecycleState::Fault;
            current.lastError = configurationError_;
        }
        return current;
    }
    ProductCapabilities capabilities() const override {
        return radio_.capabilities();
    }
    Result openPairingMode() override {
        return radio_.openPairingMode();
    }
    Result cancelPairingMode() override {
        return radio_.cancelPairingMode();
    }
    Result confirmPhysicalPresence() override {
        return application_.confirmPhysicalPresence();
    }

private:
    Result initialize() {
        if (initialized_) return Result::success();
        if (configurationError_ != ErrorCode::Ok) {
            return Result::failure(configurationError_);
        }
        Result result = platform_.begin();
        if (result) {
            result = deviceInstanceStore_.loadOrCreate(
                random_, deviceInstance_);
        }
        if (!result) {
            platform_.end();
            configurationError_ = result.code();
            return result;
        }
        initialized_ = true;
        return Result::success();
    }

    Platform platform_;
    ArduinoClock clock_;
    PlatformHardwareRandom random_;
    DeviceInstanceId deviceInstance_;
    DeviceInstanceIdStore deviceInstanceStore_;
    ControllerCredentialStore controllers_;
    LocalControllerCredentialCompletion setupCompletion_;
    LocalControllerPairingCoordinator pairing_;
    LocalControllerPairingApplication application_;
    BleOnlyRadio<
        Platform,
        LocalControllerPairingApplication,
        kLocalControllerPairingApplicationWorkspaceSize,
        kLocalControllerPairingApplicationResponseMaxEncodedSize>
        radio_;
    bool initialized_;
    ErrorCode configurationError_;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
