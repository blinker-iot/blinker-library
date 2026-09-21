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
#include <BlinkerV2/provisioning/DeviceKeyProvisioningEndpoint.h>
#include <BlinkerV2/runtime/EdgeHubLifecycle.h>
#include <BlinkerV2/runtime/GatewayCloudMux.h>
#include <BlinkerV2/runtime/GatewayManagementClient.h>
#include <BlinkerV2/runtime/GatewayPermitJoinCoordinator.h>
#include <BlinkerV2/runtime/GatewayPermitJoinRelayClient.h>
#include <BlinkerV2/runtime/GatewayRouteClient.h>
#include <BlinkerV2/runtime/GatewayGattRadio.h>
#include <BlinkerV2/runtime/GatewayGattPool.h>
#include <BlinkerV2/runtime/GatewayGattChildContext.h>
#include <BlinkerV2/runtime/GatewayChildDirectory.h>
#include <BlinkerV2/runtime/GatewayChildManagementRouter.h>
#include <BlinkerV2/runtime/GatewayChildBinding.h>
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
template <typename Platform, typename RouteEndpoint = GatewayRouteEndpoint>
class BasicEsp32EdgeHubProduct final : public IProductLifecycle {
public:
    typedef GatewayRouteClient RouteClient;
    enum : uint16_t {
        kEdgeHubMqttPacketBufferSize =
            static_cast<uint16_t>(
                RouteClient::maximumTopicSize) +
            static_cast<uint16_t>(
                RouteClient::maximumEnvelopeSize) + 9U
    };
    typedef DeviceKeyWifiStack<
        Platform,
        Esp32EdgeHubClock,
        kEdgeHubMqttPacketBufferSize> Stack;
    typedef BasicEdgeHubLifecycle<
        WifiConnectionLifecycle,
        typename Stack::CloudTransport,
        HttpDeviceKeySessionProvider,
        Esp32EdgeHubClock,
        GatewayManagementClient,
        RouteClient> Lifecycle;
    typedef BasicGatewayGattChildContext<RouteEndpoint, 512U,
        BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE> ChildContext;
    typedef typename ChildContext::Bridge ChildRouteBridge;
    typedef typename ChildContext::Revocation RevocationCoordinator;
    typedef typename ChildContext::Renewal RenewalCoordinator;
    typedef BasicGatewayGattRadio<GatewayGattChildSession, Esp32NimBleCentralPort, ChildRouteBridge> ChildRadio;
    typedef BasicGatewayGattPool<ChildRadio, Esp32NimBleCentralPort> GattPool;
    enum : size_t { activeChildCapacity = 2U };
    static_assert(Esp32NimBleCentralHost::capacity >= activeChildCapacity,
                  "This internal two-child Edge Hub needs at least two SDK NimBLE slots");
    static_assert(Platform::GatewayStorage::capacity == 2U,
                  "Update binding composition when admitting a new durable capacity");

