#ifndef BLINKER_V2_ARDUINO_ESP32_PRECLOUDWIFIPRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_PRECLOUDWIFIPRODUCT_H

#include <sdkconfig.h>
#if !defined(ARDUINO_ARCH_ESP32) || !CONFIG_IDF_TARGET_ESP32S3 || \
    !defined(BLINKER_INTERNAL_ESP32_PRE_CLOUD_WIFI_CANDIDATE) || BLINKER_INTERNAL_ESP32_PRE_CLOUD_WIFI_CANDIDATE != 1
#error "PreCloud WiFi is an explicit internal ESP32-S3 candidate, not a default product"
#endif

#include "Esp32WiFiBLEProduct.h"
#include "Esp32ProvisionedTaskExtension.h"
#include <BlinkerV2/security/InitialCloudKeyRegistrationEndpoint.h>
#include "../../ports/esp32/Esp32PreCloudWifiHandoffPorts.h"
#include "../../ports/esp32/Esp32PreCloudWifiOnlyReceiver.h"
#include <new>
#include <type_traits>

#if !defined(BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS) || BLINKER_INTERNAL_ESP32_PROVISIONED_TASKS != 1 || \
    BLINKER_TASK_CAPACITY != 4 || !BLINKER_OFFICIAL_CLOUD_SESSION_TLS || !BLINKER_OFFICIAL_MQTT_TLS
#error "PreCloud WiFi requires the existing guarded four-slot ESP32 TLS profile"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

// Single-use maintenance product. Only its normal Cloud continuation owns a
// Client transport; TaskStore stays an internal installer, not a new task
// management/execution admission. Never selected by any public entry header.
class Esp32PreCloudWifiProduct final : public IProductLifecycle {
    typedef DeviceKeyWifiStack<Esp32WifiBlePlatform> Stack;
    typedef Esp32ProvisionedTaskExtension<Stack, BLINKER_TASK_CAPACITY> Installer;
    typedef BasicWifiCloudLifecycle<WifiConnectionLifecycle, Stack::CloudTransport> Cloud;
    enum : uint32_t { setupWindowMillis = 10UL * 60UL * 1000UL };

    struct Dedicated {
        Dedicated(Stack& stack, Installer& installer, DeviceInstanceIdStore& instances, uint32_t openedAt)
            : profile(stack.deviceInstance(), stack.platform().deviceAccessStore(), stack.random(), stack.clock()),
              endpoints(EndpointDescriptorTable()), runtime(hub, endpoints, span(frame), runtimeConfig()),
              link(stack.platform().bleLink()),
              transport(link, stack.clock(), stack.platform().noiseCrypto(), span(rx), span(tx), span(packet),
                  span(plain), transportConfig()),
              registration(stack.deviceInstance(), stack.platform().deviceKeys(), stack.random(), stack.clock(),
                  transport, sessions, BLINKER_BLE_MAX_SESSIONS,
                  security::InitialCloudKeySetupWindow(openedAt, setupWindowMillis)),
              endpoint(registration, installer, transport), binding{transport, registration, endpoint},
              retirement(runtime, link), access(stack.platform().deviceKeys()),
              receiver(retirement, access, stack.platform().wifiCredentials(), stack.clock()),
              supervisor(endpoint, retirement, receiver, instances, stack.platform().deviceKeys(),
                  stack.clock(), openedAt, setupWindowMillis) {}

        Result start() {
            ble::ModeProfile advertised;
            Result result = profile.make(transport.sessionRevision(), advertised);
            // This direction requires a real enrolled source discoverable by
            // its authorized Presence selector, not a MAC or open fallback.
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
        security::InitialCloudKeyRegistrationEndpoint endpoint;
        const PreCloudDirectBinding binding;
        Esp32PreCloudDirectRetirement retirement;
        security::InitialCloudWifiAccessSource access;
        Esp32PreCloudWifiOnlyReceiver receiver;
        PreCloudWifiHandoffSupervisor supervisor;
    };

public:
    Esp32PreCloudWifiProduct()
        : stack_(platform_, platform_.deviceKeys()), installer_(stack_, *this),
          instances_(platform_.deviceInstanceBlob()), cloud_(stack_.wifiLifecycle(), stack_.cloudTransport()) {}
    ~Esp32PreCloudWifiProduct() override {
        stop();
        if (dedicated_ != nullptr) std::terminate(); // Native owner still holds callbacks.
    }
    Esp32PreCloudWifiProduct(const Esp32PreCloudWifiProduct&) = delete;
    Esp32PreCloudWifiProduct& operator=(const Esp32PreCloudWifiProduct&) = delete;

