#ifndef BLINKER_OFFICIAL_WIFI_CLOUD_STACK_H
#define BLINKER_OFFICIAL_WIFI_CLOUD_STACK_H

#include "BlinkerOfficialArduinoRuntime.h"

#include <BlinkerV2Advanced.h>
#include <BlinkerArduinoClientHttpAdapter.h>
#include <BlinkerHardwareRandomAdapter.h>
#include <BlinkerOfficialBuildConfig.h>
#include <BlinkerPubSubClientAdapter.h>

#include <PubSubClient.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline HttpCloudSessionConfig cloudSessionConfig() {
    HttpCloudSessionConfig config;
    config.host = StringView(official::controlHost);
    config.port = official::controlPort;
    config.path = StringView("/api/v2/device-sessions");
    config.firmwareVersion = StringView(official::firmwareVersion);
    config.httpSecurity = official::cloudSessionTls
                              ? HttpSecurity::Tls
                              : HttpSecurity::PlainTcp;
    config.mqttSecurity = official::mqttTls
                              ? MqttSecurity::Tls
                              : MqttSecurity::PlainTcp;
    return config;
}

inline MqttFrameTransportConfig mqttTransportConfig() {
    MqttFrameTransportConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

// Shared lower half of the official WiFi and WiFi+BLE products. It owns one
// platform, identity/claim/cloud coordinators, WiFi connection lifecycle and
// cloud session/MQTT transport. Product-specific onboarding stays above this
// class, so adding another radio never creates a second Client or cloud stack.
template <typename Platform>
class WifiCloudStack {
public:
    typedef typename Platform::CloudSessionClient CloudSessionClient;
    typedef typename Platform::MqttClient MqttClient;
    typedef ArduinoClientHttpAdapter<CloudSessionClient, 97U, 384U, 128U>
        ControlHttp;

    WifiCloudStack()
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
          cloudEnrollmentJournal_(platform_.cloudEnrollmentBlob()),
          encryptionKeys_(
              official::activeCloudEncryptionKeyId,
              official::cloudEncryptionKeys,
              official::cloudEncryptionKeysCount),
          envelopeSealer_(platform_.enrollmentCrypto(), encryptionKeys_),
          cloudGrantVerifier_(platform_.serverSignatureVerifier()),
          cloudEnrollment_(
              deviceInstance_,
              platform_.ownership(),
              platform_.cloudCredentialStore(),
              cloudEnrollmentJournal_,
              random_,
              envelopeSealer_,
              cloudGrantVerifier_),
          wifiProfile_(),
          wifiLifecycle_(
              platform_.wifiCredentials(),
              platform_.wifiStation(),
              clock_,
              wifiProfile_),
          arena_(),
          controlHttp_(
              platform_.controlNetwork(),
              official::cloudSessionTls
                  ? HttpSecurity::Tls
                  : HttpSecurity::PlainTcp,
              clock_),
          cloudSession_(
              platform_.cloudCredentials(),
              random_,
              controlHttp_,
              clock_,
              cloudSessionConfig(),
              arena_.cloudRequest(),
              arena_.cloudResponse(),
              arena_.cloudCredentials()),
          nativeMqtt_(platform_.mqttNetwork()),
          mqttAdapter_(
              nativeMqtt_,
              official::mqttTls
                  ? MqttSecurity::Tls
                  : MqttSecurity::PlainTcp,
              kMqttPacketBufferSize,
              1U),
          mqttTransport_(mqttAdapter_, clock_, mqttTransportConfig()),
          cloudTransport_(cloudSession_, mqttTransport_),
          initialized_(false) {}

    ~WifiCloudStack() { end(); }

    Result begin() {
        if (initialized_) return Result::success();
        platform_.configureSecurity();
        Result result = platform_.begin();
        if (result) {
            result = deviceInstanceStore_.loadOrCreate(
                random_, deviceInstance_);
        }
        if (!result) {
            platform_.end();
            return result;
        }
        initialized_ = true;
        return Result::success();
    }

    void end() {
        if (!initialized_) return;
        platform_.end();
        initialized_ = false;
    }

    bool initialized() const { return initialized_; }
    Platform& platform() { return platform_; }
    ArduinoClock& clock() { return clock_; }
    PlatformHardwareRandom& random() { return random_; }
    const DeviceInstanceId& deviceInstance() const {
        return deviceInstance_;
    }
    OwnershipClaimCoordinator& claim() { return claim_; }
    CloudCredentialEnrollmentCoordinator& cloudEnrollment() {
        return cloudEnrollment_;
    }
    WifiConnectionLifecycle& wifiLifecycle() { return wifiLifecycle_; }
    WifiControlPlaneArena& arena() { return arena_; }
    ControlHttp& controlHttp() { return controlHttp_; }
    ManagedMqttTransport& cloudTransport() { return cloudTransport_; }

private:
    enum : uint16_t {
        kMqttPacketBufferSize =
            BLINKER_DEVICE_FRAME_SIZE + kCloudSessionMaxTopicSize + 9U
    };

    Platform platform_;
    ArduinoClock clock_;
    PlatformHardwareRandom random_;

    DeviceInstanceId deviceInstance_;
    DeviceInstanceIdStore deviceInstanceStore_;
    OwnershipClaimRecordStore claimJournal_;
    OwnershipClaimGrantVerifier claimVerifier_;
    OwnershipClaimCoordinator claim_;
    CloudEnrollmentRecordStore cloudEnrollmentJournal_;
    CloudEnrollmentKeyRing encryptionKeys_;
    CloudCredentialEnvelopeSealer envelopeSealer_;
    CloudCredentialEnrollmentGrantVerifier cloudGrantVerifier_;
    CloudCredentialEnrollmentCoordinator cloudEnrollment_;

    WifiCredentialProfile wifiProfile_;
    WifiConnectionLifecycle wifiLifecycle_;
    WifiControlPlaneArena arena_;
    ControlHttp controlHttp_;
    HttpCloudSessionProvider cloudSession_;

    PubSubClient nativeMqtt_;
    PubSubClientAdapter<0U, 96U, 64U, 64U, 96U, 96U> mqttAdapter_;
    MqttFrameTransport mqttTransport_;
    ManagedMqttTransport cloudTransport_;
    bool initialized_;

    WifiCloudStack(const WifiCloudStack&);
    WifiCloudStack& operator=(const WifiCloudStack&);
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
