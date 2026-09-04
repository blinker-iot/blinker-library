#ifndef BLINKER_V2_ARDUINO_ESP32_WIFI_BLE_PRODUCT_H
#define BLINKER_V2_ARDUINO_ESP32_WIFI_BLE_PRODUCT_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "Esp32WiFiBLEProduct requires an ESP32 Arduino target"
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

#include "Esp32WiFiPlatform.h"
#include "../../internal/WiFiProduct.h"
#include "../../ports/esp32/Esp32Crypto.h"
#include "../../ports/esp32/Esp32NimBle.h"
#include "../../ports/esp32/Esp32WiFiProv.h"

#include <BlinkerV2/api/BlinkerFacade.h>
#include <BlinkerV2/arduino/config/Esp32ServerKeys.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/provisioning/ControllerControlCoordinator.h>
#include <BlinkerV2/provisioning/ControllerControlEndpoint.h>
#include <BlinkerV2/provisioning/ControllerGrantVerifier.h>
#include <BlinkerV2/runtime/BleDirectProfile.h>
#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>
#include <BlinkerV2/security/P256ServerKeyRingVerifier.h>
#include <BlinkerV2/transport/BleFrameTransport.h>

namespace blinker {
namespace integration {
namespace official_detail {

class Esp32WifiBlePlatform : public Esp32WifiNetworkPlatform {
public:
    enum : size_t {
        maximumBlePacketSize = BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE
    };

    Esp32WifiBlePlatform()
        : Esp32WifiNetworkPlatform(),
          accessBlob_(esp32BlobStorageConfig(
              "bl_v2_access",
              "root",
              DeviceAccessStore::serializedSize)),
          accessStore_(accessBlob_),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          ble_() {}

    Result begin() {
        Result result = Esp32WifiNetworkPlatform::begin();
        if (result) result = accessBlob_.begin();
        if (!result) end();
        return result;
    }

    void end() {
        ble_.stop();
        accessBlob_.end();
        Esp32WifiNetworkPlatform::end();
    }

    DeviceAccessStore& deviceKeys() { return accessStore_; }
    IDeviceAccessStore& deviceAccessStore() { return accessStore_; }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    Esp32NimBleLink& bleLink() { return ble_; }

private:
    Esp32NvsAtomicBlobStore accessBlob_;
    DeviceAccessStore accessStore_;
    Esp32MbedTlsCryptoProvider crypto_;
    P256ServerKeyRingVerifier signatureVerifier_;
    Esp32NimBleLink ble_;

    Esp32WifiBlePlatform(const Esp32WifiBlePlatform&);
    Esp32WifiBlePlatform& operator=(const Esp32WifiBlePlatform&);
};

template <typename Platform>
class DirectBleExtension {
public:
    DirectBleExtension(
        Platform& platform,
        const DeviceInstanceId& deviceInstance,
        IClock& clock,
        IRandom& random)
        : platform_(platform),
          access_(platform.deviceAccessStore()),
          directProfile_(
              deviceInstance,
              access_,
              random,
              clock),
          rx_(),
          tx_(),
          packet_(),
          transport_(
              platform.bleLink(),
              clock,
              platform.noiseCrypto(),
              MutableByteSpan(rx_, sizeof(rx_)),
              MutableByteSpan(tx_, sizeof(tx_)),
              MutableByteSpan(packet_, sizeof(packet_)),
              MutableByteSpan(plaintext_, sizeof(plaintext_)),
              transportConfig()),
          sessions_(),
          authorizer_(
              access_,
              random,
              transport_,
              sessions_,
              BLINKER_BLE_MAX_SESSIONS,
              security::ControllerAuthTransportPolicy::EstablishDirectSecure),
          grantVerifier_(platform.serverSignatureVerifier()),
          controlCoordinator_(
              deviceInstance,
              access_,
              access_,
              grantVerifier_,
              &access_),
          controlWorkspace_(),
          controlEndpoint_(
              controlCoordinator_,
              random,
              MutableByteSpan(
                  controlWorkspace_,
                  sizeof(controlWorkspace_))),
          client_(nullptr) {}

    Result prepare() {
        ble::ModeProfile profile;
        Result result = directProfile_.make(
            transport_.sessionRevision(), profile);
        if (result) {
            result = platform_.bleLink().configureBleProfile(profile);
        }
        return result;
    }

