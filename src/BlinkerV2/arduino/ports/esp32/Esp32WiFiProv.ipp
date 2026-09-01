#if defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <network_provisioning/manager.h>

#if defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
#include <network_provisioning/scheme_softap.h>
#else
#include <network_provisioning/scheme_ble.h>
#endif

#include <stdlib.h>
#include <string.h>

#if defined(CONFIG_ESP_WIFI_REMOTE_ENABLED) && CONFIG_ESP_WIFI_REMOTE_ENABLED
#error "ESP32 WiFiProv requires native ESP32 Wi-Fi"
#endif

#if !defined(CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI) || \
    !CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI
#error "ESP32 Network Provisioning Wi-Fi support is disabled"
#endif

#if !defined(CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1) || \
    !CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1
#error "ESP32 Protocomm Security1 support is disabled"
#endif

namespace blinker {

#if defined(BLINKER_ESP32_PROVISIONING_SOFTAP)
inline Esp32WifiProvConfig esp32SoftApWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession,
    const char* serviceKey) {
    Esp32WifiProvConfig config;
    config.scheme = &network_prov_scheme_softap;
    config.serviceName = serviceName;
    config.proofOfPossession = proofOfPossession;
    config.serviceKey = serviceKey;
    return config;
}
#else
inline Esp32WifiProvConfig esp32BleWifiProvConfig(
    const char* serviceName,
    const char* proofOfPossession) {
    Esp32WifiProvConfig config;
    config.scheme = &network_prov_scheme_ble;
    config.serviceName = serviceName;
    config.proofOfPossession = proofOfPossession;
    return config;
}
#endif

namespace {

static const char kEndpointName[] = "blinker-config";

size_t boundedLength(const uint8_t* value, size_t capacity) {
    size_t length = 0U;
    while (length < capacity && value[length] != 0U) ++length;
    return length;
}

} // namespace

inline Esp32WifiProvAdapter::Esp32WifiProvAdapter(
    DeviceKeyProvisioningEndpoint& deviceKey,
    IWifiCredentialSink& wifiCredentials)
    : deviceKey_(deviceKey),
      wifiCredentials_(wifiCredentials),
      state_(static_cast<uint8_t>(Esp32WifiProvState::Stopped)),
      lastError_(static_cast<uint8_t>(ErrorCode::Ok)),
      keyInstalled_(false),
      credentialsCommitted_(false),
      wifiSucceeded_(false),
      ended_(false),
      managerInitialized_(false),
      serviceStarted_(false),
      stopRequested_(false) {}

inline Esp32WifiProvAdapter::~Esp32WifiProvAdapter() {
    end();
}

inline bool Esp32WifiProvAdapter::validText(
    const char* value,
    size_t minimum,
    size_t maximum) {
    if (value == nullptr || minimum > maximum) return false;
    size_t length = 0U;
    while (value[length] != '\0' && length <= maximum) ++length;
    return length >= minimum && length <= maximum;
}

inline ErrorCode Esp32WifiProvAdapter::mapPlatformError(int error) {
    if (error == ESP_ERR_INVALID_ARG) return ErrorCode::InvalidArgument;
    if (error == ESP_ERR_INVALID_STATE) return ErrorCode::StateConflict;
    if (error == ESP_ERR_NO_MEM) return ErrorCode::CapacityExceeded;
    if (error == ESP_ERR_NOT_SUPPORTED) {
        return ErrorCode::UnsupportedFeature;
    }
    return ErrorCode::InternalError;
}

inline void Esp32WifiProvAdapter::fail(ErrorCode error) {
    lastError_ = static_cast<uint8_t>(
        error == ErrorCode::Ok ? ErrorCode::InternalError : error);
    state_ = static_cast<uint8_t>(Esp32WifiProvState::Fault);
}

