#include <BlinkerV2/api/Client.h>
#include <BlinkerV2/interface/IAuthorizationProvider.h>
#include <BlinkerV2/interface/IBleModeLink.h>
#include <BlinkerV2/runtime/BleOnlyLifecycle.h>
#include <BlinkerV2/runtime/EdgeHubLifecycle.h>
#include <BlinkerV2/runtime/WifiCloudLifecycle.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define WIFI_BLE_CHECK(condition)                                             \
    do {                                                                      \
        if (!(condition)) {                                                   \
            std::cerr << "WiFi+BLE lifecycle check failed at line "         \
                      << __LINE__ << ": " << #condition << std::endl;        \
            std::exit(EXIT_FAILURE);                                          \
        }                                                                     \
    } while (false)

class FakeWifiConnection {
public:
    FakeWifiConnection()
        : currentState(blinker::WifiConnectionState::Stopped),
          error(blinker::ErrorCode::Ok), confirmed(false), connected(false),
          startResult(blinker::Result::success()),
          startCalls(0U), stopCalls(0U), pollCalls(0U) {}

    blinker::Result start() {
        ++startCalls;
        if (!startResult) {
            error = startResult.code();
            currentState = blinker::WifiConnectionState::Fault;
            return startResult;
        }
        error = blinker::ErrorCode::Ok;
        currentState = connected
                           ? blinker::WifiConnectionState::Online
                           : blinker::WifiConnectionState::WaitingForCredentials;
        return blinker::Result::success();
    }
    void stop() {
        ++stopCalls;
        currentState = blinker::WifiConnectionState::Stopped;
    }
    void poll() {
        ++pollCalls;
        if (currentState != blinker::WifiConnectionState::Fault) {
            currentState = connected
                               ? blinker::WifiConnectionState::Online
                               : blinker::WifiConnectionState::WaitingForCredentials;
        }
    }
    blinker::WifiConnectionState state() const { return currentState; }
    bool online() const { return currentState == blinker::WifiConnectionState::Online; }
    bool confirmedProfile() const { return confirmed; }
    blinker::ErrorCode lastError() const { return error; }

    blinker::WifiConnectionState currentState;
    blinker::ErrorCode error;
    bool confirmed;
    bool connected;
    blinker::Result startResult;
    uint32_t startCalls;
    uint32_t stopCalls;
    uint32_t pollCalls;
};

class FakeModeLink : public blinker::IBleModeLink {
public:
    FakeModeLink()
        : linkState(blinker::BleLinkState::Stopped), profile(),
          packetReceiver(nullptr), packetContext(nullptr),
          connected(nullptr), disconnected(nullptr), sessionContext(nullptr),
          configureResult(blinker::Result::success()),
          startResult(blinker::Result::success()),
          error(blinker::ErrorCode::Ok),
          configureCalls(0U), startCalls(0U), stopCalls(0U) {}

    blinker::Result configureBleProfile(
        const blinker::ble::ModeProfile& value) override {
        if (linkState != blinker::BleLinkState::Stopped) {
            return blinker::Result::failure(blinker::ErrorCode::StateConflict);
        }
        blinker::Result result = blinker::ble::validateModeProfile(value);
        ++configureCalls;
        if (result && !configureResult) result = configureResult;
        if (result) {
            profile = value;
        }
        return result;
    }
    blinker::ble::ModeProfile bleProfile() const override { return profile; }
    blinker::Result start() override {
        if (linkState != blinker::BleLinkState::Stopped) {
            return blinker::Result::failure(blinker::ErrorCode::AlreadyExists);
        }
        ++startCalls;
        if (!startResult) {
            error = startResult.code();
            linkState = blinker::BleLinkState::Error;
            return startResult;
        }
        error = blinker::ErrorCode::Ok;
        linkState = blinker::BleLinkState::Ready;
        return blinker::Result::success();
    }
    void stop() override {
        ++stopCalls;
        linkState = blinker::BleLinkState::Stopped;
        error = blinker::ErrorCode::Ok;
    }
    void poll(uint32_t) override {}
    blinker::BleLinkState state() const override { return linkState; }
    blinker::ErrorCode lastError() const override {
        return error;
    }
    size_t sessionCount() const override { return 0U; }
    blinker::Result sessionAt(
        size_t, blinker::BleSessionInfo&) const override {
        return blinker::Result::failure(blinker::ErrorCode::NotFound);
    }
    blinker::Result sendPacket(uint32_t, blinker::ByteView) override {
        return blinker::Result::failure(blinker::ErrorCode::NotConnected);
    }
    void setPacketReceiver(
        blinker::BlePacketReceiver receiver, void* context) override {
        packetReceiver = receiver;
        packetContext = context;
    }
    void setSessionHandlers(
        blinker::BleSessionHandler onConnected,
        blinker::BleSessionHandler onDisconnected,
        void* context) override {
        connected = onConnected;
        disconnected = onDisconnected;
        sessionContext = context;
    }

