#ifndef BLINKER_V2_ARDUINO_ESP32_NIMBLE_CENTRAL_HOST_H
#define BLINKER_V2_ARDUINO_ESP32_NIMBLE_CENTRAL_HOST_H

#include <Arduino.h>
#include <BlinkerV2/core/Result.h>
#include "Esp32NimBleRuntime.h"
#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <host/ble_gap.h>
#include <host/ble_hs.h>
#include <host/ble_hs_id.h>
#include <host/ble_sm.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <stdlib.h>

namespace blinker {

// Product-owned SDK lifetime and bounded connection slots. Only scan/initiate
// is exclusive; connected Ports keep their independent slots and callbacks.
// Lifecycle/admission methods run on the product loop; SDK callbacks may run
// concurrently and only enter through Callback or the reset/sync latches.
class Esp32NimBleCentralHost {
    struct Port {
        void* owner = nullptr;
        void (*reset)(void*) = nullptr;
        uint32_t token = 0U;
        uint16_t callbacks = 0U;
        bool active = false;
    };
public:
    // Respect a smaller SDK pool without requiring a particular core version.
    enum : uint8_t { capacity = CONFIG_BT_NIMBLE_MAX_CONNECTIONS < 2 ? CONFIG_BT_NIMBLE_MAX_CONNECTIONS : 2 };
    static_assert(capacity > 0U, "Central requires at least one SDK connection slot");
    explicit Esp32NimBleCentralHost(uint32_t readyTimeoutMillis = 2000U)
        : ports_(), initiating_(nullptr), timeout_(readyTimeoutMillis), initialized_(false),
          stopped_(false), ready_(false), error_(ErrorCode::Ok), addressType_(0U), lock_(portMUX_INITIALIZER_UNLOCKED) {}
    ~Esp32NimBleCentralHost() {
        // Destroying an SDK owner with live borrowers/callbacks would be UAF.
        // Ordinary stop timeout returns WouldBlock; only invalid destruction
        // is fatal. The product declares this owner before all borrowed ports.
        if (!end()) abort();
    }

    Result attach(void* owner, void (*reset)(void*)) {
        if (owner == nullptr || reset == nullptr) return Result::failure(ErrorCode::InvalidArgument);
        Port* slot = nullptr;
        portENTER_CRITICAL(&lock_);
        slot = find(owner);
        if (slot == nullptr) for (auto& entry : ports_) if (entry.owner == nullptr) { slot = &entry; break; }
        portEXIT_CRITICAL(&lock_);
        if (slot == nullptr) return Result::failure(ErrorCode::CapacityExceeded);
        Result result = begin();
        if (!result) return result;
        portENTER_CRITICAL(&lock_);
        if (ready_) { slot->owner = owner; slot->reset = reset; }
        else result = Result::failure(ErrorCode::NotConnected);
        portEXIT_CRITICAL(&lock_);
        return result;
    }

    bool detach(void* owner) {
        portENTER_CRITICAL(&lock_);
        Port* port = find(owner);
        const bool released = port == nullptr || (!port->active && port->callbacks == 0U);
        if (released && port != nullptr) *port = Port();
        portEXIT_CRITICAL(&lock_);
        return released;
    }

    Result acquireActivity(void* owner, void*& context) {
        context = nullptr;
        portENTER_CRITICAL(&lock_);
        Port* port = find(owner);
        ErrorCode error = ErrorCode::Ok;
        if (port == nullptr || !ready_) error = ErrorCode::NotConfigured;
        else if (initiating_ != nullptr || port->active || port->callbacks != 0U) error = ErrorCode::WouldBlock;
        else if (sequence() == UINT32_MAX) error = ErrorCode::CapacityExceeded;
        else {
            initiating_ = owner;
            port->active = true;
            port->token = ++sequence();
            context = reinterpret_cast<void*>(static_cast<uintptr_t>(port->token));
        }
        portEXIT_CRITICAL(&lock_);
        return Result(error);
    }

    bool ownsActivity(void* owner) {
        portENTER_CRITICAL(&lock_);
        Port* port = find(owner);
        const bool owned = port != nullptr && port->active;
        portEXIT_CRITICAL(&lock_);
        return owned;
    }
    bool ownsInitiation(void* owner) {
        portENTER_CRITICAL(&lock_); const bool owned = initiating_ == owner; portEXIT_CRITICAL(&lock_);
        return owned;
    }
    // Called only after native CONNECT completion or a whole SDK reset. An
    // allocated ACL keeps its slot; it no longer owns the global initiator.
    void finishInitiation(void* owner) {
        portENTER_CRITICAL(&lock_);
        if (initiating_ == owner) initiating_ = nullptr;
        portEXIT_CRITICAL(&lock_);
    }
    bool releaseActivity(void* owner) {
        portENTER_CRITICAL(&lock_);
        Port* port = find(owner);
        if (port != nullptr) port->token = 0U; // Reject late SDK callbacks before reusing RAM.
        const bool released = port == nullptr || port->callbacks == 0U;
        if (released) {
            if (port != nullptr) port->active = false;
            if (initiating_ == owner) initiating_ = nullptr;
        }
        portEXIT_CRITICAL(&lock_);
        return released;
    }
    uint8_t addressType() const { return addressType_; }