inline Result Esp32WifiProvAdapter::begin(
    const Esp32WifiProvConfig& config) {
    if (state() != Esp32WifiProvState::Stopped || managerInitialized_) {
        return Result::failure(ErrorCode::StateConflict);
    }
    if (!validText(config.serviceName, 1U, 31U) ||
        (config.proofOfPossession != nullptr &&
         !validText(config.proofOfPossession, 1U, 64U)) ||
        config.scheme == nullptr ||
        (config.serviceKey != nullptr &&
         !validText(config.serviceKey, 8U, 63U))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    deviceKey_.reset();
    keyInstalled_ = false;
    credentialsCommitted_ = false;
    wifiSucceeded_ = false;
    ended_ = false;
    serviceStarted_ = false;
    stopRequested_ = false;
    lastError_ = static_cast<uint8_t>(ErrorCode::Ok);

    // Must happen before the first low-level WiFi initialization. Blinker's
    // WifiCredentialStore remains the only durable network credential source.
    WiFi.persistent(false);
    (void)WiFi.STA.begin(false);

    network_prov_mgr_config_t manager = {};
    manager.scheme = *config.scheme;
    // Do not use a FREE_BLE/BTDM scheme handler. Those handlers release
    // controller memory irreversibly, while this product starts NimBLE Direct
    // after provisioning has fully stopped and de-initialized.
    manager.app_event_handler.event_cb = &Esp32WifiProvAdapter::onEvent;
    manager.app_event_handler.user_data = this;

    esp_err_t error = network_prov_mgr_init(manager);
    if (error != ESP_OK) {
        fail(mapPlatformError(error));
        return Result::failure(lastError());
    }
    managerInitialized_ = true;

    error = network_prov_mgr_endpoint_create(kEndpointName);
    if (error == ESP_OK) {
        error = network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_1,
            config.proofOfPossession,
            config.serviceName,
            config.serviceKey);
        serviceStarted_ = error == ESP_OK;
    }
    if (error == ESP_OK) {
        error = network_prov_mgr_endpoint_register(
            kEndpointName,
            &Esp32WifiProvAdapter::onRequest,
            this);
    }
    if (error != ESP_OK) {
        const ErrorCode mapped = mapPlatformError(error);
        shutdownManager();
        fail(mapped);
        return Result::failure(mapped);
    }

    state_ = static_cast<uint8_t>(Esp32WifiProvState::Running);
    return Result::success();
}

inline void Esp32WifiProvAdapter::onEvent(
    void* context,
    network_prov_cb_event_t event,
    void* eventData) {
    Esp32WifiProvAdapter* self =
        static_cast<Esp32WifiProvAdapter*>(context);
    if (self != nullptr) self->handleEvent(event, eventData);
}

inline void Esp32WifiProvAdapter::handleEvent(
    network_prov_cb_event_t event,
    void* eventData) {
    if (event == NETWORK_PROV_WIFI_CRED_RECV) {
        const Result result = commitCredentials(eventData);
        if (result) {
            credentialsCommitted_ = true;
        } else {
            fail(result.code());
        }
    } else if (event == NETWORK_PROV_WIFI_CRED_SUCCESS) {
        wifiSucceeded_ = true;
    } else if (event == NETWORK_PROV_END) {
        ended_ = true;
        serviceStarted_ = false;
    }
}