    blinker::BleLinkState linkState;
    blinker::ble::ModeProfile profile;
    blinker::BlePacketReceiver packetReceiver;
    void* packetContext;
    blinker::BleSessionHandler connected;
    blinker::BleSessionHandler disconnected;
    void* sessionContext;
    blinker::Result configureResult;
    blinker::Result startResult;
    blinker::ErrorCode error;
    uint32_t configureCalls;
    uint32_t startCalls;
    uint32_t stopCalls;
};

struct FakeSetupMetrics {
    uint32_t requests;
    FakeSetupMetrics() : requests(0U) {}
};

class FakeProvisioningEndpoint : public blinker::ITransportLifecycle {
public:
    explicit FakeProvisioningEndpoint(FakeModeLink& link)
        : link_(link), values(), sessionId(0U), recovery(false),
          started(false), startCalls(0U), pollCalls(0U) {}

    blinker::Result start() override {
        blinker::Result result = link_.start();
        if (result) {
            started = true;
            recovery = false;
            ++startCalls;
        }
        return result;
    }
    void stop() override {
        if (started) link_.stop();
        started = false;
        sessionId = 0U;
        recovery = false;
    }
    void poll(uint32_t) override { ++pollCalls; }
    blinker::TransportState state() const override {
        if (!started) return blinker::TransportState::Stopped;
        return recovery ? blinker::TransportState::Error
                        : blinker::TransportState::Online;
    }
    uint32_t activeSessionId() const { return sessionId; }
    bool recoveryRequired() const { return recovery; }
    const FakeSetupMetrics& metrics() const { return values; }
    void connect(uint32_t value) { sessionId = value; }
    void recordRequest() { ++values.requests; }
    void disconnect() {
        sessionId = 0U;
        recovery = true;
    }

    FakeModeLink& link_;
    FakeSetupMetrics values;
    uint32_t sessionId;
    bool recovery;
    bool started;
    uint32_t startCalls;
    uint32_t pollCalls;
};

class FakeFrameTransport : public blinker::IFrameTransport {
public:
    FakeFrameTransport(uint16_t transportFeatures, FakeModeLink* link = nullptr)
        : features(transportFeatures), link_(link),
          currentState(blinker::TransportState::Stopped), network(true),
          startResult(blinker::Result::success()),
          error(blinker::ErrorCode::Ok),
          receiver(nullptr), receiverContext(nullptr), connected(nullptr),
          disconnected(nullptr), sessionContext(nullptr), startCalls(0U),
          stopCalls(0U), pollCalls(0U), sendCalls(0U) {}

    void setNetworkAvailable(bool available) {
        if (network == available) return;
        network = available;
        if (currentState != blinker::TransportState::Stopped &&
            currentState != blinker::TransportState::Error) {
            currentState = available ? blinker::TransportState::Online
                                     : blinker::TransportState::Starting;
        }
    }
    bool networkAvailable() const { return network; }
    blinker::ErrorCode lastError() const {
        return link_ != nullptr && link_->state() == blinker::BleLinkState::Error
                   ? link_->lastError()
                   : error;
    }
    blinker::Result start() override {
        ++startCalls;
        if (!startResult) {
            error = startResult.code();
            return startResult;
        }
        if (link_ != nullptr) {
            blinker::Result result = link_->start();
            if (!result) return result;
        }
        error = blinker::ErrorCode::Ok;
        currentState = network ? blinker::TransportState::Online
                               : blinker::TransportState::Starting;
        return blinker::Result::success();
    }
    void stop() override {
        if (currentState == blinker::TransportState::Stopped) return;
        if (link_ != nullptr) link_->stop();
        ++stopCalls;
        currentState = blinker::TransportState::Stopped;
    }
    void poll(uint32_t) override { ++pollCalls; }
    blinker::TransportState state() const override {
        return link_ != nullptr && link_->state() == blinker::BleLinkState::Error
                   ? blinker::TransportState::Error
                   : currentState;
    }
    blinker::TransportCapabilities capabilities() const override {
        blinker::TransportCapabilities result;
        result.maxFrameSize = 256U;
        result.features = features;
        return result;
    }
    blinker::Result send(
        blinker::ByteView, const blinker::SendTarget&) override {
        ++sendCalls;
        return currentState == blinker::TransportState::Online
                   ? blinker::Result::success()
                   : blinker::Result::failure(blinker::ErrorCode::NotConnected);
    }
    void setReceiver(
        blinker::FrameReceiver value, void* context) override {
        receiver = value;
        receiverContext = context;
    }
    void setSessionHandlers(
        blinker::FrameSessionHandler onConnected,
        blinker::FrameSessionHandler onDisconnected,
        void* context) override {
        connected = onConnected;
        disconnected = onDisconnected;
        sessionContext = context;
    }