    Result poll() {
        if (!directProfile_.refreshDue(
                platform_.bleLink().sessionCount(),
                transport_.sessionRevision())) {
            return Result::success();
        }
        ble::ModeProfile profile;
        Result result = directProfile_.make(
            transport_.sessionRevision(), profile);
        if (result) {
            result = platform_.bleLink().refreshBleProfile(profile);
        }
        if (!result) {
            directProfile_.requestRefresh();
            if (result.code() == ErrorCode::StateConflict ||
                result.code() == ErrorCode::WouldBlock) {
                return Result::success();
            }
        }
        return result;
    }

    void resetProfile() { directProfile_.reset(); }

    Result attach(Client& client) {
        if (client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }

        Result result = client.addTransport(transport_);
        if (result) result = client.setAuthorizationProvider(&authorizer_);
        if (result) {
            result = client.setControllerControlEndpoint(&controlEndpoint_);
        }
        if (!result) {
            (void)client.setControllerControlEndpoint(nullptr);
            (void)client.setAuthorizationProvider(nullptr);
            (void)client.removeTransport(transport_);
            return result;
        }
        client_ = &client;
        return Result::success();
    }

    void detach(Client& client) {
        if (client_ != &client) return;
        (void)client.setControllerControlEndpoint(nullptr);
        (void)client.setAuthorizationProvider(nullptr);
        (void)client.removeTransport(transport_);
        client_ = nullptr;
    }

    uint16_t capabilities() const {
        return ProductCapabilityDirectBleData;
    }

private:
    static BleFrameTransportConfig transportConfig() {
        BleFrameTransportConfig config;
        config.maxFrameSize = BLINKER_DEVICE_FRAME_SIZE;
        return config;
    }

    Platform& platform_;
    IDeviceAccessStore& access_;
    BleDirectProfileProvider directProfile_;
    enum : size_t {
        directRecordSize =
            BLINKER_DEVICE_FRAME_SIZE + security::kDirectSecureOverhead
    };
    uint8_t rx_[directRecordSize * BLINKER_BLE_MAX_SESSIONS];
    uint8_t tx_[directRecordSize * BLINKER_BLE_MAX_TX_FRAMES];
    uint8_t packet_[Platform::maximumBlePacketSize];
    uint8_t plaintext_[BLINKER_DEVICE_FRAME_SIZE];
    BleFrameTransport transport_;
    security::ControllerAuthSession sessions_[BLINKER_BLE_MAX_SESSIONS];
    security::ControllerHmacSha256Authorizer authorizer_;
    ControllerGrantVerifier grantVerifier_;
    ControllerControlCoordinator controlCoordinator_;
    uint8_t controlWorkspace_[kControllerControlWorkspaceSize];
    ControllerControlEndpoint controlEndpoint_;
    Client* client_;

    DirectBleExtension(const DirectBleExtension&);
    DirectBleExtension& operator=(const DirectBleExtension&);
};

enum class Esp32WifiBleProductState : uint8_t {
    Stopped = 0U,
    Provisioning,
    Active,
    Fault
};

// One ESP32 product graph: BLE WiFiProv Security1 bootstrap, then WiFi cloud
// plus Direct BBP/2. Provisioning and Direct are sequential owners of the
// ESP-IDF NimBLE Host. SoftAP remains an explicit low-resource fallback.
// The ordinary education profile deliberately uses no Sketch setup secret and
// does not claim active-MITM resistance.
template <typename Platform>
class Esp32WifiBleProduct final : public IProductLifecycle {
public:
    Esp32WifiBleProduct()
        : platform_(),
          stack_(platform_, platform_.deviceKeys()),
          direct_(
              platform_,
              stack_.deviceInstance(),
              stack_.clock(),
              stack_.random()),
          endpoint_(
              stack_.deviceInstance(),
              platform_.deviceKeys(),
              &platform_.deviceAccessStore()),
          provisioner_(endpoint_, platform_.wifiCredentials()),
          cloud_(stack_.wifiLifecycle(), stack_.cloudTransport()),
          client_(nullptr),
          serviceName_(),
          provisioningStartedMs_(0U),
          state_(Esp32WifiBleProductState::Stopped),
          lastError_(ErrorCode::Ok) {}

    ~Esp32WifiBleProduct() override { stop(); }

    Result attach(Client& client) override {
        if (client_ != nullptr) {
            return client_ == &client
                       ? Result::success()
                       : Result::failure(ErrorCode::AlreadyExists);
        }

        Result result = client.setMonotonicClock(&stack_.clock());
        if (result) result = direct_.attach(client);
        if (result) result = cloud_.attach(client);
        if (!result) {
            direct_.detach(client);
            lastError_ = result.code();
            return result;
        }
        client_ = &client;
        return Result::success();
    }