inline Result Esp32WifiProvAdapter::commitCredentials(
    const void* eventData) {
    if (eventData == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const wifi_sta_config_t& native =
        *static_cast<const wifi_sta_config_t*>(eventData);
    const size_t ssidSize = boundedLength(native.ssid, sizeof(native.ssid));
    const size_t credentialSize =
        boundedLength(native.password, sizeof(native.password));

    WifiNetworkConfig candidate;
    candidate.ssid = ByteView(native.ssid, ssidSize);
    candidate.credential = ByteView(native.password, credentialSize);
    candidate.hidden = false;
    if (credentialSize == 0U) {
        candidate.authentication = WifiAuthentication::Open;
    } else if (native.threshold.authmode == WIFI_AUTH_WPA3_PSK) {
        candidate.authentication = WifiAuthentication::Wpa3Personal;
    } else if (native.threshold.authmode == WIFI_AUTH_WPA2_WPA3_PSK) {
        candidate.authentication = WifiAuthentication::Wpa2Wpa3Personal;
    } else {
        candidate.authentication = WifiAuthentication::Wpa2Personal;
    }
    return wifiCredentials_.commit(candidate);
}

inline esp_err_t Esp32WifiProvAdapter::onRequest(
    uint32_t sessionId,
    const uint8_t* input,
    ssize_t inputSize,
    uint8_t** output,
    ssize_t* outputSize,
    void* context) {
    (void)sessionId;
    if (context == nullptr || output == nullptr || outputSize == nullptr ||
        inputSize < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *output = nullptr;
    *outputSize = 0;

    Esp32WifiProvAdapter& self =
        *static_cast<Esp32WifiProvAdapter*>(context);
    uint8_t response[kDeviceKeyProvisioningMaxResponseSize] = {};
    size_t written = 0U;
    const Result result = self.deviceKey_.handle(
        ByteView(input, static_cast<size_t>(inputSize)),
        MutableByteSpan(response, sizeof(response)),
        written);
    if (!result) return ESP_FAIL;

    uint8_t* allocated = static_cast<uint8_t*>(malloc(written));
    if (allocated == nullptr) return ESP_ERR_NO_MEM;
    memcpy(allocated, response, written);
    *output = allocated;
    *outputSize = static_cast<ssize_t>(written);
    if (written >= kDeviceKeyProvisioningStatusResponseSize &&
        (response[1] == static_cast<uint8_t>(
             DeviceKeyProvisioningOperation::Install) ||
         response[1] == static_cast<uint8_t>(
             DeviceKeyProvisioningOperation::Bootstrap)) &&
        response[2] == static_cast<uint8_t>(
            DeviceKeyProvisioningStatus::Success)) {
        self.keyInstalled_ = true;
    }
    return ESP_OK;
}

inline void Esp32WifiProvAdapter::poll() {
    if (state() == Esp32WifiProvState::Stopped ||
        state() == Esp32WifiProvState::Complete) {
        return;
    }
    if (state() == Esp32WifiProvState::Fault && !ended_ &&
        managerInitialized_ && !stopRequested_) {
        stopRequested_ = true;
        network_prov_mgr_stop_provisioning();
    }
    if (!ended_) return;

    if (!shutdownManager()) return;
    if (state() == Esp32WifiProvState::Fault) return;
    if (!keyInstalled_ || !credentialsCommitted_ || !wifiSucceeded_) {
        fail(ErrorCode::NotConfigured);
        return;
    }
    lastError_ = static_cast<uint8_t>(ErrorCode::Ok);
    state_ = static_cast<uint8_t>(Esp32WifiProvState::Complete);
}

inline void Esp32WifiProvAdapter::end() {
    if (!shutdownManager()) {
        fail(ErrorCode::WouldBlock);
        return;
    }
    deviceKey_.reset();
    keyInstalled_ = false;
    credentialsCommitted_ = false;
    wifiSucceeded_ = false;
    ended_ = false;
    stopRequested_ = false;
    lastError_ = static_cast<uint8_t>(ErrorCode::Ok);
    state_ = static_cast<uint8_t>(Esp32WifiProvState::Stopped);
}

inline bool Esp32WifiProvAdapter::shutdownManager() {
    if (!managerInitialized_) return true;
    if (serviceStarted_ && !ended_) {
        if (!stopRequested_) {
            stopRequested_ = true;
            network_prov_mgr_stop_provisioning();
        }
        // The bundled IDF component stops asynchronously. Its END callback is
        // the ownership boundary after which protocomm/NimBLE may be released.
        const uint32_t startedAt = millis();
        while (!ended_ &&
               static_cast<uint32_t>(millis() - startedAt) < 3000U) {
            delay(1U);
        }
        if (!ended_) return false;
    }
    network_prov_mgr_deinit();
    managerInitialized_ = false;
    serviceStarted_ = false;
    return true;
}

} // namespace blinker

#endif // ARDUINO_ARCH_ESP32