    uint16_t features;
    FakeModeLink* link_;
    blinker::TransportState currentState;
    bool network;
    blinker::Result startResult;
    blinker::ErrorCode error;
    blinker::FrameReceiver receiver;
    void* receiverContext;
    blinker::FrameSessionHandler connected;
    blinker::FrameSessionHandler disconnected;
    void* sessionContext;
    uint32_t startCalls;
    uint32_t stopCalls;
    uint32_t pollCalls;
    uint32_t sendCalls;
};

class FakeTrustedClock : public blinker::IClock {
public:
    FakeTrustedClock()
        : now(0U), startedValue(false), trustedValue(false),
          startResult(blinker::Result::success()), startCalls(0U),
          stopCalls(0U), pollCalls(0U) {}

    blinker::Result start() {
        ++startCalls;
        if (!startResult) return startResult;
        startedValue = true;
        return blinker::Result::success();
    }
    void poll() { ++pollCalls; }
    void stop() {
        ++stopCalls;
        startedValue = false;
        trustedValue = false;
    }
    bool started() const { return startedValue; }
    bool trusted() const { return trustedValue; }
    uint32_t monotonicMillis() const override { return now; }
    bool unixTime(uint64_t& output) const override {
        output = trustedValue ? 1700000000ULL : 0U;
        return trustedValue;
    }

    uint32_t now;
    bool startedValue;
    bool trustedValue;
    blinker::Result startResult;
    uint32_t startCalls;
    uint32_t stopCalls;
    uint32_t pollCalls;
};

class FakeEdgeSession {
public:
    FakeEdgeSession() : generation(1U), version(7U) {}

    uint32_t credentialGeneration() const { return generation; }
    uint32_t authenticatedCredentialVersion() const { return version; }
    blinker::Result credentials(
        blinker::SessionCredentials& output) const {
        output.security = blinker::MqttSecurity::Tls;
        output.clientId = blinker::StringView("edge-test");
        return blinker::Result::success();
    }

    uint32_t generation;
    uint32_t version;
};

class FakeEdgeManagement {
public:
    FakeEdgeManagement()
        : configuredValue(false), subscribedValue(false),
          error(blinker::ErrorCode::Ok), configureCalls(0U),
          resetCalls(0U), pollCalls(0U), configuredVersion(0U) {}

    blinker::Result configure(
        const blinker::SessionCredentials& credentials,
        const blinker::GatewayAccessDeliveryContext& context) {
        ++configureCalls;
        if (credentials.security != blinker::MqttSecurity::Tls ||
            context.deviceKeyVersion == 0U) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        configuredValue = true;
        subscribedValue = false;
        configuredVersion = context.deviceKeyVersion;
        error = blinker::ErrorCode::Ok;
        return blinker::Result::success();
    }
    void reset() {
        ++resetCalls;
        configuredValue = false;
        subscribedValue = false;
        configuredVersion = 0U;
        error = blinker::ErrorCode::Ok;
    }
    void poll() {
        ++pollCalls;
        if (configuredValue) subscribedValue = true;
    }
    bool configured() const { return configuredValue; }
    bool subscribed() const { return subscribedValue; }
    blinker::ErrorCode lastError() const { return error; }

    bool configuredValue;
    bool subscribedValue;
    blinker::ErrorCode error;
    uint32_t configureCalls;
    uint32_t resetCalls;
    uint32_t pollCalls;
    uint32_t configuredVersion;
};

class FakeEdgeRoute {
public:
    FakeEdgeRoute()
        : configuredValue(false), subscribedValue(false),
          error(blinker::ErrorCode::Ok), configureCalls(0U),
          resetCalls(0U), pollCalls(0U) {}

