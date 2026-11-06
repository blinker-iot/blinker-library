#ifndef BLINKER_V2_ESP32_PRECLOUDWIFIONLYRECEIVER_H
#define BLINKER_V2_ESP32_PRECLOUDWIFIONLYRECEIVER_H

#include "Esp32InitialCloudWifiOnlyHttp.h"
#include <BlinkerV2/interface/IProductLifecycle.h>
#include <BlinkerV2/runtime/PreCloudWifiHandoffSupervisor.h>
#include <BlinkerV2/security/InitialCloudWifiAccess.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <exception>

namespace blinker {

// Candidate-only receiver, operated by the original cooperative product
// loop. No other code may start a network interface during this phase.
// Keep it alive if a native close fails; an unknown close never enables STA.
class Esp32PreCloudWifiOnlyReceiver final : public IPreCloudWifiOnlyReceiver {
public:
    Esp32PreCloudWifiOnlyReceiver(IPreCloudDirectRetirement& direct,
        security::InitialCloudWifiAccessSource& access, WifiCredentialStore& journal, IClock& clock)
        : direct_(direct), access_(access), journal_(journal), clock_(clock), carrier_(mailbox_) {}
    ~Esp32PreCloudWifiOnlyReceiver() override {
        clearPop();
        if (!carrier_.shutdownComplete() || ap_.ownsResources()) std::terminate();
    }
    Esp32PreCloudWifiOnlyReceiver(const Esp32PreCloudWifiOnlyReceiver&) = delete;
    Esp32PreCloudWifiOnlyReceiver& operator=(const Esp32PreCloudWifiOnlyReceiver&) = delete;

    Result beginWifiOnly(const security::InitialCloudKeyHandoff& handoff,
        IInitialCloudWifiWriteGuard& guard) override {
        if (phase_ != Phase::Idle) return Result::failure(ErrorCode::StateConflict);
        phase_ = Phase::StartingAp;
        committedAtMillis_ = handoff.authority.committedAtMillis;
        if (!liveDirectRelease()) return finish(Result::failure(ErrorCode::StateConflict));
        security::InitialCloudWifiAccess temporary;
        Result result = access_.derive(handoff, temporary);
        if (result && !liveDirectRelease()) result = Result::failure(ErrorCode::StateConflict);
        if (result) result = ap_.begin(temporary);
        if (!result) return finish(result);
        memcpy(pop_, temporary.pop, sizeof(pop_));
        guard_ = &guard;
        return Result::success();
    }

    Result poll() {
        if (phase_ == Phase::Closed || phase_ == Phase::Locked) return final_;
        if (phase_ == Phase::Idle) return Result::failure(ErrorCode::NotConfigured);
        if (!liveDirectRelease() || !ap_.exclusive()) return finish(Result::failure(ErrorCode::StateConflict));
        if (phase_ == Phase::StartingAp) {
            const Result ready = ap_.ready();
            if (!ready) return ready.code() == ErrorCode::WouldBlock ? ready : finish(ready);
            const Result listening = carrier_.beginOnExclusiveSoftAp(ByteView(pop_, sizeof(pop_) - 1U));
            clearPop();
            if (!listening) return finish(listening);
            phase_ = Phase::Listening;
        }
        WifiCredentialProfile profile;
        if (!carrier_.take(profile)) return Result::failure(ErrorCode::WouldBlock);
        // The only durable write lives on this loop. A WouldBlock here is a
        // consumed attempt, not the wait result from an empty mailbox.
        const Result committed = commitInitialCloudWifiOnly(profile, *guard_, journal_);
        carrier_.finish(committed);
        return finish(committed);
    }

    // Cancels pending writes and closes the native resources in order. A
    // successful cleanup does not give cancelled work permission to start STA.
    Result end() {
        if (phase_ == Phase::Locked) return final_;
        if (phase_ != Phase::Closed) (void)finish(Result::failure(ErrorCode::StateConflict));
        else final_ = Result::failure(ErrorCode::StateConflict);
        return phase_ == Phase::Closed ? Result::success() : final_;
    }
    bool staMayStart() const {
        return phase_ == Phase::Closed && final_.ok() && !staAttempted_ && direct_.shutdownComplete() &&
            carrier_.shutdownComplete() && ap_.shutdownComplete();
    }
    bool terminal() const { return phase_ == Phase::Closed || phase_ == Phase::Locked; }
    // Starts the same STA/journal via the original product lifecycle, once.
    // Starting that lifecycle is not association or Cloud online.
    Result startOriginalProduct(IProductLifecycle& lifecycle) {
        if (!staMayStart()) return Result::failure(ErrorCode::StateConflict);
        staAttempted_ = true;
        return lifecycle.start();
    }

private:
    enum class Phase : uint8_t { Idle, StartingAp, Listening, Closed, Locked };

    // Unlike Arduino softAP(), this configures WPA2 before native start. It
    // cannot briefly advertise the SDK's unconfigured/default AP profile.
    class SoftAp {
    public:
        Result begin(const security::InitialCloudWifiAccess& temporary) {
            esp_err_t result = esp_netif_init();
            if (result != ESP_OK) return nativeFailure(result);
            result = esp_event_loop_create_default();
            if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) return nativeFailure(result);
            if (!shutdownComplete()) return Result::failure(ErrorCode::StateConflict);

            claimed_ = true;
            wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
            init.nvs_enable = 0;
            result = esp_wifi_init(&init);
            if (result != ESP_OK) return nativeFailure(result);
            ownsDriver_ = true;
            result = esp_wifi_set_storage(WIFI_STORAGE_RAM);
            if (result == ESP_OK) result = esp_wifi_set_mode(WIFI_MODE_AP);
            if (result != ESP_OK) return nativeFailure(result);