    BasicEsp32EdgeHubProduct()
        : platform_(), deviceKey_(platform_.deviceKeyBlob()),
          stack_(
              platform_, deviceKey_, edgeHubSessionConfig(),
              MqttSecurity::Tls, edgeHubMqttConfig()),
          crypto_(), childRandom_(), cloudMux_(stack_.mqttTransport()),
          route_(cloudMux_, stack_.clock()), centralHost_(),
          childPorts_{Esp32NimBleCentralPort(centralHost_), Esp32NimBleCentralPort(centralHost_)},
          bindings_{{*platform_.gatewayStorage().records(0U), deviceKey_, crypto_, stack_.clock()},
                    {*platform_.gatewayStorage().records(1U), deviceKey_, crypto_, stack_.clock()}},
          bootGuard_(stack_.clock()),
          children_{{childPorts_[0U], bindings_[0U].records, route_, stack_.clock(), childRandom_, crypto_},
                    {childPorts_[1U], bindings_[1U].records, route_, stack_.clock(), childRandom_, crypto_}},
          radios_{{children_[0U].session, childPorts_[0U], children_[0U].connection, children_[0U].bridge, children_[0U].management, bootGuard_, stack_.clock()},
                  {children_[1U].session, childPorts_[1U], children_[1U].connection, children_[1U].bridge, children_[1U].management, bootGuard_, stack_.clock()}},
          poolSlots_{{radios_[0U], childPorts_[0U], bindings_[0U].records}, {radios_[1U], childPorts_[1U], bindings_[1U].records}},
          gattPool_(poolSlots_, activeChildCapacity, bootGuard_, stack_.clock()),
          childEntries_{{bindings_[0U].records, bindings_[0U].delivery, &children_[0U].management},
                        {bindings_[1U].records, bindings_[1U].delivery, &children_[1U].management}},
          directory_(childEntries_, Platform::GatewayStorage::capacity),
          permitJoinAdapter_(stack_.clock(), gattPool_),
          permitJoin_(permitJoinAdapter_, stack_.clock()),
          managementRouter_(directory_, permitJoin_),
          management_(cloudMux_, directory_, managementRouter_),
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
        managementRouter_.setPermitAdmission(gattPool_);
    }

    ~BasicEsp32EdgeHubProduct() override {
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

    // Own-device business time only. Child Hello must not advertise a responder
    // until the southbound permission/negotiation/dispatch seam is installed.
    Result attachTime(Client& client, ITimeSync& service) override {
        return client.setTimeSync(service, stack_.clock(), stack_.random());
    }

    // Internal opt-in protocol injection, before any child attempt. The caller
    // owns each module and supplies the same own-device business-time source.
    Result setChildRequestHandler(size_t index, IGatewayChildRequestHandler* handler) {
        if (index >= activeChildCapacity) return Result::failure(ErrorCode::InvalidArgument);
        return children_[index].executor.setRequestHandler(handler);
    }

    Result start() override {
        if (state_ != Esp32EdgeHubProductState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (permitJoin_.ownsChildSession() || !gattPool_.stopped()) {
            return Result::failure(ErrorCode::WouldBlock);
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
        gattPool_.prepare(
            state_ == Esp32EdgeHubProductState::Active &&
            lifecycle_.state() == EdgeHubLifecycleState::Online);
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
        if (state_ != Esp32EdgeHubProductState::Active) {
            pollRadio(false, totalBudgetMicros);
            return;
        }
        lifecycle_.poll(totalBudgetMicros);
        const ProductLifecycleStatus current = lifecycle_.status();
        if (current.state == ProductLifecycleState::Fault) {
            enterFault(current.lastError);
            return;
        }

        pollRadio(lifecycle_.state() == EdgeHubLifecycleState::Online, totalBudgetMicros);
        permitJoinRelay_.poll();
    }

    void stop() override {
        provisioner_.end();
        permitJoinRelay_.reset();
        gattPool_.stop();
        lifecycle_.stop();
        state_ = Esp32EdgeHubProductState::Stopped;
        configurationError_ = ErrorCode::Ok;
    }

    Result resetAccess() override {
        const bool closeAfterReset = !stack_.initialized();
        directory_.invalidate();
        Result result = stack_.open();
        // Outbound child authority must disappear before this Hub loses its
        // own cloud/direct access root. A reset interrupted between the two
        // writes therefore cannot leave an orphan child credential reachable
        // by a newly provisioned owner.
        if (result) result = platform_.gatewayStorage().clear();
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

    ChildContext* child(size_t index) { return index < activeChildCapacity ? &children_[index] : nullptr; }
    GatewayManagementClient& management() { return management_; }
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

    static ByteView asBytes(StringView value) {
        return ByteView(
            reinterpret_cast<const uint8_t*>(value.data), value.size);
    }

    Result initialize() {
        if (stack_.initialized()) return Result::success();
        directory_.invalidate(); // Backend lifecycle is outside tracked writes.
        if (configurationError_ != ErrorCode::Ok ||
            onboardingMode_ == Esp32EdgeHubOnboardingMode::None) {
            return Result::failure(
                configurationError_ == ErrorCode::Ok
                    ? ErrorCode::NotConfigured
                    : configurationError_);
        }
        Result result = stack_.wifiLifecycle().setCandidateFailurePolicy(
            onboardingMode_ == Esp32EdgeHubOnboardingMode::Manual
                ? WifiCandidateFailurePolicy::Retry
                : WifiCandidateFailurePolicy::Rollback);
        if (result) result = stack_.open();
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
        gattPool_.stop();
        lifecycle_.stop();
        configurationError_ = error == ErrorCode::Ok
                                  ? ErrorCode::InternalError
                                  : error;
        state_ = Esp32EdgeHubProductState::Fault;
    }

    void pollRadio(bool online, uint32_t budgetMicros) {
        // A subscribed management client ticks the router. Offline or a failed
        // subscription still needs one Hub tick to drain the original budgets.
        if (!online && (lifecycle_.state() != EdgeHubLifecycleState::ManagementSubscribing ||
                !management_.subscribed())) managementRouter_.poll();
        gattPool_.poll(online, budgetMicros);
    }

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
    Esp32MbedTlsCryptoProvider crypto_;
    PlatformHardwareRandom childRandom_;
    GatewayCloudMux cloudMux_;
    RouteClient route_;
    Esp32NimBleCentralHost centralHost_;
    Esp32NimBleCentralPort childPorts_[activeChildCapacity];
    GatewayChildBinding bindings_[Platform::GatewayStorage::capacity];
    GatewayGattBootGuard bootGuard_;
    ChildContext children_[activeChildCapacity];
    ChildRadio radios_[activeChildCapacity];
    typename GattPool::Slot poolSlots_[activeChildCapacity];
    GattPool gattPool_;
    GatewayChildDirectoryEntry childEntries_[Platform::GatewayStorage::capacity];
    GatewayChildDirectory directory_;
    NativeGattPermitJoinAdapter permitJoinAdapter_;
    GatewayPermitJoinCoordinator permitJoin_;
    GatewayChildManagementRouter managementRouter_;
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