    blinker::Result configure(
        const blinker::SessionCredentials& credentials) {
        ++configureCalls;
        if (credentials.security != blinker::MqttSecurity::Tls) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        configuredValue = true;
        subscribedValue = false;
        return blinker::Result::success();
    }
    void reset() {
        ++resetCalls;
        configuredValue = false;
        subscribedValue = false;
        error = blinker::ErrorCode::Ok;
    }
    void poll() {
        ++pollCalls;
        if (configuredValue) subscribedValue = true;
    }
    bool configured() const { return configuredValue; }
    bool subscribed() const { return subscribedValue; }
    blinker::ErrorCode lastError() const { return error; }

    bool configuredValue;
    bool subscribedValue;
    blinker::ErrorCode error;
    uint32_t configureCalls;
    uint32_t resetCalls;
    uint32_t pollCalls;
};

class FakeSetupCompletion : public blinker::IBleSetupCompletion {
public:
    FakeSetupCompletion() : present(false), complete(false), loadCalls(0U) {}

    blinker::Result load(bool& output) override {
        ++loadCalls;
        if (!present) {
            output = false;
            return blinker::Result::success();
        }
        output = complete;
        return blinker::Result::success();
    }

    bool present;
    bool complete;
    uint32_t loadCalls;
};

class FakeRandom : public blinker::IRandom {
public:
    FakeRandom() : calls(0U) {}
    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = static_cast<uint8_t>(index + calls);
        }
        return blinker::Result::success();
    }
    uint32_t calls;
};

class FakeAuthorization : public blinker::IAuthorizationProvider {
public:
    size_t methodCount() const override { return 1U; }
    uint16_t methodAt(size_t index) const override {
        return index == 0U ? 2U : 0U;
    }
    blinker::Result authorize(
        uint16_t,
        blinker::ByteView,
        const blinker::AuthorizationSessionContext&,
        blinker::AuthorizationDecision& decision) override {
        decision = blinker::AuthorizationDecision();
        return blinker::Result::success();
    }
    void sessionClosed(
        const blinker::AuthorizationSessionContext&) override {}
    void resetSessions() override {}
};

blinker::DeviceRuntimeConfig runtimeConfig() {
    blinker::DeviceRuntimeConfig config;
    config.maxFrameSize = 256U;
    config.maxReassemblySize = 256U;
    return config;
}

struct BleOnlyFixture {
    typedef blinker::BasicBleOnlyLifecycle<FakeProvisioningEndpoint>
        Lifecycle;

    blinker::EndpointDescriptor descriptor;
    blinker::ClientStorage<1U, 256U, 64U, 8U> storage;
    blinker::Client client;
    FakeModeLink link;
    FakeProvisioningEndpoint provisioning;
    FakeFrameTransport direct;
    blinker::BleModeCoordinator coordinator;
    FakeSetupCompletion completion;
    FakeAuthorization authorization;
    FakeRandom random;
    blinker::BleSetupLifecycleConfig config;
    Lifecycle lifecycle;

    BleOnlyFixture()
        : descriptor(
              blinker::StringView("power", 5U),
              blinker::EndpointKind::Property,
              blinker::ValueType::Boolean,
              static_cast<uint8_t>(
                  blinker::AccessRead | blinker::AccessNotify)),
          storage(),
          client(storage.buffers(&descriptor, 1U), runtimeConfig()),
          link(),
          provisioning(link),
          direct(static_cast<uint16_t>(
                     blinker::TransportFeatureBinary |
                     blinker::TransportFeatureLocal |
                     blinker::TransportFeatureFragmented),
                 &link),
          coordinator(link, provisioning, direct),
          completion(),
          authorization(),
          random(),
          config(),
          lifecycle(coordinator,
                    provisioning,
                    completion,
                    direct,
                    authorization,
                    random,
                    config) {}
};

struct WifiCloudFixture {
    typedef blinker::BasicWifiCloudLifecycle<
        FakeWifiConnection,
        FakeFrameTransport> Lifecycle;

    blinker::EndpointDescriptor descriptor;
    blinker::ClientStorage<1U, 256U, 64U, 8U> storage;
    blinker::Client client;
    FakeWifiConnection wifi;
    FakeFrameTransport cloud;
    Lifecycle lifecycle;

