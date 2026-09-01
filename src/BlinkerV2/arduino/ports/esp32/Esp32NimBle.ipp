#include <esp_err.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_sm.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <os/os_mbuf.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include <string.h>

namespace blinker {

inline Esp32NimBleLink::Esp32NimBleLink(
    const Esp32NimBleLinkConfig& config)
    : config_(config),
      profile_(ble::makeDirectModeProfile()),
      serviceUuid_(),
      receiveUuid_(),
      transmitUuid_(),
      characteristics_(),
      services_(),
      modeServiceData_(),
      session_(),
      packetReceiver_(nullptr),
      packetContext_(nullptr),
      connectedHandler_(nullptr),
      disconnectedHandler_(nullptr),
      sessionContext_(nullptr),
      packets_(),
      packetHead_(0U),
      packetTail_(0U),
      packetCount_(0U),
      ownAddressType_(0U),
      receiveHandle_(0U),
      transmitHandle_(0U),
      connectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingConnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingDisconnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingSubscribeHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingSecurityHandle_(BLE_HS_CONN_HANDLE_NONE),
      nextSessionId_(1U),
      connectedAtMillis_(0U),
      pendingHostError_(ErrorCode::Ok),
      pendingNotifyEnabled_(false),
      pendingEncrypted_(false),
      pendingBonded_(false),
      connectPending_(false),
      disconnectPending_(false),
      subscribePending_(false),
      securityPending_(false),
      hostResultPending_(false),
      hostReady_(false),
      initialized_(false),
      stopping_(false),
      sessionAnnounced_(false),
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok),
      lock_(portMUX_INITIALIZER_UNLOCKED) {
    initializeUuids();
}

inline Esp32NimBleLink*& Esp32NimBleLink::activeLink() {
    static Esp32NimBleLink* active = nullptr;
    return active;
}

inline Result Esp32NimBleLink::start() {
    if (state_ != BleLinkState::Stopped || activeLink() != nullptr ||
        !esp32_nimble_detail::claim(this)) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        esp32_nimble_detail::release(this);
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    uint8_t encodedBytes[ble::kModeServiceDataSize] = {};
    ByteView encoded;
    Result result = ble::encodeModeServiceData(
        profile_,
        MutableByteSpan(encodedBytes, sizeof(encodedBytes)),
        encoded);
    if (!result) {
        esp32_nimble_detail::release(this);
        lastError_ = result.code();
        state_ = BleLinkState::Error;
        return result;
    }
    memcpy(modeServiceData_, ble::kServiceUuidLittleEndian, 16U);
    memcpy(modeServiceData_ + 16U, encoded.data, encoded.size);

    portENTER_CRITICAL(&lock_);
    resetRuntimeLocked();
    stopping_ = false;
    portEXIT_CRITICAL(&lock_);
    initializeGatt();
    state_ = BleLinkState::Starting;
    lastError_ = ErrorCode::Ok;
    activeLink() = this;

    const esp_err_t initError = nimble_port_init();
    if (initError != ESP_OK) {
        activeLink() = nullptr;
        esp32_nimble_detail::release(this);
        const ErrorCode error = initError == ESP_ERR_NO_MEM
                                    ? ErrorCode::CapacityExceeded
                                    : ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        lastError_ = error;
        return Result::failure(error);
    }
    initialized_ = true;

    ble_hs_cfg.reset_cb = &Esp32NimBleLink::onHostReset;
    ble_hs_cfg.sync_cb = &Esp32NimBleLink::onHostSync;
    ble_hs_cfg.store_status_cb = nullptr;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 0U;
    ble_hs_cfg.sm_mitm = 0U;
    ble_hs_cfg.sm_sc = 0U;
    ble_hs_cfg.sm_sc_only = 0U;
    ble_hs_cfg.sm_our_key_dist = 0U;
    ble_hs_cfg.sm_their_key_dist = 0U;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    if (ble_svc_gap_device_name_set(config_.deviceName) != 0 ||
        ble_gatts_count_cfg(services_) != 0 ||
        ble_gatts_add_svcs(services_) != 0) {
        const ErrorCode error = ErrorCode::ProtocolError;
        stop();
        lastError_ = error;
        state_ = BleLinkState::Error;
        return Result::failure(error);
    }
    nimble_port_freertos_init(&Esp32NimBleLink::hostTask);

    // Match the previous Arduino adapter contract: start() succeeds only after
    // the Host has synchronized and the complete advertising payload is live.
    const uint32_t startedAt = millis();
    while (static_cast<uint32_t>(millis() - startedAt) < 2000U) {
        bool ready = false;
        bool completed = false;
        ErrorCode error = ErrorCode::Ok;
        portENTER_CRITICAL(&lock_);
        completed = hostResultPending_;
        ready = hostReady_;
        error = pendingHostError_;
        portEXIT_CRITICAL(&lock_);
        if (completed) {
            if (ready) {
                state_ = BleLinkState::Ready;
                return Result::success();
            }
            stop();
            lastError_ = error == ErrorCode::Ok
                             ? ErrorCode::ProtocolError
                             : error;
            state_ = BleLinkState::Error;
            return Result::failure(lastError_);
        }
        delay(1U);
    }

    stop();
    lastError_ = ErrorCode::NotConnected;
    state_ = BleLinkState::Error;
    return Result::failure(lastError_);
}

