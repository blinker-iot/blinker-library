#ifndef BLINKER_V2_ARDUINO_ESP32_INITIALNETWORKRECOVERYPRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_INITIALNETWORKRECOVERYPRODUCT_H

#include <sdkconfig.h>
#if !defined(ARDUINO_ARCH_ESP32) || !defined(ARDUINO_XIAO_ESP32S3) || \
    !CONFIG_IDF_TARGET_ESP32S3 || \
    !defined(BLINKER_INTERNAL_ESP32_INITIAL_NETWORK_RECOVERY) || BLINKER_INTERNAL_ESP32_INITIAL_NETWORK_RECOVERY != 1
#error "Initial network recovery is an explicit internal XIAO ESP32-S3 candidate"
#endif

#include "Esp32WiFiBLEProduct.h"
#include "../../ports/esp32/Esp32PreCloudWifiHandoffPorts.h"
#include "../../ports/esp32/Esp32PreCloudWifiOnlyReceiver.h"
#include <BlinkerV2/provisioning/WifiCredentialInitialNetworkBaseline.h>
#include <BlinkerV2/security/InitialCloudNetworkRecoveryEndpoint.h>
#include <new>
#include <type_traits>

#if BLINKER_TASK_CAPACITY != 0 || (defined(BLINKER_ENABLE_TASKS) && BLINKER_ENABLE_TASKS) || \
    (defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) && BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) || \
    !BLINKER_OFFICIAL_CLOUD_SESSION_TLS || !BLINKER_OFFICIAL_MQTT_TLS
#error "Initial network recovery requires task0, HTTPS and MQTTS"
#endif
#if (defined(BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) && BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL) || \
    (defined(BLINKER_INTERNAL_LAN_WEBSOCKET) && BLINKER_INTERNAL_LAN_WEBSOCKET) || \
    (defined(BLINKER_INTERNAL_ESP32_WIFIPROV_REKEY) && BLINKER_INTERNAL_ESP32_WIFIPROV_REKEY)
#error "Initial network recovery excludes LAN, rekey and other root writers"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

// Single-use, explicit maintenance composition. It does not select an ordinary
// product, install a Key, own a TaskStore or attach a Client in the private phase.
// Only the original cooperative loop may call its lifecycle; native callbacks
// only feed the existing transport/mailbox and never mutate durable roots.
class Esp32InitialNetworkRecoveryProduct final : public IProductLifecycle {
    // Keep the shared stack's open() ordering, but deny its loadOrCreate()
    // fallback even if the original instance temporarily reads NotFound.
    // This is a write-denying view, not a fresh durable NVS recovery handle.
    class ExistingInstanceBlob final : public IAtomicBlobStore {
    public:
        explicit ExistingInstanceBlob(IAtomicBlobStore& original) : original_(original) {}
        Result load(MutableByteSpan output, size_t& written) override {
            return original_.load(output, written);
        }
        Result replace(ByteView) override { return Result::failure(ErrorCode::StateConflict); }
        Result clear() override { return Result::failure(ErrorCode::StateConflict); }
        StorageProtection protection() const override { return original_.protection(); }
    private:
        IAtomicBlobStore& original_;
    };
    class RecoveryPlatform final : public Esp32WifiBlePlatform {
    public:
        RecoveryPlatform() : originalInstance_(Esp32WifiBlePlatform::deviceInstanceBlob()) {}
        IAtomicBlobStore& deviceInstanceBlob() { return originalInstance_; }
    private:
        ExistingInstanceBlob originalInstance_;
    };
    typedef DeviceKeyWifiStack<RecoveryPlatform> Stack;
    typedef BasicWifiCloudLifecycle<WifiConnectionLifecycle, Stack::CloudTransport> Cloud;
    enum : uint32_t { setupWindowMillis = 10UL * 60UL * 1000UL };