    Result start() override {
        if (state_ != Esp32WifiBleProductState::Stopped) {
            return Result::failure(ErrorCode::AlreadyExists);
        }
        if (client_ == nullptr) {
            return failStart(ErrorCode::NotConfigured);
        }

        Result result = stack_.open();
        if (!result) return failStart(result.code());

        DeviceKey current;
        result = platform_.deviceKeys().load(current);
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
        const Esp32WifiProvConfig config = provisioningConfig(serviceName_);
        result = provisioner_.begin(config);
        if (!result) return failStart(result.code());

        provisioningStartedMs_ = stack_.clock().monotonicMillis();
        state_ = Esp32WifiBleProductState::Provisioning;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    void poll(uint32_t totalBudgetMicros) override {
        if (state_ == Esp32WifiBleProductState::Provisioning) {
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
        } else if (state_ == Esp32WifiBleProductState::Active) {
            cloud_.poll(totalBudgetMicros);
            const ProductLifecycleStatus current = cloud_.status();
            if (current.state == ProductLifecycleState::Fault) {
                enterFault(current.lastError);
            } else {
                const Result result = direct_.poll();
                if (!result) enterFault(result.code());
            }
        }
    }

    void stop() override {
        provisioner_.end();
        cloud_.stop();
        direct_.resetProfile();
        stack_.end();
        state_ = Esp32WifiBleProductState::Stopped;
        lastError_ = ErrorCode::Ok;
    }

    Result resetAccess() override {
        Result result = stack_.open();
        if (result) result = platform_.deviceKeys().eraseAccess();
        stack_.end();
        return result;
    }

    Result resetNetwork() override {
        Result result = stack_.open();
        if (result) result = platform_.wifiCredentials().clear();
        stack_.end();
        return result;
    }

    ProductLifecycleStatus status() const override {
        if (state_ == Esp32WifiBleProductState::Stopped) {
            return ProductLifecycleStatus();
        }
        if (state_ == Esp32WifiBleProductState::Provisioning) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Provisioning,
                lastError_,
                false);
        }
        if (state_ == Esp32WifiBleProductState::Fault) {
            return ProductLifecycleStatus(
                ProductLifecycleState::Fault,
                lastError_,
                false);
        }
        return cloud_.status();
    }

    ProductCapabilities capabilities() const override {
        return ProductCapabilities(static_cast<uint16_t>(
            ProductCapabilityCloudData |
            direct_.capabilities() |
            ProductCapabilityAccessReset |
            ProductCapabilityNetworkReset));
    }

private:
    enum : uint32_t {
        kProvisioningWindowMs = 10UL * 60UL * 1000UL
    };

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
        static const char prefix[] = "BLINKER_";
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
        Result result = direct_.prepare();
        if (result) result = stack_.begin();
        if (result) result = cloud_.start();
        if (!result) return failStart(result.code());
        state_ = Esp32WifiBleProductState::Active;
        lastError_ = ErrorCode::Ok;
        return Result::success();
    }

    Result failStart(ErrorCode error) {
        enterFault(error);
        return Result::failure(lastError_);
    }

    void enterFault(ErrorCode error) {
        provisioner_.end();
        cloud_.stop();
        direct_.resetProfile();
        stack_.end();
        lastError_ = error == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : error;
        state_ = Esp32WifiBleProductState::Fault;
    }

    Platform platform_;
    DeviceKeyWifiStack<Platform> stack_;
    DirectBleExtension<Platform> direct_;
    DeviceKeyProvisioningEndpoint endpoint_;
    Esp32WifiProvAdapter provisioner_;
    WifiCloudLifecycle cloud_;
    Client* client_;
    char serviceName_[32U];
    uint32_t provisioningStartedMs_;
    Esp32WifiBleProductState state_;
    ErrorCode lastError_;

    Esp32WifiBleProduct(const Esp32WifiBleProduct&);
    Esp32WifiBleProduct& operator=(const Esp32WifiBleProduct&);
};

typedef Esp32WifiBleProduct<Esp32WifiBlePlatform>
    Esp32OfficialWifiBleProduct;

inline Esp32OfficialWifiBleProduct& esp32WifiBleProduct() {
    static Esp32OfficialWifiBleProduct instance;
    return instance;
}

} // namespace official_detail

inline IProductLifecycle& lifecycle(facade_detail::WifiBleProfile) {
    return official_detail::esp32WifiBleProduct();
}

} // namespace integration
} // namespace blinker

#endif