    WifiCloudFixture()
        : descriptor(
              blinker::StringView("power", 5U),
              blinker::EndpointKind::Property,
              blinker::ValueType::Boolean,
              static_cast<uint8_t>(
                  blinker::AccessRead | blinker::AccessNotify)),
          storage(),
          client(storage.buffers(&descriptor, 1U), runtimeConfig()),
          wifi(),
          cloud(static_cast<uint16_t>(
              blinker::TransportFeatureBinary |
              blinker::TransportFeatureCloud |
              blinker::TransportFeatureAuthenticated)),
          lifecycle(wifi, cloud) {}
};

struct EdgeHubFixture {
    typedef blinker::BasicEdgeHubLifecycle<
        FakeWifiConnection,
        FakeFrameTransport,
        FakeEdgeSession,
        FakeTrustedClock,
        FakeEdgeManagement,
        FakeEdgeRoute> Lifecycle;

    blinker::EndpointDescriptor descriptor;
    blinker::ClientStorage<1U, 256U, 64U, 8U> storage;
    blinker::Client client;
    FakeWifiConnection wifi;
    FakeFrameTransport cloud;
    FakeEdgeSession session;
    FakeTrustedClock clock;
    FakeEdgeManagement management;
    FakeEdgeRoute route;
    Lifecycle lifecycle;

    EdgeHubFixture()
        : descriptor(
              blinker::StringView("power", 5U),
              blinker::EndpointKind::Property,
              blinker::ValueType::Boolean,
              static_cast<uint8_t>(
                  blinker::AccessRead | blinker::AccessNotify)),
          storage(),
          client(storage.buffers(&descriptor, 1U), runtimeConfig()),
          wifi(),
          cloud(static_cast<uint16_t>(
              blinker::TransportFeatureBinary |
              blinker::TransportFeatureCloud |
              blinker::TransportFeatureAuthenticated)),
          session(), clock(), management(), route(),
          lifecycle(wifi, cloud, session, clock, management, route) {}
};

void testBleOnlyPlatformEnrollmentLifecycle() {
    BleOnlyFixture fixture;
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    WIFI_BLE_CHECK(
        (fixture.link.profile.capabilities &
         blinker::ble::ModeCapabilityEnrollmentV2) != 0U);
    WIFI_BLE_CHECK(
        (fixture.link.profile.capabilities &
         blinker::ble::ModeCapabilityWifiConfigV1) == 0U);

    fixture.provisioning.connect(11U);
    fixture.completion.present = true;
    fixture.completion.complete = true;
    fixture.provisioning.recordRequest();
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(fixture.lifecycle.setupComplete());
    fixture.provisioning.disconnect();
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.coordinator.activeMode() ==
        blinker::ble::ApplicationMode::Direct);
    fixture.lifecycle.stop();
}

void testBleOnlyRecoveryAndFaultBoundaries() {
    BleOnlyFixture fixture;
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    const blinker::ble::ModeProfile first = fixture.link.profile;
    fixture.provisioning.connect(12U);
    fixture.provisioning.disconnect();
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(blinker::ble::modeProfilesEqual(first, fixture.link.profile));
    WIFI_BLE_CHECK(fixture.provisioning.startCalls == 2U);

    fixture.completion.present = true;
    fixture.completion.complete = true;
    fixture.provisioning.recordRequest();
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.coordinator.activeMode() ==
        blinker::ble::ApplicationMode::Direct);
    fixture.link.error = blinker::ErrorCode::AuthenticationRequired;
    fixture.link.linkState = blinker::BleLinkState::Error;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() == blinker::BleOnlyLifecycleState::Fault);
    WIFI_BLE_CHECK(
        fixture.lifecycle.status().lastError ==
        blinker::ErrorCode::AuthenticationRequired);
}

void testBleAttachmentRollbackPreservesOtherOwners() {
    FakeAuthorization retained;
    BleOnlyFixture fixture;
    WIFI_BLE_CHECK(fixture.client.addTransport(fixture.direct,
        blinker::TransportLifecyclePolicy::External,
        blinker::TransportAccess(&retained)));
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client).code() ==
                   blinker::ErrorCode::AlreadyExists);
    WIFI_BLE_CHECK(fixture.client.status().transportCount == 1U);
    WIFI_BLE_CHECK(fixture.client.removeTransport(fixture.direct));
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
}

