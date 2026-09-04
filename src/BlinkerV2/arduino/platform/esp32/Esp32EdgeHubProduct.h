#ifndef BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_EDGEHUB_PRODUCT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32EdgeHubProduct requires an ESP32 Arduino target"
#endif

#include <sdkconfig.h>

#if defined(BLINKER_ESP32_PROVISIONING_BLE) && \
    defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
#error "Select only one ESP32 provisioning transport"
#endif

#if !defined(BLINKER_ESP32_PROVISIONING_BLE) && \
    !defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
#define BLINKER_ESP32_PROVISIONING_BLE 1
#endif

#if defined(BLINKER_ESP32_PROVISIONING_BLE) && \
    !defined(CONFIG_NIMBLE_ENABLED) && \
    !defined(CONFIG_BT_NIMBLE_ENABLED)
#error "BLE WiFiProv requires an ESP32 Arduino core built with IDF NimBLE"
#endif

#include "Esp32EdgeHubPlatform.h"
#include "../../internal/WiFiProduct.h"
#include "../../ports/esp32/Esp32Crypto.h"
#include "../../ports/esp32/Esp32NimBleCentralPort.h"
#include "../../ports/esp32/Esp32SntpClock.h"
#include "../../ports/esp32/Esp32WiFiProv.h"
#include "../../ports/arduino/HardwareRandom.h"

#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/identity/DeviceKeyStore.h>
#include <BlinkerV2/identity/GatewayAccessStore.h>
#include <BlinkerV2/identity/GatewayCredentialRenewalStore.h>
#include <BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.h>
#include <BlinkerV2/runtime/EdgeHubLifecycle.h>
#include <BlinkerV2/runtime/GatewayCloudMux.h>
#include <BlinkerV2/runtime/GatewayCredentialRenewalCoordinator.h>
#include <BlinkerV2/runtime/GatewayCredentialRenewalDeliveryProcessor.h>
#include <BlinkerV2/runtime/GatewayChildRouteBridge.h>
#include <BlinkerV2/runtime/GatewayManagementClient.h>
#include <BlinkerV2/runtime/GatewayManagementControlMux.h>
#include <BlinkerV2/runtime/GatewayManagementDeliveryMux.h>
#include <BlinkerV2/runtime/GatewayPermitJoinCoordinator.h>
#include <BlinkerV2/runtime/GatewayPermitJoinRelayClient.h>
#include <BlinkerV2/runtime/GatewayRouteClient.h>
#include <BlinkerV2/runtime/GatewayAccessDeliveryProcessor.h>
#include <BlinkerV2/runtime/GatewayGattChildSession.h>
#include <BlinkerV2/runtime/GatewayProofCoordinator.h>
#include <BlinkerV2/runtime/GatewayRevocationCoordinator.h>
#include <BlinkerV2/transport/NativeGattPermitJoinAdapter.h>

#include <string.h>

