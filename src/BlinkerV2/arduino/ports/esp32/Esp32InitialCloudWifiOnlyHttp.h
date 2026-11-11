#ifndef BLINKER_V2_ESP32_INITIAL_CLOUD_WIFI_ONLY_HTTP_H
#define BLINKER_V2_ESP32_INITIAL_CLOUD_WIFI_ONLY_HTTP_H

#include <sdkconfig.h>
#if !defined(ARDUINO_ARCH_ESP32) || !CONFIG_IDF_TARGET_ESP32S3 || \
    !CONFIG_ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1 || !CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI
#error "Initial Cloud WiFi-only HTTP is an ESP32-S3 Security1 candidate"
#endif

#include <assert.h>
#include <stdlib.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <network_provisioning/network_config.h>
#include <protocomm.h>
#include <protocomm_httpd.h>
#include <protocomm_security1.h>

#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/provisioning/InitialCloudWifiConfigWire.h>
#include <BlinkerV2/provisioning/InitialCloudWifiOnly.h>

namespace blinker {

// Candidate transport only: the cooperative owner must already have retired
// native Direct and started a private, exclusive SoftAP with a separate WPA2+
// secret. No stock manager, native STA Apply or default product wiring here.
// Keep this object and mailbox alive until a successful end(), including if
// native HTTP shutdown fails. Only the cooperative owner calls public methods.
class Esp32InitialCloudWifiOnlyHttp {
public:
    explicit Esp32InitialCloudWifiOnlyHttp(InitialCloudWifiOnly& mailbox)
        : mailbox_(mailbox), mutex_(xSemaphoreCreateMutexStatic(&mutexStorage_)) {}
    ~Esp32InitialCloudWifiOnlyHttp() { assert(pc_ == nullptr); }
    Esp32InitialCloudWifiOnlyHttp(const Esp32InitialCloudWifiOnlyHttp&) = delete;
    Esp32InitialCloudWifiOnlyHttp& operator=(const Esp32InitialCloudWifiOnlyHttp&) = delete;

    Result beginOnExclusiveSoftAp(ByteView pop) {
        if (attempted_) return Result::failure(ErrorCode::StateConflict);
        if (pop.data == nullptr || pop.size < 16U || pop.size > sizeof(pop_))
            return Result::failure(ErrorCode::InvalidArgument);
        for (size_t i = 0U; i < pop.size; ++i)
            if (pop.data[i] < 0x21U || pop.data[i] > 0x7EU)
                return Result::failure(ErrorCode::InvalidArgument);
        attempted_ = true;
        pc_ = protocomm_new();
        if (pc_ == nullptr) return Result::failure(ErrorCode::CapacityExceeded);

        protocomm_httpd_config_t config = {};
        config.data.config.port = 80U;
        config.data.config.stack_size = 4096U;
        config.data.config.task_priority = tskIDLE_PRIORITY + 5U;
        esp_err_t result = protocomm_httpd_start(pc_, &config);
        if (result != ESP_OK) {
            // The SDK claims the singleton only when start succeeds.
            protocomm_delete(pc_);
            pc_ = nullptr;
            return nativeResult(result);
        }
        memcpy(pop_, pop.data, pop.size);
        security_.data = pop_;
        security_.len = static_cast<uint16_t>(pop.size);
        result = protocomm_set_version(pc_, "proto-ver",
            "{\"prov\":{\"ver\":\"v1.1\",\"sec_ver\":1,\"cap\":[]}}");
        if (result == ESP_OK) {
            securityOps_ = protocomm_security1;
            securityOps_.security_req_handler = &authorizeSession;
            result = protocomm_set_security(pc_, "prov-session", &securityOps_, &security_);
        }
        if (result == ESP_OK)
            result = protocomm_add_endpoint(pc_, "prov-config", &request, this);
        if (result != ESP_OK) {
            const Result closed = end();
            return closed ? nativeResult(result) : closed;
        }
        activeHttpOwner().store(this);
        { Lock lock(mutex_); accepting_ = true; }
        return Result::success();
    }

    bool take(WifiCredentialProfile& output) {
        Lock lock(mutex_);
        return mailbox_.take(output);
    }
    void finish(Result result) {
        Lock lock(mutex_);
        mailbox_.finish(result);
    }

    Result end() {
        { Lock lock(mutex_); accepting_ = false; mailbox_.cancel(); }
        attempted_ = true;
        if (pc_ == nullptr) return Result::success();
        const esp_err_t stopped = protocomm_httpd_stop(pc_);
        if (stopped != ESP_OK) return nativeResult(stopped);
        auto* owner = this;
        (void)activeHttpOwner().compare_exchange_strong(owner, nullptr);
        protocomm_delete(pc_);
        pc_ = nullptr;
        security_ = {};
        secureZero(MutableByteSpan(pop_, sizeof(pop_)));
        return Result::success();
    }
    bool shutdownComplete() const { return pc_ == nullptr; }

private:
    class Lock {
    public:
        explicit Lock(SemaphoreHandle_t mutex) : mutex_(mutex) {
            (void)xSemaphoreTake(mutex_, portMAX_DELAY);
        }
        ~Lock() { (void)xSemaphoreGive(mutex_); }
    private:
        SemaphoreHandle_t mutex_;
    };
    struct Request {
        Esp32InitialCloudWifiOnlyHttp& owner;
        uint32_t session;
    };