void testBleDestructionReleasesBinding() {
    BleOnlyFixture fixture;
    {
        BleOnlyFixture::Lifecycle scoped(
            fixture.coordinator, fixture.provisioning, fixture.completion,
            fixture.direct, fixture.authorization, fixture.random,
            fixture.config);
        WIFI_BLE_CHECK(scoped.attach(fixture.client));
        WIFI_BLE_CHECK(scoped.start());
        WIFI_BLE_CHECK(fixture.client.status().transportCount == 1U);
    }
    WIFI_BLE_CHECK(!fixture.client.status().started);
    WIFI_BLE_CHECK(fixture.client.status().transportCount == 0U);
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.lifecycle.start());
}

void testWifiCloudSelectsOneCarrier() {
    WifiCloudFixture fixture;
    FakeFrameTransport carrier(fixture.cloud.features);
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client, carrier));
    WIFI_BLE_CHECK(fixture.client.status().transportCount == 1U);
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    fixture.wifi.connected = true;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(fixture.cloud.networkAvailable()); // WiFi still controls the underlying connection.
    WIFI_BLE_CHECK(carrier.startCalls == 1U && carrier.pollCalls == 1U);
    WIFI_BLE_CHECK(fixture.cloud.startCalls == 0U && fixture.cloud.pollCalls == 0U);
    fixture.lifecycle.stop();
    WIFI_BLE_CHECK(carrier.stopCalls == 1U && !fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    WIFI_BLE_CHECK(carrier.startCalls == 2U && fixture.client.status().transportCount == 1U);
    fixture.lifecycle.stop(); // Borrowed carrier must outlive the running lifecycle.
}

void testWifiCloudLifecycleOrderingAndFaults() {
    {
        WifiCloudFixture fixture;
        WIFI_BLE_CHECK(
            fixture.lifecycle.start().code() ==
            blinker::ErrorCode::NotConfigured);
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        WIFI_BLE_CHECK(fixture.lifecycle.start());
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::NetworkConnecting);
        WIFI_BLE_CHECK(
            fixture.lifecycle.status().state ==
            blinker::ProductLifecycleState::Provisioning);
        WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
        WIFI_BLE_CHECK(fixture.client.status().started);

        fixture.wifi.connected = true;
        fixture.lifecycle.poll(1000U);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::Online);
        WIFI_BLE_CHECK(
            fixture.lifecycle.status().state ==
            blinker::ProductLifecycleState::Active);
        WIFI_BLE_CHECK(fixture.lifecycle.status().networkReady);
        WIFI_BLE_CHECK(fixture.client.status().cloudReady);

        fixture.wifi.connected = false;
        fixture.lifecycle.poll(1000U);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::NetworkConnecting);
        WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
        fixture.lifecycle.stop();
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::Stopped);
        WIFI_BLE_CHECK(!fixture.client.status().started);
    }

    {
        WifiCloudFixture fixture;
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        fixture.wifi.startResult = blinker::Result::failure(
            blinker::ErrorCode::NotConfigured);
        WIFI_BLE_CHECK(
            fixture.lifecycle.start().code() ==
            blinker::ErrorCode::NotConfigured);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::Stopped);
        fixture.wifi.startResult = blinker::Result::success();
        WIFI_BLE_CHECK(fixture.lifecycle.start());
        fixture.lifecycle.stop();
    }

    {
        WifiCloudFixture fixture;
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        fixture.cloud.startResult = blinker::Result::failure(
            blinker::ErrorCode::InternalError);
        WIFI_BLE_CHECK(
            fixture.lifecycle.start().code() ==
            blinker::ErrorCode::InternalError);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::Stopped);
        fixture.cloud.startResult = blinker::Result::success();
        WIFI_BLE_CHECK(fixture.lifecycle.start());
        fixture.lifecycle.stop();
    }

    {
        WifiCloudFixture fixture;
        fixture.wifi.connected = true;
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        WIFI_BLE_CHECK(fixture.lifecycle.start());
        fixture.cloud.error = blinker::ErrorCode::AuthenticationRequired;
        fixture.cloud.currentState = blinker::TransportState::Error;
        fixture.lifecycle.poll(1000U);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::AccessFault);
        WIFI_BLE_CHECK(
            fixture.lifecycle.status().lastError ==
            blinker::ErrorCode::AuthenticationRequired);
        WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
        WIFI_BLE_CHECK(!fixture.client.status().started);
        WIFI_BLE_CHECK(!fixture.client.status().cloudReady);
        WIFI_BLE_CHECK(!fixture.lifecycle.status().networkReady);
        WIFI_BLE_CHECK(fixture.lifecycle.status().state ==
                       blinker::ProductLifecycleState::Fault);
    }

    {
        WifiCloudFixture fixture;
        WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
        WIFI_BLE_CHECK(fixture.lifecycle.start());
        fixture.wifi.error = blinker::ErrorCode::NotConnected;
        fixture.wifi.currentState = blinker::WifiConnectionState::Fault;
        fixture.lifecycle.poll(1000U);
        WIFI_BLE_CHECK(
            fixture.lifecycle.state() ==
            blinker::WifiCloudLifecycleState::NetworkFault);
        WIFI_BLE_CHECK(
            fixture.lifecycle.status().lastError ==
            blinker::ErrorCode::NotConnected);
        WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
        WIFI_BLE_CHECK(fixture.client.status().started);
        WIFI_BLE_CHECK(!fixture.lifecycle.status().networkReady);
        WIFI_BLE_CHECK(fixture.lifecycle.status().state ==
                       blinker::ProductLifecycleState::Degraded);
    }
}