namespace blinker {
namespace integration {
namespace official_detail {

inline bool edgeHubPolicyConfigured() {
    return official::controlHost[0] != '\0' &&
           official::controlPort != 0U &&
           official::cloudSessionCaPem[0] != '\0' &&
           official::mqttCaPem[0] != '\0' &&
           official::edgeHubNvsPartition[0] != '\0' &&
           official::edgeHubNtpPrimary[0] != '\0';
}

inline HttpDeviceKeySessionConfig edgeHubSessionConfig() {
    HttpDeviceKeySessionConfig config;
    config.host = StringView(official::controlHost);
    config.port = official::controlPort;
    config.firmwareVersion = StringView(official::firmwareVersion);
    config.httpSecurity = HttpSecurity::Tls;
    config.mqttSecurity = MqttSecurity::Tls;
    return config;
}

inline MqttFrameTransportConfig edgeHubMqttConfig() {
    MqttFrameTransportConfig config;
    config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
    return config;
}

inline Esp32SntpClockConfig edgeHubClockConfig() {
    Esp32SntpClockConfig config;
    config.primaryServer = official::edgeHubNtpPrimary;
    config.secondaryServer = official::edgeHubNtpSecondary;
    return config;
}

class Esp32EdgeHubClock final : public Esp32SntpClock {
public:
    Esp32EdgeHubClock() : Esp32SntpClock(edgeHubClockConfig()) {}
};

enum class Esp32EdgeHubOnboardingMode : uint8_t {
    None = 0U,
    Manual,
    WifiProv
};

enum class Esp32EdgeHubProductState : uint8_t {
    Stopped = 0U,
    Provisioning,
    Active,
    Fault
};

// Internal D4 product. It intentionally has no public BlinkerEdgeHub facade;
// the ordinary Sketch API remains unchanged until the single-child gateway
// data plane and production hardware gates are complete.
template <typename Platform>
class BasicEsp32EdgeHubProduct final
    : public IProductLifecycle,
      private IGatewayPermitJoinPortLease {
public:
    enum : uint16_t {
        kEdgeHubMqttPacketBufferSize =
            static_cast<uint16_t>(
                GatewayRouteClient::maximumTopicSize) +
            static_cast<uint16_t>(
                GatewayRouteClient::maximumEnvelopeSize) + 9U
    };
    typedef DeviceKeyWifiStack<
        Platform,
        Esp32EdgeHubClock,
        kEdgeHubMqttPacketBufferSize> Stack;
    typedef BasicEdgeHubLifecycle<
        WifiConnectionLifecycle,
        ManagedMqttTransport,
        HttpDeviceKeySessionProvider,
        Esp32EdgeHubClock,
        GatewayManagementClient,
        GatewayRouteClient> Lifecycle;
    typedef BasicGatewayChildRouteBridge<
        GatewayGattChildSession,
        GatewayRouteClient> ChildRouteBridge;
    typedef BasicGatewayProofCoordinator<GatewayGattChildSession>
        ProofCoordinator;
    typedef BasicGatewayRevocationCoordinator<ChildRouteBridge>
        RevocationCoordinator;
    typedef BasicGatewayCredentialRenewalCoordinator<ChildRouteBridge>
        RenewalCoordinator;

    BasicEsp32EdgeHubProduct()
        : platform_(), deviceKey_(platform_.deviceKeyBlob()),
          stack_(
              platform_, deviceKey_, edgeHubSessionConfig(),
              MqttSecurity::Tls, edgeHubMqttConfig()),
          access_(platform_.gatewayAccessBlob()),
          renewal_(platform_.gatewayCredentialRenewalBlob()), crypto_(),
          delivery_(access_, deviceKey_, crypto_, stack_.clock()),
          renewalDelivery_(
              access_, renewal_, deviceKey_, crypto_, stack_.clock()),
          deliveryMux_(delivery_, renewalDelivery_),
          childPort_(), childRx_(), childTx_(), childPacket_(),
          childHandshake_(), childPlaintext_(), childSecureRecord_(),
          childLink_(
              childPort_, stack_.clock(),
              MutableByteSpan(childRx_, sizeof(childRx_)),
              MutableByteSpan(childTx_, sizeof(childTx_)),
              MutableByteSpan(childPacket_, sizeof(childPacket_))),
          childRandom_(),
          childSession_(
              childLink_, access_, renewal_, stack_.clock(), childRandom_,
              crypto_,
              MutableByteSpan(childHandshake_, sizeof(childHandshake_)),
              MutableByteSpan(childPlaintext_, sizeof(childPlaintext_)),
              MutableByteSpan(
                  childSecureRecord_, sizeof(childSecureRecord_))),
          proof_(childSession_, access_, stack_.clock()),
          cloudMux_(stack_.mqttTransport()),
          route_(cloudMux_),
          routeBridge_(
              childSession_, route_, access_, stack_.clock(), childRandom_),
          revocation_(routeBridge_, access_, stack_.clock()),
          renewalCoordinator_(
              routeBridge_, access_, renewal_, stack_.clock()),
          permitJoinAdapter_(childPort_, stack_.clock(), *this),
          permitJoin_(permitJoinAdapter_, stack_.clock()),
          managementControl_(
              proof_, revocation_, permitJoin_, renewalCoordinator_),
          management_(cloudMux_, deliveryMux_, managementControl_),
          permitJoinRelay_(
              cloudMux_, management_, permitJoin_, permitJoinAdapter_,
              stack_.clock()),
          lifecycle_(
              stack_.wifiLifecycle(), stack_.cloudTransport(),
              stack_.sessionProvider(), stack_.clock(), management_, route_),
          endpoint_(stack_.deviceInstance(), deviceKey_),
          provisioner_(endpoint_, platform_.wifiCredentialSink()),
          pendingDeviceKey_(),
          setupSsid_(), setupCredential_(), setupSsidSize_(0U),
          setupCredentialSize_(0U), serviceName_(),
          provisioningStartedMs_(0U), client_(nullptr),
          state_(Esp32EdgeHubProductState::Stopped),
          onboardingMode_(Esp32EdgeHubOnboardingMode::None),
          configurationError_(ErrorCode::Ok) {
        // Revocation and renewal share one private child-control channel.
        // The mux, not construction order, owns response dispatch.
        routeBridge_.setControllerControlReceiver(
            &IGatewayManagementControl::controllerControlResponseThunk,
            &managementControl_);
    }

    ~BasicEsp32EdgeHubProduct() override {
        routeBridge_.setControllerControlReceiver(nullptr, nullptr);
        stop();
        stack_.end();
        clearManualSetup();
    }

    Result configure(
        StringView deviceKey,
        StringView ssid,
        StringView password) {
        if (stack_.initialized() ||
            onboardingMode_ != Esp32EdgeHubOnboardingMode::None) {
            return remember(ErrorCode::AlreadyExists);
        }
        if (!edgeHubPolicyConfigured()) {
            return remember(ErrorCode::NotConfigured);
        }

        WifiNetworkConfig network;
        network.ssid = asBytes(ssid);
        network.authentication = password.empty()
                                     ? WifiAuthentication::Open
                                     : WifiAuthentication::Wpa2Personal;
        network.credential = asBytes(password);
        Result result = validateWifiNetworkConfig(network);
        DeviceKey decodedKey;
        if (result) result = decodeDeviceKey(deviceKey, decodedKey);
        if (!result) return remember(result.code());

        pendingDeviceKey_ = decodedKey;
        clearDeviceKey(decodedKey);
        memcpy(setupSsid_, network.ssid.data, network.ssid.size);
        if (!network.credential.empty()) {
            memcpy(
                setupCredential_, network.credential.data,
                network.credential.size);
        }
        setupSsidSize_ = static_cast<uint8_t>(network.ssid.size);
        setupCredentialSize_ =
            static_cast<uint8_t>(network.credential.size);
        onboardingMode_ = Esp32EdgeHubOnboardingMode::Manual;
        return Result::success();
    }

    Result configureProvisioning() {
        if (stack_.initialized() ||
            onboardingMode_ != Esp32EdgeHubOnboardingMode::None) {
            return remember(ErrorCode::AlreadyExists);
        }
        if (!edgeHubPolicyConfigured()) {
            return remember(ErrorCode::NotConfigured);
        }
        onboardingMode_ = Esp32EdgeHubOnboardingMode::WifiProv;
        return Result::success();
    }

    Result attach(Client& client) override {
        Result result = initialize();
        if (result) result = client.setMonotonicClock(&stack_.clock());
        if (result) result = lifecycle_.attach(client);
        if (result) {
            client_ = &client;
        } else {
            configurationError_ = result.code();
        }
        return result;
    }

    Result start() override {
        if (state_ != Esp32EdgeHubProductState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (!stack_.initialized() || client_ == nullptr ||
            onboardingMode_ == Esp32EdgeHubOnboardingMode::None) {
            return failStart(ErrorCode::NotConfigured);
        }

        if (onboardingMode_ == Esp32EdgeHubOnboardingMode::Manual) {
            return startActive();
        }

        DeviceKey current;
        Result result = deviceKey_.load(current);
        clearDeviceKey(current);
        const bool hasDeviceKey = result.ok();
        if (!result && result.code() != ErrorCode::NotFound) {
            return failStart(result.code());
        }
        if (hasDeviceKey &&
            platform_.wifiCredentials().hasActiveProfile()) {
            return startActive();
        }

        makeServiceName(stack_.deviceInstance(), serviceName_);
        endpoint_.reset();
        result = provisioner_.begin(provisioningConfig(serviceName_));
        if (!result) return failStart(result.code());
        provisioningStartedMs_ = stack_.clock().monotonicMillis();
        state_ = Esp32EdgeHubProductState::Provisioning;
        configurationError_ = ErrorCode::Ok;
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == Esp32EdgeHubProductState::Provisioning) {
            provisioner_.poll();
            const uint32_t elapsed =
                stack_.clock().monotonicMillis() - provisioningStartedMs_;
            if (provisioner_.state() == Esp32WifiProvState::Fault) {
                const ErrorCode error = provisioner_.lastError();
                provisioner_.end();
                enterFault(error);
            } else if (elapsed >= kProvisioningWindowMs) {
                provisioner_.end();
                enterFault(ErrorCode::NotConnected);
            } else if (provisioner_.state() ==
                       Esp32WifiProvState::Complete) {
                provisioner_.end();
                (void)startActive();
            }
            return;
        }
        if (state_ != Esp32EdgeHubProductState::Active) return;
        lifecycle_.poll(totalBudgetMicros);
        permitJoinRelay_.poll();
        const ProductLifecycleStatus current = lifecycle_.status();
        if (current.state == ProductLifecycleState::Fault) {
            enterFault(current.lastError);
            return;
        }

        const EdgeHubLifecycleState cloudState = lifecycle_.state();
        if (cloudState == EdgeHubLifecycleState::CloudConnecting ||
            cloudState == EdgeHubLifecycleState::ManagementSubscribing) {
            // Cloud route admission is per MQTT session, while the proved
            // DirectSecure child session is not. Suspend forwarding during a
            // credential refresh but keep servicing the local BLE session.
            routeBridge_.reset();
            if (childSession_.state() !=
                GatewayChildSessionState::Stopped) {
                childSession_.poll(totalBudgetMicros);
            }
            return;
        }
        if (cloudState != EdgeHubLifecycleState::Online) {
            stopChildDataPlane();
            return;
        }
        if (permitJoin_.windowOpen()) return;
        // The southbound radio is demand-driven. Permit-join owns it during
        // enrollment; proof, route admission or revocation starts the native
        // child session only when an authenticated task actually needs it.
        if (childSession_.state() !=
            GatewayChildSessionState::Stopped) {
            childSession_.poll(totalBudgetMicros);
        }
        routeBridge_.poll();
    }

    void stop() override {
        provisioner_.end();
        permitJoinRelay_.reset();
        stopChildDataPlane();
        lifecycle_.stop();
        state_ = Esp32EdgeHubProductState::Stopped;
        configurationError_ = ErrorCode::Ok;
    }

    Result resetAccess() override {
        const bool closeAfterReset = !stack_.initialized();
        Result result = stack_.open();
        // Outbound child authority must disappear before this Hub loses its
        // own cloud/direct access root. A reset interrupted between the two
        // writes therefore cannot leave an orphan child credential reachable
        // by a newly provisioned owner.
        if (result) result = renewal_.clear();
        if (result) result = access_.clear();
        if (result) result = deviceKey_.clear();
        if (closeAfterReset) stack_.end();
        return result;
    }

    Result resetNetwork() override {
        if (onboardingMode_ != Esp32EdgeHubOnboardingMode::WifiProv) {
            return Result::failure(ErrorCode::UnsupportedFeature);
        }
        const bool closeAfterReset = !stack_.initialized();
        Result result = stack_.open();
        if (result) result = platform_.wifiCredentials().clear();
        if (closeAfterReset) stack_.end();
        return result;
    }

    ProductLifecycleStatus status() const override {
        if (state_ == Esp32EdgeHubProductState::Provisioning) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Provisioning,
                ErrorCode::Ok,
                false);
        }
        if (state_ == Esp32EdgeHubProductState::Fault ||
            configurationError_ != ErrorCode::Ok) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Fault,
                configurationError_,
                false);
        }
        return lifecycle_.status();
    }

    ProductCapabilities capabilities() const override {
        uint16_t flags = static_cast<uint16_t>(
            ProductCapabilityCloudData |
            ProductCapabilityAccessReset);
        if (onboardingMode_ == Esp32EdgeHubOnboardingMode::WifiProv) {
            flags = static_cast<uint16_t>(
                flags | ProductCapabilityNetworkReset);
        }
        return ProductCapabilities(flags);
    }

    GatewayAccessStore& gatewayAccess() { return access_; }
    GatewayCredentialRenewalStore& gatewayCredentialRenewal() {
        return renewal_;
    }
    GatewayManagementClient& management() { return management_; }
    GatewayGattChildSession& childSession() { return childSession_; }
    ChildRouteBridge& routeBridge() { return routeBridge_; }
    RevocationCoordinator& revocation() { return revocation_; }
    RenewalCoordinator& renewalCoordinator() {
        return renewalCoordinator_;
    }
    GatewayPermitJoinCoordinator& permitJoin() { return permitJoin_; }
    GatewayPermitJoinRelayClient& permitJoinRelay() {
        return permitJoinRelay_;
    }
    NativeGattPermitJoinAdapter& permitJoinAdapter() {
        return permitJoinAdapter_;
    }
    WifiConnectionLifecycle& wifiLifecycle() {
        return stack_.wifiLifecycle();
    }