    static Result nativeResult(esp_err_t result) {
        return result == ESP_ERR_NO_MEM ? Result::failure(ErrorCode::CapacityExceeded) :
            Result::failure(ErrorCode::InternalError);
    }
    static size_t textLength(const char* value, size_t capacity) {
        size_t length = 0U;
        while (length < capacity && value[length] != '\0') ++length;
        return length;
    }
    static Request& scope(network_prov_ctx_t** ctx) {
        return *reinterpret_cast<Request*>(*ctx);
    }
    static std::atomic<Esp32InitialCloudWifiOnlyHttp*>& activeHttpOwner() {
        // The SDK HTTP transport already has exactly one live owner.
        static std::atomic<Esp32InitialCloudWifiOnlyHttp*> owner{nullptr};
        return owner;
    }
    static esp_err_t authorizeSession(protocomm_security_handle_t handle, const void* params,
        uint32_t session, const uint8_t* input, ssize_t size, uint8_t** output, ssize_t* written,
        void* context) {
        auto* owner = activeHttpOwner().load();
        if (owner == nullptr || context != owner->pc_) return ESP_ERR_INVALID_STATE;
        {
            Lock lock(owner->mutex_);
            // Once Set is staged the existing secure cookie may continue
            // across sockets, but no renegotiation/new crypto context may
            // acquire permission, even if the same numeric ID recurs.
            if (!owner->accepting_ || owner->sessionSealed_) return ESP_ERR_INVALID_STATE;
        }
        return protocomm_security1.security_req_handler(handle, params, session, input, size,
            output, written, context);
    }
    static esp_err_t request(uint32_t session, const uint8_t* input, ssize_t size,
        uint8_t** output, ssize_t* written, void* context) {
        // Runs in the SDK HTTP task after Security1 unprotects prov-config.
        if (context == nullptr || input == nullptr || size <= 0 || size > 160 ||
            output == nullptr || written == nullptr) return ESP_ERR_INVALID_ARG;
        *output = nullptr;
        *written = 0;
        if (!validateInitialCloudWifiConfigWire(ByteView(input, static_cast<size_t>(size))))
            return ESP_ERR_INVALID_ARG;
        Request scoped{*static_cast<Esp32InitialCloudWifiOnlyHttp*>(context), session};
        network_prov_config_handlers_t handlers = {};
        handlers.wifi_set_config_handler = &set;
        handlers.wifi_apply_config_handler = &apply;
        handlers.wifi_get_status_handler = &status;
        handlers.ctx = reinterpret_cast<network_prov_ctx_t*>(&scoped);
        const esp_err_t result = network_prov_config_data_handler(session, input, size, output, written, &handlers);
        if (result == ESP_OK && (*output == nullptr || *written <= 0)) {
            // The native App may treat an empty protobuf as default success.
            // Never send success without the SDK's typed response envelope.
            free(*output);
            *output = nullptr;
            *written = 0;
            return ESP_FAIL;
        }
        return result;
    }
    static esp_err_t set(const network_prov_config_set_wifi_data_t* input, network_prov_ctx_t** ctx) {
        // The original App supplies SSID/password, not BSSID/channel. Reject
        // unsupported targeting instead of silently connecting to a different AP.
        if (input == nullptr || input->channel != 0U) return ESP_ERR_INVALID_ARG;
        for (char byte : input->bssid) if (byte != 0) return ESP_ERR_INVALID_ARG;
        const size_t ssid = textLength(input->ssid, sizeof(input->ssid));
        const size_t password = textLength(input->password, sizeof(input->password));
        WifiNetworkConfig candidate;
        candidate.ssid = ByteView(reinterpret_cast<const uint8_t*>(input->ssid), ssid);
        candidate.credential = ByteView(reinterpret_cast<const uint8_t*>(input->password), password);
        candidate.authentication = password == 0U ? WifiAuthentication::Open : WifiAuthentication::Wpa2Personal;
        Request& item = scope(ctx);
        Lock lock(item.owner.mutex_);
        if (!item.owner.accepting_ || !item.owner.mailbox_.stage(item.session, candidate)) return ESP_FAIL;
        item.owner.sessionSealed_ = true;
        return ESP_OK;
    }
    static esp_err_t apply(network_prov_ctx_t** ctx) {
        Request& item = scope(ctx);
        Lock lock(item.owner.mutex_);
        return item.owner.accepting_ && item.owner.mailbox_.apply(item.session) ? ESP_OK : ESP_FAIL;
    }
    static esp_err_t status(network_prov_config_get_wifi_data_t* output, network_prov_ctx_t** ctx) {
        if (output == nullptr) return ESP_ERR_INVALID_ARG;
        Request& item = scope(ctx);
        Lock lock(item.owner.mutex_);
        const auto state = item.owner.mailbox_.status(item.session);
        if (!item.owner.accepting_ || (state != InitialCloudWifiOnlyState::Pending &&
            state != InitialCloudWifiOnlyState::Processing && state != InitialCloudWifiOnlyState::Committed))
            return ESP_FAIL;
        // Even a committed journal is not STA association. This isolated
        // transport does not synthesize Connected or a Wi-Fi reject reason.
        output->wifi_state = NETWORK_PROV_WIFI_STA_CONNECTING;
        return ESP_OK;
    }

    InitialCloudWifiOnly& mailbox_;
    StaticSemaphore_t mutexStorage_;
    SemaphoreHandle_t mutex_;
    protocomm_t* pc_ = nullptr;
    uint8_t pop_[64U] = {};
    protocomm_security1_params_t security_ = {};
    protocomm_security_t securityOps_ = {};
    bool attempted_ = false;
    bool accepting_ = false;
    bool sessionSealed_ = false;
};

} // namespace blinker
#endif