void testWifiCloudFailureIsolationAndRecovery() {
    // Keep transports alive until the fixture/lifecycle has stopped them.
    FakeFrameTransport local(blinker::TransportFeatureLocal);
    FakeFrameTransport otherLocal(blinker::TransportFeatureLocal);
    WifiCloudFixture fixture;
    WIFI_BLE_CHECK(fixture.client.addTransport(local));
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.client.addTransport(otherLocal));
    fixture.wifi.connected = true;
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    fixture.lifecycle.poll(900U);

    const uint32_t localPolls = local.pollCalls;
    fixture.cloud.currentState = blinker::TransportState::Error;
    fixture.cloud.error = blinker::ErrorCode::ProtocolError;
    for (uint8_t index = 0U; index < 3U; ++index) {
        fixture.lifecycle.poll(900U);
    }
    WIFI_BLE_CHECK(fixture.lifecycle.state() ==
                   blinker::WifiCloudLifecycleState::CloudFault);
    WIFI_BLE_CHECK(fixture.client.status().started);
    WIFI_BLE_CHECK(fixture.client.status().localReady);
    WIFI_BLE_CHECK(!fixture.client.status().cloudReady);
    WIFI_BLE_CHECK(fixture.client.status().errorCount == 1U);
    WIFI_BLE_CHECK(local.pollCalls == localPolls + 3U);
    WIFI_BLE_CHECK(otherLocal.pollCalls == localPolls + 3U);
    WIFI_BLE_CHECK(local.stopCalls == 0U && otherLocal.stopCalls == 0U);
    WIFI_BLE_CHECK(fixture.wifi.stopCalls == 0U);
    WIFI_BLE_CHECK(fixture.cloud.startCalls == 1U);

    // Recovery belongs to the cloud provider. Poll must observe it, without
    // restarting the whole Client or silently retrying a rejected credential.
    fixture.cloud.currentState = blinker::TransportState::Online;
    fixture.cloud.error = blinker::ErrorCode::Ok;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.state() ==
                   blinker::WifiCloudLifecycleState::Online);
    WIFI_BLE_CHECK(fixture.lifecycle.status().lastError == blinker::ErrorCode::Ok);
    WIFI_BLE_CHECK(local.startCalls == 1U && otherLocal.startCalls == 1U);

    fixture.cloud.currentState = blinker::TransportState::Backoff;
    fixture.cloud.error = blinker::ErrorCode::NotConnected;
    const uint32_t beforeBackoff = local.pollCalls;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.state() == blinker::WifiCloudLifecycleState::CloudConnecting);
    WIFI_BLE_CHECK(fixture.lifecycle.status().lastError == blinker::ErrorCode::NotConnected);
    WIFI_BLE_CHECK(fixture.lifecycle.status().state == blinker::ProductLifecycleState::Starting);
    WIFI_BLE_CHECK(fixture.client.status().started && fixture.client.status().localReady);
    WIFI_BLE_CHECK(local.pollCalls == beforeBackoff + 1U);
    WIFI_BLE_CHECK(local.stopCalls == 0U && otherLocal.stopCalls == 0U);
    fixture.cloud.currentState = blinker::TransportState::Online;
    fixture.cloud.error = blinker::ErrorCode::Ok;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.status().lastError == blinker::ErrorCode::Ok);

    fixture.wifi.currentState = blinker::WifiConnectionState::Fault;
    fixture.wifi.error = blinker::ErrorCode::InternalError;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.state() ==
                   blinker::WifiCloudLifecycleState::NetworkFault);
    WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(fixture.client.status().started);
    WIFI_BLE_CHECK(local.stopCalls == 0U);
    // These local fakes represent IP-independent links. A LAN adapter will
    // independently retire its sockets on IP loss; it is not implemented here.
    fixture.wifi.currentState = blinker::WifiConnectionState::Online;
    fixture.wifi.error = blinker::ErrorCode::Ok;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.status().state ==
                   blinker::ProductLifecycleState::Active);
    WIFI_BLE_CHECK(fixture.cloud.networkAvailable());

    fixture.cloud.currentState = blinker::TransportState::Error;
    fixture.cloud.error = blinker::ErrorCode::Ok;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.status().lastError ==
                   blinker::ErrorCode::InternalError);
    fixture.lifecycle.stop();
    WIFI_BLE_CHECK(!fixture.client.status().started);
    WIFI_BLE_CHECK(local.stopCalls == 1U && otherLocal.stopCalls == 1U);
    WIFI_BLE_CHECK(fixture.wifi.stopCalls == 1U);
    fixture.lifecycle.stop();
    WIFI_BLE_CHECK(fixture.wifi.stopCalls == 1U);
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.client.status().cloudReady);
    WIFI_BLE_CHECK(fixture.client.status().localReady);
    fixture.cloud.currentState = blinker::TransportState::Error;
    fixture.cloud.error = blinker::ErrorCode::AuthenticationRequired;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(fixture.lifecycle.state() ==
                   blinker::WifiCloudLifecycleState::AccessFault);
    WIFI_BLE_CHECK(!fixture.client.status().started);
    WIFI_BLE_CHECK(local.stopCalls == 2U && otherLocal.stopCalls == 2U);
    const uint32_t pollsAfterRevocation = local.pollCalls;
    fixture.lifecycle.poll(900U);
    WIFI_BLE_CHECK(local.pollCalls == pollsAfterRevocation);
}