inline void Esp32NimBleLink::stop() {
    portENTER_CRITICAL(&lock_);
    stopping_ = true;
    const uint16_t handle = connectionHandle_ != BLE_HS_CONN_HANDLE_NONE
                                ? connectionHandle_
                                : pendingConnectHandle_;
    portEXIT_CRITICAL(&lock_);

    if (initialized_) {
        (void)ble_gap_adv_stop();
        if (handle != BLE_HS_CONN_HANDLE_NONE) {
            (void)ble_gap_terminate(handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        if (nimble_port_stop() == 0) {
            (void)nimble_port_deinit();
        }
        initialized_ = false;
    }
    if (activeLink() == this) activeLink() = nullptr;
    esp32_nimble_detail::release(this);

    session_ = BleSessionInfo();
    connectedAtMillis_ = 0U;
    receiveHandle_ = 0U;
    transmitHandle_ = 0U;
    portENTER_CRITICAL(&lock_);
    resetRuntimeLocked();
    stopping_ = false;
    portEXIT_CRITICAL(&lock_);
    state_ = BleLinkState::Stopped;
}

inline void Esp32NimBleLink::poll(uint32_t) {
    if (state_ != BleLinkState::Ready) return;

    bool hostResult = false;
    bool hostReady = true;
    ErrorCode hostError = ErrorCode::Ok;
    portENTER_CRITICAL(&lock_);
    hostResult = hostResultPending_;
    hostReady = hostReady_;
    hostError = pendingHostError_;
    hostResultPending_ = false;
    portEXIT_CRITICAL(&lock_);
    if (hostResult && !hostReady) {
        lastError_ = hostError == ErrorCode::Ok
                         ? ErrorCode::ProtocolError
                         : hostError;
        state_ = BleLinkState::Error;
        return;
    }

    processPendingEvents();
    if (session_.connected && !sessionAnnounced_ &&
        static_cast<uint32_t>(millis() - connectedAtMillis_) >=
            config_.sessionReadyTimeoutMillis &&
        connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
        (void)ble_gap_terminate(
            connectionHandle_, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }
    drainPackets();
}

inline BleLinkState Esp32NimBleLink::state() const { return state_; }
inline ErrorCode Esp32NimBleLink::lastError() const { return lastError_; }
inline size_t Esp32NimBleLink::sessionCount() const {
    return sessionAnnounced_ ? 1U : 0U;
}

inline Result Esp32NimBleLink::sessionAt(
    size_t index,
    BleSessionInfo& session) const {
    if (index != 0U || !sessionAnnounced_) {
        return Result::failure(ErrorCode::NotFound);
    }
    session = session_;
    return Result::success();
}

inline Result Esp32NimBleLink::sendPacket(
    uint32_t sessionId,
    ByteView packet) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        !session_.notifyEnabled ||
        connectionHandle_ == BLE_HS_CONN_HANDLE_NONE || transmitHandle_ == 0U) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.empty() ||
        packet.size > session_.maxPacketSize ||
        packet.size > BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    os_mbuf* buffer = ble_hs_mbuf_from_flat(
        packet.data, static_cast<uint16_t>(packet.size));
    if (buffer == nullptr) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    return ble_gatts_notify_custom(
               connectionHandle_, transmitHandle_, buffer) == 0
               ? Result::success()
               : Result::failure(ErrorCode::WouldBlock);
}

inline Result Esp32NimBleLink::disconnectSession(uint32_t sessionId) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
        return Result::failure(ErrorCode::NotFound);
    }
    return ble_gap_terminate(
               connectionHandle_, BLE_ERR_REM_USER_CONN_TERM) == 0
               ? Result::success()
               : Result::failure(ErrorCode::WouldBlock);
}