    struct Dedicated {
        Dedicated(Stack& stack, DeviceInstanceIdStore& instances,
            IInitialCloudNetworkBaseline& baseline, uint32_t openedAt,
            ByteView operation, const DeviceKeyLocator& locator)
            : profile(stack.deviceInstance(), stack.platform().deviceAccessStore(), stack.random(), stack.clock()),
              endpoints(EndpointDescriptorTable()), runtime(hub, endpoints, span(frame), runtimeConfig()),
              link(stack.platform().bleLink()),
              transport(link, stack.clock(), stack.platform().noiseCrypto(), span(rx), span(tx), span(packet),
                  span(plain), transportConfig()),
              registration(stack.deviceInstance(), stack.platform().deviceKeys(), stack.random(), stack.clock(),
                  transport, sessions, BLINKER_BLE_MAX_SESSIONS,
                  security::InitialCloudKeySetupWindow(openedAt, setupWindowMillis)),
              endpoint(registration, stack.platform().deviceKeys(), baseline, transport, operation, locator),
              binding{transport, registration, endpoint}, retirement(runtime, link),
              access(stack.platform().deviceKeys()),
              receiver(retirement, access, stack.platform().wifiCredentials(), stack.clock()),
              supervisor(endpoint, retirement, receiver, instances, stack.platform().deviceKeys(),
                  stack.clock(), openedAt, setupWindowMillis, &baseline) {}

        Result start() {
            ble::ModeProfile advertised;
            Result result = profile.make(transport.sessionRevision(), advertised);
            if (result && advertised.wireVersion != ble::kAuthorizedDirectBbp2WireVersion)
                result = Result::failure(ErrorCode::NotConfigured);
            if (result) result = link.configureBleProfile(advertised);
            if (result) result = runtime.bindPreCloudDirect(binding);
            if (result) result = runtime.start();
            return result;
        }

        Result pollDirect(uint32_t budget) {
            runtime.poll(budget);
            if (link.state() == BleLinkState::Error)
                return Result::failure(link.lastError() == ErrorCode::Ok ? ErrorCode::InternalError : link.lastError());
            if (!profile.refreshDue(link.sessionCount(), transport.sessionRevision())) return Result::success();
            ble::ModeProfile next;
            Result result = profile.make(transport.sessionRevision(), next);
            if (!result) return result;
            if (next.wireVersion != ble::kAuthorizedDirectBbp2WireVersion)
                return Result::failure(ErrorCode::StateConflict);
            result = link.refreshBleProfile(next);
            if (!result && (result.code() == ErrorCode::WouldBlock || result.code() == ErrorCode::StateConflict)) {
                profile.requestRefresh();
                return Result::success();
            }
            return result;
        }

        template <size_t N>
        static MutableByteSpan span(uint8_t (&buffer)[N]) { return MutableByteSpan(buffer, N); }
        static DeviceRuntimeConfig runtimeConfig() {
            DeviceRuntimeConfig config;
            config.protocolFeatures = bbp2::FeatureAuthentication;
            config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
            config.maxReassemblySize = BLINKER_DEVICE_FRAME_SIZE;
            return config;
        }
        static BleFrameTransportConfig transportConfig() {
            BleFrameTransportConfig config;
            config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
            return config;
        }

        enum : size_t { recordSize = BLINKER_DEVICE_FRAME_SIZE + security::kDirectSecureOverhead };
        BleDirectProfileProvider profile;
        TransportHub hub;
        EndpointRegistry endpoints;
        uint8_t frame[BLINKER_DEVICE_FRAME_SIZE] = {};
        DeviceRuntime runtime;
        Esp32NimBleLink& link;
        uint8_t rx[recordSize * BLINKER_BLE_MAX_SESSIONS] = {};
        uint8_t tx[recordSize * BLINKER_BLE_MAX_TX_FRAMES] = {};
        uint8_t packet[Esp32WifiBlePlatform::maximumBlePacketSize] = {};
        uint8_t plain[BLINKER_DEVICE_FRAME_SIZE] = {};
        BleFrameTransport transport;
        security::ControllerAuthSession sessions[BLINKER_BLE_MAX_SESSIONS] = {};
        security::InitialCloudKeyRegistration registration;
        security::InitialCloudNetworkRecoveryEndpoint endpoint;
        const PreCloudDirectBinding binding;
        Esp32PreCloudDirectRetirement retirement;
        security::InitialCloudWifiAccessSource access;
        Esp32PreCloudWifiOnlyReceiver receiver;
        PreCloudWifiHandoffSupervisor supervisor;
    };

public:
    Esp32InitialNetworkRecoveryProduct(ByteView freshOperation, const DeviceKeyLocator& expectedLocator)
        : stack_(platform_, platform_.deviceKeys()), baseline_(platform_.wifiCredentials()),
          instances_(platform_.deviceInstanceBlob()), cloud_(stack_.wifiLifecycle(), stack_.cloudTransport()),
          locator_(expectedLocator), operation_(),
          configured_(bbp2::exactInitialCloudNetworkBytes(freshOperation, sizeof(operation_)) &&
              bbp2::exactInitialCloudNetworkBytes(ByteView(locator_.bytes, sizeof(locator_.bytes)), sizeof(locator_.bytes))) {
        if (configured_) memcpy(operation_, freshOperation.data, sizeof(operation_));
    }
    ~Esp32InitialNetworkRecoveryProduct() override {
        stop();
        if (dedicated_ != nullptr) std::terminate(); // Native callbacks still own borrowed storage.
    }
    Esp32InitialNetworkRecoveryProduct(const Esp32InitialNetworkRecoveryProduct&) = delete;
    Esp32InitialNetworkRecoveryProduct& operator=(const Esp32InitialNetworkRecoveryProduct&) = delete;