    Result end() {
        // Explicit whole-product shutdown only. Never stop another Port to
        // make shutdown appear successful, or release ownership on SDK error.
        for (const auto& port : ports_) if (port.owner != nullptr) return Result::failure(ErrorCode::WouldBlock);
        if (!initialized_) return Result::success();
        if (initiating_ != nullptr) return Result::failure(ErrorCode::WouldBlock);
        if (!stopped_) {
            if (nimble_port_stop() != 0) return Result::failure(ErrorCode::WouldBlock);
            stopped_ = true;
        }
        if (nimble_port_deinit() != ESP_OK) return Result::failure(ErrorCode::ProtocolError);
        initialized_ = false; stopped_ = false; ready_ = false; error_ = ErrorCode::Ok;
        active() = nullptr; esp32_nimble_detail::release(this);
        return Result::success();
    }

    // SDK arg is a non-reused integer token, never an address of a Port. A
    // bounded in-flight borrow prevents stop/destruction while callbacks use
    // the selected owner. No per-attempt heap objects or callback queue.
    class Callback {
    public:
        explicit Callback(void* token) : host_(active()), port_(nullptr), owner_(nullptr) {
            if (host_ == nullptr || token == nullptr) return;
            const uintptr_t id = reinterpret_cast<uintptr_t>(token);
            portENTER_CRITICAL(&host_->lock_);
            for (auto& entry : host_->ports_) {
                if (entry.owner != nullptr && entry.token == id && entry.callbacks != UINT16_MAX) {
                    port_ = &entry; owner_ = entry.owner; ++entry.callbacks; break;
                }
            }
            portEXIT_CRITICAL(&host_->lock_);
        }
        ~Callback() {
            if (port_ == nullptr) return;
            portENTER_CRITICAL(&host_->lock_); --port_->callbacks; portEXIT_CRITICAL(&host_->lock_);
        }
        void* owner() const { return owner_; }
        Callback(const Callback&) = delete;
        Callback& operator=(const Callback&) = delete;
    private:
        Esp32NimBleCentralHost* host_;
        Port* port_;
        void* owner_;
    };

    Esp32NimBleCentralHost(const Esp32NimBleCentralHost&) = delete;
    Esp32NimBleCentralHost& operator=(const Esp32NimBleCentralHost&) = delete;
private:
    static Esp32NimBleCentralHost*& active() { static Esp32NimBleCentralHost* host = nullptr; return host; }
    static uint32_t& sequence() { static uint32_t value = 0U; return value; }
    Port* find(void* owner) { for (auto& port : ports_) if (port.owner == owner) return &port; return nullptr; }
    static void hostTask(void*) { nimble_port_run(); nimble_port_freertos_deinit(); }
    static void onReset(int) {
        Esp32NimBleCentralHost* host = active();
        if (host == nullptr) return;
        portENTER_CRITICAL(&host->lock_);
        host->ready_ = false; host->error_ = ErrorCode::ProtocolError;
        host->initiating_ = nullptr; // SDK reset destroys all native procedures/ACLs.
        // Callbacks only latch a Port error under its lock. They do not call
        // SDK or reenter this owner; detach waits for this short notification.
        for (auto& port : host->ports_) if (port.owner != nullptr) {
            port.token = 0U;
            port.reset(port.owner);
        }
        portEXIT_CRITICAL(&host->lock_);
    }
    static void onSync() {
        Esp32NimBleCentralHost* host = active();
        if (host == nullptr) return;
        uint8_t address = 0U;
        const bool ready = ble_hs_util_ensure_addr(0) == 0 && ble_hs_id_infer_auto(0, &address) == 0;
        portENTER_CRITICAL(&host->lock_);
        host->addressType_ = address; host->ready_ = ready;
        host->error_ = ready ? ErrorCode::Ok : ErrorCode::NotConfigured;
        portEXIT_CRITICAL(&host->lock_);
    }
    Result begin() {
        if (timeout_ == 0U) return Result::failure(ErrorCode::InvalidArgument);
        if (stopped_) return Result::failure(ErrorCode::NotConfigured);
        if (!initialized_) {
            if (!esp32_nimble_detail::claim(this)) return Result::failure(ErrorCode::AlreadyExists);
            active() = this;
            const esp_err_t result = nimble_port_init();
            if (result != ESP_OK) {
                active() = nullptr; esp32_nimble_detail::release(this);
                return Result::failure(result == ESP_ERR_NO_MEM ? ErrorCode::CapacityExceeded : ErrorCode::InternalError);
            }
            initialized_ = true;
            ble_hs_cfg.reset_cb = &onReset; ble_hs_cfg.sync_cb = &onSync;
            ble_hs_cfg.store_status_cb = nullptr; ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
            ble_hs_cfg.sm_bonding = 0U; ble_hs_cfg.sm_mitm = 0U; ble_hs_cfg.sm_sc = 0U; ble_hs_cfg.sm_sc_only = 0U;
            ble_hs_cfg.sm_our_key_dist = 0U; ble_hs_cfg.sm_their_key_dist = 0U;
            nimble_port_freertos_init(&hostTask);
        }
        const uint32_t started = millis();
        do {
            portENTER_CRITICAL(&lock_);
            const bool ready = ready_; const ErrorCode error = error_;
            portEXIT_CRITICAL(&lock_);
            if (ready) return Result::success();
            if (error != ErrorCode::Ok) return Result::failure(error);
            delay(1U);
        } while (static_cast<uint32_t>(millis() - started) < timeout_);
        return Result::failure(ErrorCode::NotConnected);
    }

    Port ports_[capacity];
    void* initiating_;
    uint32_t timeout_;
    bool initialized_, stopped_, ready_;
    ErrorCode error_;
    uint8_t addressType_;
    portMUX_TYPE lock_;
};

} // namespace blinker
#endif