void testEdgeHubAdmissionAndManagementBinding() {
    EdgeHubFixture fixture;
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());

    fixture.wifi.connected = true;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() ==
        blinker::EdgeHubLifecycleState::TimeSynchronizing);
    WIFI_BLE_CHECK(fixture.clock.startCalls == 1U);
    WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(fixture.management.configureCalls == 0U);

    fixture.clock.trustedValue = true;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() ==
        blinker::EdgeHubLifecycleState::Online);
    WIFI_BLE_CHECK(fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(fixture.management.configureCalls == 1U);
    WIFI_BLE_CHECK(fixture.management.configuredVersion == 7U);

    fixture.session.generation = 2U;
    fixture.session.version = 8U;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(fixture.management.configureCalls == 2U);
    WIFI_BLE_CHECK(fixture.management.configuredVersion == 8U);

    fixture.wifi.connected = false;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() ==
        blinker::EdgeHubLifecycleState::NetworkConnecting);
    WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(!fixture.management.configured());

    fixture.wifi.connected = true;
    fixture.clock.trustedValue = true;
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() ==
        blinker::EdgeHubLifecycleState::Online);
    WIFI_BLE_CHECK(fixture.management.configureCalls == 3U);
    fixture.lifecycle.stop();
}

void testEdgeHubFailsClosed() {
    EdgeHubFixture fixture;
    WIFI_BLE_CHECK(fixture.lifecycle.attach(fixture.client));
    WIFI_BLE_CHECK(fixture.lifecycle.start());
    fixture.wifi.connected = true;
    fixture.clock.startResult = blinker::Result::failure(
        blinker::ErrorCode::NotConfigured);
    fixture.lifecycle.poll(1000U);
    WIFI_BLE_CHECK(
        fixture.lifecycle.state() ==
        blinker::EdgeHubLifecycleState::Fault);
    WIFI_BLE_CHECK(!fixture.cloud.networkAvailable());
    WIFI_BLE_CHECK(
        fixture.lifecycle.status().lastError ==
        blinker::ErrorCode::NotConfigured);
}

} // namespace

void runConnectivityLifecycleTests() {
    testBleOnlyPlatformEnrollmentLifecycle();
    testBleOnlyRecoveryAndFaultBoundaries();
    testBleAttachmentRollbackPreservesOtherOwners();
    testBleDestructionReleasesBinding();
    testWifiCloudLifecycleOrderingAndFaults();
    testWifiCloudSelectsOneCarrier();
    testWifiCloudFailureIsolationAndRecovery();
    testEdgeHubAdmissionAndManagementBinding();
    testEdgeHubFailsClosed();
}