    Result attach(Client& client) override {
        if (used_ || (client_ != nullptr && client_ != &client)) return Result::failure(ErrorCode::StateConflict);
        client_ = &client; // No addTransport(), Client::begin() or Cloud activity yet.
        return Result::success();
    }

    Result start() override {
        if (used_) return Result::failure(ErrorCode::StateConflict);
        if (client_ == nullptr || !configured_) return Result::failure(ErrorCode::NotConfigured);
        used_ = true;
        const uint32_t openedAt = stack_.clock().monotonicMillis();
        Result result = platform_.begin();
        platformOpened_ = result.ok();
        DeviceInstanceId expected;
        if (result) result = instances_.load(expected); // Never create/replace an original identity.
        if (result) result = checkInstalledRoot();
        if (result) result = baseline_.validateEmptyNetwork();
        if (result) result = stack_.open();
        if (result && !sameDeviceInstanceId(expected, stack_.deviceInstance()))
            result = Result::failure(ErrorCode::StateConflict);
        if (result) result = stack_.wifiLifecycle().setCandidateFailurePolicy(WifiCandidateFailurePolicy::Stop);
        if (!result) return fail(result.code());

        dedicated_ = new (&storage_) Dedicated(stack_, instances_, baseline_, openedAt,
            ByteView(operation_, sizeof(operation_)), locator_);
        result = dedicated_->start();
        if (!result) return fail(result.code());
        phase_ = Phase::Private;
        return Result::success();
    }

    void poll(uint32_t budget) override {
        if (stopping_) { finishStop(); return; }
        if (phase_ == Phase::Cloud) { cloud_.poll(budget); return; }
        if (phase_ != Phase::Private) return;
        if (!dedicated_->supervisor.windowLive()) { (void)fail(ErrorCode::StateConflict); return; }

        // Acceptance can still have unsent fragments after one bounded poll.
        // Retiring now would drop that receipt. Drain the existing queue before
        // taking the handoff; empty means local submission, NOT App receipt.
        if (dedicated_->supervisor.waitingForDirectHandoff() && dedicated_->transport.queuedFrameCount() != 0U) {
            const Result direct = dedicated_->pollDirect(budget);
            if (!direct) (void)fail(direct.code());
            return;
        }

        // Task0 has no installer/ordinary control root writer. This sole loop
        // owns fresh root+baseline validation and the complete journal call;
        // the HTTP callback only queues RAM and cannot enter this owner.
        const Result handoff = dedicated_->supervisor.advance();
        Result wifi = Result::failure(ErrorCode::WouldBlock);
        if (handoff) wifi = dedicated_->receiver.poll();
        if (!handoff) {
            if (handoff.code() != ErrorCode::WouldBlock) { (void)fail(handoff.code()); return; }
            if (!dedicated_->supervisor.waitingForDirectHandoff()) return;
            const Result direct = dedicated_->pollDirect(budget);
            if (!direct) (void)fail(direct.code());
            return;
        }
        if (!wifi) {
            if (wifi.code() != ErrorCode::WouldBlock || dedicated_->receiver.terminal()) (void)fail(wifi.code());
            return;
        }
        if (!dedicated_->receiver.staMayStart()) { (void)fail(ErrorCode::StateConflict); return; }
        Result result = client_->setMonotonicClock(&stack_.clock());
        if (result) result = cloud_.attach(*client_);
        if (result) result = dedicated_->receiver.startOriginalProduct(cloud_);
        if (!result) { (void)fail(result.code()); return; }
        destroyDedicated();
        phase_ = Phase::Cloud;
    }