            wifi_config_t config = {};
            memcpy(config.ap.ssid, temporary.ssid, sizeof(temporary.ssid) - 1U);
            memcpy(config.ap.password, temporary.password, sizeof(temporary.password));
            config.ap.ssid_len = sizeof(temporary.ssid) - 1U;
            config.ap.channel = 1U;
            config.ap.authmode = WIFI_AUTH_WPA2_PSK;
            config.ap.pairwise_cipher = WIFI_CIPHER_TYPE_CCMP;
            config.ap.max_connection = 1U;
            config.ap.beacon_interval = 100U;
            result = esp_wifi_set_config(WIFI_IF_AP, &config);
            secureZero(MutableByteSpan(reinterpret_cast<uint8_t*>(&config), sizeof(config)));
            if (result != ESP_OK) return nativeFailure(result);

            // The pinned SDK convenience constructors assert on allocation
            // failure or swallow the attach result. Use their checked steps.
            esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_WIFI_AP();
            netif_ = esp_netif_new(&netifConfig);
            if (netif_ == nullptr) return Result::failure(ErrorCode::CapacityExceeded);
            result = esp_netif_attach_wifi_ap(netif_);
            if (result == ESP_OK) result = esp_wifi_set_default_wifi_ap_handlers();
            if (result != ESP_OK) return nativeFailure(result);
            startAttempted_ = true;
            result = esp_wifi_start();
            return result == ESP_OK ? Result::success() : nativeFailure(result);
        }

        bool exclusive() const {
            wifi_mode_t mode = WIFI_MODE_NULL;
            return ownsDriver_ && netif_ != nullptr && esp_wifi_get_mode(&mode) == ESP_OK &&
                mode == WIFI_MODE_AP && esp_netif_get_nr_of_ifs() == 1U &&
                esp_netif_get_handle_from_ifkey("WIFI_AP_DEF") == netif_;
        }
        Result ready() const {
            if (!exclusive()) return Result::failure(ErrorCode::StateConflict);
            if (!esp_netif_is_netif_up(netif_)) return Result::failure(ErrorCode::WouldBlock);
            esp_netif_dhcp_status_t dhcp;
            esp_netif_ip_info_t ip = {};
            const esp_err_t status = esp_netif_dhcps_get_status(netif_, &dhcp);
            if (status != ESP_OK) return nativeFailure(status);
            if (dhcp != ESP_NETIF_DHCP_STARTED) return Result::failure(ErrorCode::WouldBlock);
            if (esp_netif_get_ip_info(netif_, &ip) != ESP_OK || ip.ip.addr != ESP_IP4TOADDR(192, 168, 4, 1))
                return Result::failure(ErrorCode::StateConflict);
            return Result::success();
        }

        Result end() {
            if (!claimed_) return Result::success();
            if (startAttempted_) {
                const esp_err_t stopped = esp_wifi_stop();
                if (stopped != ESP_OK && stopped != ESP_ERR_WIFI_NOT_STARTED) return nativeFailure(stopped);
                startAttempted_ = false;
            }
            if (netif_ != nullptr) {
                const esp_err_t cleared = esp_wifi_clear_default_wifi_driver_and_handlers(netif_);
                if (cleared != ESP_OK) return nativeFailure(cleared);
                esp_netif_destroy(netif_);
                netif_ = nullptr;
            }
            if (ownsDriver_) {
                const esp_err_t stopped = esp_wifi_deinit();
                if (stopped != ESP_OK) return nativeFailure(stopped);
                ownsDriver_ = false;
            }
            return shutdownComplete() ? Result::success() : Result::failure(ErrorCode::StateConflict);
        }
        bool ownsResources() const { return ownsDriver_ || netif_ != nullptr; }
        bool shutdownComplete() const {
            wifi_mode_t mode = WIFI_MODE_NULL;
            return !ownsResources() && esp_wifi_get_mode(&mode) == ESP_ERR_WIFI_NOT_INIT &&
                esp_netif_get_nr_of_ifs() == 0U;
        }
    private:
        static Result nativeFailure(esp_err_t error) {
            return Result::failure(error == ESP_ERR_NO_MEM ? ErrorCode::CapacityExceeded : ErrorCode::InternalError);
        }
        esp_netif_t* netif_ = nullptr;
        bool claimed_ = false;
        bool ownsDriver_ = false;
        bool startAttempted_ = false;
    };

    bool liveDirectRelease() const {
        return direct_.shutdownComplete() && clock_.monotonicMillis() - committedAtMillis_ <
            security::kInitialCloudKeyRegistrationWindowMillis;
    }
    Result finish(Result outcome) {
        guard_ = nullptr;
        clearPop();
        const Result http = carrier_.end();
        const Result closed = http && carrier_.shutdownComplete()
            ? ap_.end() : Result::failure(http ? ErrorCode::StateConflict : http.code());
        phase_ = closed ? Phase::Closed : Phase::Locked;
        final_ = closed ? outcome : closed;
        return final_;
    }
    void clearPop() { secureZero(MutableByteSpan(pop_, sizeof(pop_))); }

    IPreCloudDirectRetirement& direct_;
    security::InitialCloudWifiAccessSource& access_;
    WifiCredentialStore& journal_;
    IClock& clock_;
    InitialCloudWifiOnly mailbox_;
    Esp32InitialCloudWifiOnlyHttp carrier_;
    SoftAp ap_;
    IInitialCloudWifiWriteGuard* guard_ = nullptr;
    uint8_t pop_[33U] = {};
    uint32_t committedAtMillis_ = 0U;
    Phase phase_ = Phase::Idle;
    Result final_ = Result::failure(ErrorCode::NotConfigured);
    bool staAttempted_ = false;
};

} // namespace blinker
#endif