inline void Esp32NimBleLink::setPacketReceiver(
    BlePacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

inline void Esp32NimBleLink::setSessionHandlers(
    BleSessionHandler connected,
    BleSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

inline Result Esp32NimBleLink::configureBleProfile(
    const ble::ModeProfile& profile) {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    const Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    profile_ = profile;
    return Result::success();
}

inline void Esp32NimBleLink::hostTask(void*) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

inline void Esp32NimBleLink::onHostReset(int) {
    Esp32NimBleLink* self = activeLink();
    if (self == nullptr) return;
    portENTER_CRITICAL(&self->lock_);
    self->hostReady_ = false;
    portEXIT_CRITICAL(&self->lock_);
}

inline void Esp32NimBleLink::onHostSync() {
    Esp32NimBleLink* self = activeLink();
    if (self == nullptr) return;
    const Result result = self->startAdvertising();
    self->markHostResult(result ? ErrorCode::Ok : result.code());
}

inline int Esp32NimBleLink::onGapEvent(
    ble_gap_event* event,
    void* context) {
    Esp32NimBleLink* self = static_cast<Esp32NimBleLink*>(context);
    return self != nullptr && event != nullptr
               ? self->handleGapEvent(*event)
               : 0;
}

inline int Esp32NimBleLink::onGattAccess(
    uint16_t connectionHandle,
    uint16_t attributeHandle,
    ble_gatt_access_ctxt* context,
    void* owner) {
    Esp32NimBleLink* self = static_cast<Esp32NimBleLink*>(owner);
    return self != nullptr && context != nullptr
               ? self->handleGattAccess(
                     connectionHandle, attributeHandle, *context)
               : BLE_ATT_ERR_UNLIKELY;
}

inline void Esp32NimBleLink::initializeUuids() {
    serviceUuid_.u.type = BLE_UUID_TYPE_128;
    memcpy(
        serviceUuid_.value,
        ble::kServiceUuidLittleEndian,
        sizeof(serviceUuid_.value));
    receiveUuid_ = serviceUuid_;
    transmitUuid_ = serviceUuid_;
    receiveUuid_.value[12] = 0x02U;
    transmitUuid_.value[12] = 0x03U;
}

inline void Esp32NimBleLink::initializeGatt() {
    memset(characteristics_, 0, sizeof(characteristics_));
    memset(services_, 0, sizeof(services_));
    receiveHandle_ = 0U;
    transmitHandle_ = 0U;

    characteristics_[0].uuid = &receiveUuid_.u;
    characteristics_[0].access_cb = &Esp32NimBleLink::onGattAccess;
    characteristics_[0].arg = this;
    characteristics_[0].flags = BLE_GATT_CHR_F_WRITE |
                                BLE_GATT_CHR_F_WRITE_NO_RSP;
    characteristics_[0].val_handle = &receiveHandle_;

    characteristics_[1].uuid = &transmitUuid_.u;
    characteristics_[1].access_cb = &Esp32NimBleLink::onGattAccess;
    characteristics_[1].arg = this;
    characteristics_[1].flags = BLE_GATT_CHR_F_READ |
                                BLE_GATT_CHR_F_NOTIFY;
    characteristics_[1].val_handle = &transmitHandle_;

    services_[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    services_[0].uuid = &serviceUuid_.u;
    services_[0].characteristics = characteristics_;
}

inline Result Esp32NimBleLink::startAdvertising() {
    if (ble_hs_util_ensure_addr(0) != 0 ||
        ble_hs_id_infer_auto(0, &ownAddressType_) != 0) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    ble_hs_adv_fields advertisement = {};
    advertisement.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    advertisement.uuids128 = &serviceUuid_;
    advertisement.num_uuids128 = 1U;
    advertisement.uuids128_is_complete = 1U;
    advertisement.name = reinterpret_cast<const uint8_t*>(config_.deviceName);
    advertisement.name_len = static_cast<uint8_t>(strlen(config_.deviceName));
    advertisement.name_is_complete = 1U;
    if (ble_gap_adv_set_fields(&advertisement) != 0) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    ble_hs_adv_fields scanResponse = {};
    scanResponse.svc_data_uuid128 = modeServiceData_;
    scanResponse.svc_data_uuid128_len =
        static_cast<uint8_t>(sizeof(modeServiceData_));
    if (ble_gap_adv_rsp_set_fields(&scanResponse) != 0) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (ble_gap_adv_active()) return Result::success();

    ble_gap_adv_params parameters = {};
    parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    return ble_gap_adv_start(
               ownAddressType_,
               nullptr,
               BLE_HS_FOREVER,
               &parameters,
               &Esp32NimBleLink::onGapEvent,
               this) == 0
               ? Result::success()
               : Result::failure(ErrorCode::ProtocolError);
}

inline void Esp32NimBleLink::markHostResult(ErrorCode error) {
    portENTER_CRITICAL(&lock_);
    hostReady_ = error == ErrorCode::Ok;
    pendingHostError_ = error;
    hostResultPending_ = true;
    portEXIT_CRITICAL(&lock_);
}

inline int Esp32NimBleLink::handleGapEvent(const ble_gap_event& event) {
    if (event.type == BLE_GAP_EVENT_CONNECT) {
        if (event.connect.status != 0) {
            const Result result = startAdvertising();
            if (!result) markHostResult(result.code());
            return 0;
        }

        const uint16_t handle = event.connect.conn_handle;
        bool reject = false;
        portENTER_CRITICAL(&lock_);
        if ((connectionHandle_ != BLE_HS_CONN_HANDLE_NONE &&
             connectionHandle_ != handle) ||
            (connectPending_ && pendingConnectHandle_ != handle)) {
            reject = true;
        } else {
            pendingConnectHandle_ = handle;
            connectPending_ = true;
        }
        portEXIT_CRITICAL(&lock_);
        if (reject) {
            (void)ble_gap_terminate(handle, BLE_ERR_REM_USER_CONN_TERM);
            return 0;
        }

        ble_gap_conn_desc description = {};
        if (ble_gap_conn_find(handle, &description) == 0 &&
            description.sec_state.encrypted) {
            portENTER_CRITICAL(&lock_);
            pendingSecurityHandle_ = handle;
            pendingEncrypted_ = true;
            pendingBonded_ = description.sec_state.bonded;
            securityPending_ = true;
            portEXIT_CRITICAL(&lock_);
        }
        return 0;
    }

    if (event.type == BLE_GAP_EVENT_DISCONNECT) {
        const uint16_t handle = event.disconnect.conn.conn_handle;
        bool restart = false;
        portENTER_CRITICAL(&lock_);
        if (connectionHandle_ == handle ||
            (connectPending_ && pendingConnectHandle_ == handle)) {
            pendingDisconnectHandle_ = handle;
            disconnectPending_ = true;
            clearPacketQueueLocked();
        }
        restart = !stopping_;
        portEXIT_CRITICAL(&lock_);
        if (restart) {
            const Result result = startAdvertising();
            if (!result) markHostResult(result.code());
        }
        return 0;
    }

    if (event.type == BLE_GAP_EVENT_ENC_CHANGE) {
        ble_gap_conn_desc description = {};
        const uint16_t handle = event.enc_change.conn_handle;
        if (event.enc_change.status != 0 ||
            ble_gap_conn_find(handle, &description) != 0) {
            return 0;
        }
        portENTER_CRITICAL(&lock_);
        pendingSecurityHandle_ = handle;
        pendingEncrypted_ = description.sec_state.encrypted;
        pendingBonded_ = description.sec_state.bonded;
        securityPending_ = true;
        portEXIT_CRITICAL(&lock_);
        return 0;
    }

    if (event.type == BLE_GAP_EVENT_SUBSCRIBE &&
        event.subscribe.attr_handle == transmitHandle_) {
        portENTER_CRITICAL(&lock_);
        pendingSubscribeHandle_ = event.subscribe.conn_handle;
        pendingNotifyEnabled_ = event.subscribe.cur_notify != 0U;
        subscribePending_ = true;
        portEXIT_CRITICAL(&lock_);
        return 0;
    }

    return 0;
}

inline int Esp32NimBleLink::handleGattAccess(
    uint16_t connectionHandle,
    uint16_t attributeHandle,
    ble_gatt_access_ctxt& context) {
    if (attributeHandle == receiveHandle_ &&
        context.op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        const uint16_t size = OS_MBUF_PKTLEN(context.om);
        if (size == 0U || size > BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        uint8_t packet[BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE] = {};
        uint16_t written = 0U;
        if (ble_hs_mbuf_to_flat(
                context.om, packet, sizeof(packet), &written) != 0 ||
            written != size) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        return queuePacket(connectionHandle, packet, written)
                   ? 0
                   : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    if (attributeHandle == transmitHandle_ &&
        context.op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

inline bool Esp32NimBleLink::queuePacket(
    uint16_t connectionHandle,
    const uint8_t* data,
    size_t size) {
    bool queued = false;
    portENTER_CRITICAL(&lock_);
    const bool accepted =
        connectionHandle_ == connectionHandle ||
        (connectPending_ && pendingConnectHandle_ == connectionHandle);
    if (accepted && packetCount_ < BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH) {
        QueuedPacket& packet = packets_[packetTail_];
        packet.connectionHandle = connectionHandle;
        packet.size = static_cast<uint16_t>(size);
        memcpy(packet.data, data, size);
        packetTail_ = static_cast<uint8_t>(
            (packetTail_ + 1U) % BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH);
        ++packetCount_;
        queued = true;
    }
    portEXIT_CRITICAL(&lock_);
    return queued;
}

inline void Esp32NimBleLink::processPendingEvents() {
    bool connect = false;
    bool disconnect = false;
    bool subscribe = false;
    bool security = false;
    bool notifyEnabled = false;
    bool encrypted = false;
    bool bonded = false;
    uint16_t connectHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t disconnectHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t subscribeHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t securityHandle = BLE_HS_CONN_HANDLE_NONE;

    portENTER_CRITICAL(&lock_);
    connect = connectPending_;
    disconnect = disconnectPending_;
    subscribe = subscribePending_;
    security = securityPending_;
    notifyEnabled = pendingNotifyEnabled_;
    encrypted = pendingEncrypted_;
    bonded = pendingBonded_;
    connectHandle = pendingConnectHandle_;
    disconnectHandle = pendingDisconnectHandle_;
    subscribeHandle = pendingSubscribeHandle_;
    securityHandle = pendingSecurityHandle_;
    connectPending_ = false;
    disconnectPending_ = false;
    subscribePending_ = false;
    securityPending_ = false;
    portEXIT_CRITICAL(&lock_);

    if (disconnect && connectionHandle_ == disconnectHandle &&
        session_.connected) {
        const BleSessionInfo oldSession = session_;
        session_ = BleSessionInfo();
        connectedAtMillis_ = 0U;
        portENTER_CRITICAL(&lock_);
        connectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
        portEXIT_CRITICAL(&lock_);
        if (sessionAnnounced_ && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, oldSession);
        }
        sessionAnnounced_ = false;
    }

    const bool connectWasAlsoDisconnected =
        disconnect && connectHandle == disconnectHandle;
    if (connect && !connectWasAlsoDisconnected && !session_.connected) {
        portENTER_CRITICAL(&lock_);
        connectionHandle_ = connectHandle;
        portEXIT_CRITICAL(&lock_);
        session_ = BleSessionInfo();
        session_.sessionId = nextSessionId();
        session_.maxPacketSize = BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE;
        session_.connected = true;
        connectedAtMillis_ = millis();
        if (subscribe && subscribeHandle == connectHandle) {
            session_.notifyEnabled = notifyEnabled;
        }
    }

    if (subscribe && session_.connected &&
        subscribeHandle == connectionHandle_) {
        session_.notifyEnabled = notifyEnabled;
    }
    if (security && session_.connected &&
        securityHandle == connectionHandle_) {
        session_.encrypted = encrypted;
        session_.bonded = bonded;
    }

    if (!sessionAnnounced_ && session_.connected && session_.notifyEnabled) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }

}

inline void Esp32NimBleLink::drainPackets() {
    uint8_t delivered = 0U;
    while (delivered < config_.maxRxPacketsPerPoll) {
        QueuedPacket packet;
        bool available = false;
        portENTER_CRITICAL(&lock_);
        if (packetCount_ != 0U) {
            packet = packets_[packetHead_];
            packetHead_ = static_cast<uint8_t>(
                (packetHead_ + 1U) % BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH);
            --packetCount_;
            available = true;
        }
        portEXIT_CRITICAL(&lock_);
        if (!available) return;
        ++delivered;
        if (sessionAnnounced_ &&
            packet.connectionHandle == connectionHandle_ &&
            packetReceiver_ != nullptr) {
            packetReceiver_(
                packetContext_,
                session_,
                ByteView(packet.data, packet.size));
        }
    }
}

inline void Esp32NimBleLink::clearPacketQueueLocked() {
    packetHead_ = 0U;
    packetTail_ = 0U;
    packetCount_ = 0U;
}

inline void Esp32NimBleLink::resetRuntimeLocked() {
    clearPacketQueueLocked();
    connectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingConnectHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingDisconnectHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingSubscribeHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingSecurityHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingHostError_ = ErrorCode::Ok;
    pendingNotifyEnabled_ = false;
    pendingEncrypted_ = false;
    pendingBonded_ = false;
    connectPending_ = false;
    disconnectPending_ = false;
    subscribePending_ = false;
    securityPending_ = false;
    hostResultPending_ = false;
    hostReady_ = false;
    sessionAnnounced_ = false;
}

inline uint32_t Esp32NimBleLink::nextSessionId() {
    const uint32_t result = nextSessionId_;
    ++nextSessionId_;
    if (nextSessionId_ == 0U) ++nextSessionId_;
    return result;
}

inline bool Esp32NimBleLink::validConfig() const {
    return config_.deviceName != nullptr && config_.deviceName[0] != '\0' &&
           strlen(config_.deviceName) <= ble::kLegacyLocalNameMaxSize &&
           config_.sessionReadyTimeoutMillis != 0U &&
           config_.maxRxPacketsPerPoll != 0U;
}

} // namespace blinker