private:
    enum : uint32_t {
        kProvisioningWindowMs = 10UL * 60UL * 1000UL
    };
    enum : size_t {
        kChildFrameSize = 512U,
        kChildRecordSize =
            kChildFrameSize + security::kDirectSecureOverhead,
        kChildPacketSize = BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE
    };

    static ByteView asBytes(StringView value) {
        return ByteView(
            reinterpret_cast<const uint8_t*>(value.data), value.size);
    }

    Result initialize() {
        if (stack_.initialized()) return Result::success();
        if (configurationError_ != ErrorCode::Ok ||
            onboardingMode_ == Esp32EdgeHubOnboardingMode::None) {
            return Result::failure(
                configurationError_ == ErrorCode::Ok
                    ? ErrorCode::NotConfigured
                    : configurationError_);
        }
        Result result = stack_.open();
        if (result &&
            onboardingMode_ == Esp32EdgeHubOnboardingMode::Manual) {
            result = deviceKey_.replace(pendingDeviceKey_);
        }
        if (result &&
            onboardingMode_ == Esp32EdgeHubOnboardingMode::Manual) {
            WifiNetworkConfig network;
            network.ssid = ByteView(setupSsid_, setupSsidSize_);
            network.authentication = setupCredentialSize_ == 0U
                                         ? WifiAuthentication::Open
                                         : WifiAuthentication::Wpa2Personal;
            network.credential = ByteView(
                setupCredential_, setupCredentialSize_);
            result = stack_.platform().wifiCredentials().commit(network);
        }
        if (onboardingMode_ == Esp32EdgeHubOnboardingMode::Manual) {
            clearManualSetup();
        }
        if (!result) {
            stack_.end();
            configurationError_ = result.code();
        }
        return result;
    }

    Result remember(ErrorCode error) {
        configurationError_ = error;
        return Result::failure(error);
    }

    static Esp32WifiProvConfig provisioningConfig(
        const char* serviceName) {
#if defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
        return esp32SoftApWifiProvConfig(serviceName, nullptr, nullptr);
#else
        return esp32BleWifiProvConfig(serviceName, nullptr);
#endif
    }

    static void makeServiceName(
        const DeviceInstanceId& instance,
        char (&output)[32U]) {
        static const char hex[] = "0123456789ABCDEF";
        static const char prefix[] = "BLINKER_HUB_";
        size_t cursor = 0U;
        for (size_t index = 0U; index < sizeof(prefix) - 1U; ++index) {
            output[cursor++] = prefix[index];
        }
        for (size_t index = 0U; index < 6U; ++index) {
            output[cursor++] = hex[instance.bytes[index] >> 4U];
            output[cursor++] = hex[instance.bytes[index] & 0x0FU];
        }
        output[cursor] = '\0';
    }

    Result startActive() {
        Result result = stack_.begin();
        if (result) result = lifecycle_.start();
        if (!result) return failStart(result.code());
        state_ = Esp32EdgeHubProductState::Active;
        configurationError_ = ErrorCode::Ok;
        return Result::success();
    }

    Result failStart(ErrorCode error) {
        enterFault(error);
        return Result::failure(configurationError_);
    }

    void enterFault(ErrorCode error) {
        provisioner_.end();
        stopChildDataPlane();
        lifecycle_.stop();
        configurationError_ = error == ErrorCode::Ok
                                  ? ErrorCode::InternalError
                                  : error;
        state_ = Esp32EdgeHubProductState::Fault;
    }

    void stopChildDataPlane() {
        routeBridge_.reset();
        if (childSession_.state() == GatewayChildSessionState::Stopped) {
            return;
        }
        childSession_.stop();
    }

    Result acquirePermitJoinPort() override {
        stopChildDataPlane();
        return childPort_.state() == BleCentralPortState::Stopped
                   ? Result::success()
                   : Result::failure(ErrorCode::WouldBlock);
    }

    void releasePermitJoinPort() override {}

    void clearManualSetup() {
        clearDeviceKey(pendingDeviceKey_);
        secureZero(MutableByteSpan(setupSsid_, sizeof(setupSsid_)));
        secureZero(MutableByteSpan(
            setupCredential_, sizeof(setupCredential_)));
        setupSsidSize_ = 0U;
        setupCredentialSize_ = 0U;
    }

    Platform platform_;
    DeviceKeyStore deviceKey_;
    Stack stack_;
    GatewayAccessStore access_;
    GatewayCredentialRenewalStore renewal_;
    Esp32MbedTlsCryptoProvider crypto_;
    GatewayAccessDeliveryProcessor delivery_;
    GatewayCredentialRenewalDeliveryProcessor renewalDelivery_;
    GatewayManagementDeliveryMux deliveryMux_;
    Esp32NimBleCentralPort childPort_;
    uint8_t childRx_[kChildRecordSize];
    uint8_t childTx_[kChildRecordSize];
    uint8_t childPacket_[kChildPacketSize];
    uint8_t childHandshake_[kChildFrameSize];
    uint8_t childPlaintext_[kChildFrameSize];
    uint8_t childSecureRecord_[kChildRecordSize];
    GattDirectChildLink childLink_;
    PlatformHardwareRandom childRandom_;
    GatewayGattChildSession childSession_;
    ProofCoordinator proof_;
    GatewayCloudMux cloudMux_;
    GatewayRouteClient route_;
    ChildRouteBridge routeBridge_;
    RevocationCoordinator revocation_;
    RenewalCoordinator renewalCoordinator_;
    NativeGattPermitJoinAdapter permitJoinAdapter_;
    GatewayPermitJoinCoordinator permitJoin_;
    GatewayManagementControlMux managementControl_;
    GatewayManagementClient management_;
    GatewayPermitJoinRelayClient permitJoinRelay_;
    Lifecycle lifecycle_;
    DeviceKeyProvisioningEndpoint endpoint_;
    Esp32WifiProvAdapter provisioner_;
    DeviceKey pendingDeviceKey_;
    uint8_t setupSsid_[kWifiSsidMaxSize];
    uint8_t setupCredential_[kWifiCredentialMaxSize];
    uint8_t setupSsidSize_;
    uint8_t setupCredentialSize_;
    char serviceName_[32U];
    uint32_t provisioningStartedMs_;
    Client* client_;
    Esp32EdgeHubProductState state_;
    Esp32EdgeHubOnboardingMode onboardingMode_;
    ErrorCode configurationError_;

    BasicEsp32EdgeHubProduct(const BasicEsp32EdgeHubProduct&);
    BasicEsp32EdgeHubProduct& operator=(
        const BasicEsp32EdgeHubProduct&);
};

typedef BasicEsp32EdgeHubProduct<Esp32EdgeHubPlatform>
    Esp32EdgeHubProduct;

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