    Result attach(Client& client) override {
        if (used_ || (client_ != nullptr && client_ != &client)) return Result::failure(ErrorCode::StateConflict);
        client_ = &client; // No Client transport or begin in the private phase.
        return Result::success();
    }

    Result start() override {
        if (client_ == nullptr) return Result::failure(ErrorCode::NotConfigured);
        if (used_) return Result::failure(ErrorCode::StateConflict);
        used_ = true;
        const uint32_t openedAt = stack_.clock().monotonicMillis();
        installer_.provisioning();

        // Open only the original stores. The candidate requires an existing
        // physical identity, so do not let stack.open() create a new one.
        Result result = platform_.begin();
        platformOpened_ = result.ok();
        DeviceInstanceId expected;
        if (result) result = instances_.load(expected);
        if (result) result = checkBleOnlyRoot();
        if (result) result = stack_.open();
        if (result && !sameDeviceInstanceId(expected, stack_.deviceInstance()))
            result = Result::failure(ErrorCode::StateConflict);
        if (!result) return fail(result.code());

        dedicated_ = new (&storage_) Dedicated(stack_, installer_, instances_, openedAt);
        result = dedicated_->start();
        if (!result) return fail(result.code());
        phase_ = Phase::Private;
        return Result::success();
    }

    void poll(uint32_t budget) override {
        if (stopping_) { finishStop(); return; }
        if (phase_ == Phase::Cloud) { cloud_.poll(budget); return; }
        if (phase_ != Phase::Private) return;
        // Expiry cancels radio independently of a busy durable-root gate.
        if (!dedicated_->supervisor.windowLive()) { (void)fail(ErrorCode::StateConflict); return; }

        // The held original task-root owner covers fresh root reads and the
        // one WiFi journal write. Release it before the Direct callback: the
        // existing installer must acquire its own original write fence there.
        if (!installer_.enterMaintenance()) return;
        const Result handoff = dedicated_->supervisor.advance();
        Result wifi = Result::failure(ErrorCode::WouldBlock);
        if (handoff) wifi = dedicated_->receiver.poll();
        installer_.leaveMaintenance();

        if (!handoff) {
            if (handoff.code() != ErrorCode::WouldBlock) { (void)fail(handoff.code()); return; }
            if (!dedicated_->supervisor.waitingForDirectHandoff()) return; // Never poll/refresh a retiring Host.
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
    Result checkBleOnlyRoot() {
        DeviceKey key;
        const Result loaded = platform_.deviceKeys().load(key);
        clearDeviceKey(key);
        if (loaded) return Result::failure(ErrorCode::StateConflict);
        if (loaded.code() != ErrorCode::NotFound) return loaded;
        DeviceAccessAdminSelector admin;
        return platform_.deviceKeys().loadAdminSelector(admin);
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
        if (!installer_.enterMaintenance()) return;
        destroyDedicated();
        if (stack_.initialized()) stack_.end();
        else if (platformOpened_) platform_.end();
        platformOpened_ = false;
        installer_.leaveMaintenance();
        stopping_ = false;
    }
    void lockFault(ErrorCode error) {
        error_ = error;
        phase_ = Phase::Fault;
        stopping_ = false;
        locked_ = true; // Preserve native callback storage; no restart or blind retry.
    }
    void destroyDedicated() {
        if (dedicated_ != nullptr) {
            dedicated_->~Dedicated();
            dedicated_ = nullptr;
        }
    }

    Esp32WifiBlePlatform platform_;
    Stack stack_;
    Installer installer_;
    DeviceInstanceIdStore instances_;
    Cloud cloud_;
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