    void stop() override {
        if (locked_) return;
        if (phase_ != Phase::Fault) phase_ = Phase::Stopped;
        cloud_.stop();
        if (dedicated_ != nullptr) dedicated_->supervisor.cancel();
        stopping_ = true;
        finishStop();
    }
    bool hasPendingStop() const override { return stopping_; }
    ProductLifecycleStatus status() const override {
        if (phase_ == Phase::Cloud) return cloud_.status();
        if (phase_ == Phase::Fault) return ProductLifecycleStatus(ProductLifecycleState::Fault, error_);
        if (phase_ == Phase::Private) return ProductLifecycleStatus(ProductLifecycleState::Provisioning);
        return ProductLifecycleStatus();
    }
    ProductCapabilities capabilities() const override { return cloud_.capabilities(); }

private:
    enum class Phase : uint8_t { Stopped, Private, Cloud, Fault };
    Result checkInstalledRoot() {
        DeviceAccessStore& access = platform_.deviceKeys();
        DeviceAccessAdminSelector selector;
        Result result = access.loadAdminSelector(selector);
        ControllerCredential admin;
        const ByteView id(selector.controllerId, sizeof(selector.controllerId));
        if (result) result = access.loadActive(id, ControllerCredentialDomain::PlatformAccess, selector.accessEpoch, admin);
        if (result && (!validateControllerCredential(admin) ||
            admin.suite != ControllerCredentialSuite::HmacSha256_32 ||
            admin.domain != ControllerCredentialDomain::PlatformAccess || admin.permissions != kControllerPermissionAll ||
            admin.ownershipGeneration != selector.accessEpoch || !constantTimeEqual(admin.id(), id)))
            result = Result::failure(ErrorCode::AuthenticationRequired);
        if (result) result = access.verifyInitialCloudWifiRoot(id, selector.accessEpoch, admin.credentialVersion, locator_);
        clearControllerCredential(admin);
        return result;
    }
    Result fail(ErrorCode error) {
        error_ = error == ErrorCode::Ok ? ErrorCode::InternalError : error;
        phase_ = Phase::Fault;
        stop();
        return Result::failure(error_);
    }
    void finishStop() {
        if (dedicated_ != nullptr) {
            const Result httpAp = dedicated_->receiver.end();
            if (!httpAp) { lockFault(httpAp.code()); return; }
            const Result direct = dedicated_->retirement.retireDirect();
            if (!direct && direct.code() == ErrorCode::WouldBlock) return;
            if (!direct || !dedicated_->retirement.shutdownComplete()) {
                lockFault(direct ? ErrorCode::StateConflict : direct.code());
                return;
            }
        }
        destroyDedicated();
        if (stack_.initialized()) stack_.end();
        else if (platformOpened_) platform_.end();
        platformOpened_ = false;
        stopping_ = false;
    }
    void lockFault(ErrorCode error) {
        error_ = error;
        phase_ = Phase::Fault;
        stopping_ = false;
        locked_ = true; // Pin callbacks; no blind native release or restart.
    }
    void destroyDedicated() {
        if (dedicated_ != nullptr) {
            dedicated_->~Dedicated();
            dedicated_ = nullptr;
        }
    }

    RecoveryPlatform platform_;
    Stack stack_;
    WifiCredentialInitialNetworkBaseline baseline_;
    DeviceInstanceIdStore instances_;
    Cloud cloud_;
    const DeviceKeyLocator locator_;
    uint8_t operation_[bbp2::kInitialCloudNetworkOperationSize];
    const bool configured_;
    typename std::aligned_storage<sizeof(Dedicated), alignof(Dedicated)>::type storage_;
    Dedicated* dedicated_ = nullptr;
    Client* client_ = nullptr;
    Phase phase_ = Phase::Stopped;
    ErrorCode error_ = ErrorCode::Ok;
    bool used_ = false;
    bool platformOpened_ = false;
    bool stopping_ = false;
    bool locked_ = false;
};

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
