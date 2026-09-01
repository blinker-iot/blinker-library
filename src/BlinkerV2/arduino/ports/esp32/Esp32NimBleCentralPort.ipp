#include <esp_err.h>
#include <host/ble_att.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_hs_mbuf.h>
#include <host/ble_sm.h>
#include <host/util/util.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <os/os_mbuf.h>

#include <string.h>

namespace blinker {

inline Esp32NimBleCentralPort::Esp32NimBleCentralPort(
    const Esp32NimBleCentralPortConfig& config)
    : config_(config), serviceUuid_(), receiveUuid_(), transmitUuid_(),
      request_(), connection_(), packetReceiver_(nullptr),
      packetContext_(nullptr), connectedHandler_(nullptr),
      disconnectedHandler_(nullptr), connectionContext_(nullptr),
      candidates_(), packets_(), candidateHead_(0U), candidateTail_(0U),
      candidateCount_(0U), packetHead_(0U), packetTail_(0U),
      packetCount_(0U), ownAddressType_(0U),
      connectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingConnectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingDisconnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      serviceStartHandle_(0U), serviceEndHandle_(0U), receiveHandle_(0U),
      transmitHandle_(0U), cccdHandle_(0U), pendingConnectStatus_(0),
      pendingHostError_(ErrorCode::Ok),
      pendingDiscoveryError_(ErrorCode::Ok),
      hostResultPending_(false), connectEventPending_(false),
      disconnectEventPending_(false), discoveryResultPending_(false),
      securityEventPending_(false), pendingEncrypted_(false),
      pendingBonded_(false), cancelCompletionPending_(false),
      initialized_(false), stopping_(false),
      state_(BleCentralPortState::Stopped), lastError_(ErrorCode::Ok),
      lock_(portMUX_INITIALIZER_UNLOCKED) {
    initializeUuids();
}

inline Esp32NimBleCentralPort*& Esp32NimBleCentralPort::activePort() {
    static Esp32NimBleCentralPort* active = nullptr;
    return active;
}

inline bool Esp32NimBleCentralPort::validConfig() const {
    return config_.hostReadyTimeoutMillis != 0U &&
           config_.hostStopTimeoutMillis != 0U &&
           config_.nativeConnectTimeoutMillis != 0U &&
           config_.maxScanResultsPerPoll != 0U &&
           config_.maxRxPacketsPerPoll != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE >=
               ble::kMinimumPacketSize;
}

inline Result Esp32NimBleCentralPort::start() {
    if (state_ != BleCentralPortState::Stopped || activePort() != nullptr ||
        !esp32_nimble_detail::claim(this)) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        esp32_nimble_detail::release(this);
        state_ = BleCentralPortState::Error;
        lastError_ = ErrorCode::NotConfigured;
        return Result::failure(lastError_);
    }

    clearAttempt();
    stopping_ = false;
    lastError_ = ErrorCode::Ok;
    activePort() = this;
    const esp_err_t nativeResult = nimble_port_init();
    if (nativeResult != ESP_OK) {
        activePort() = nullptr;
        esp32_nimble_detail::release(this);
        state_ = BleCentralPortState::Error;
        lastError_ = nativeResult == ESP_ERR_NO_MEM
                         ? ErrorCode::CapacityExceeded
                         : ErrorCode::InternalError;
        return Result::failure(lastError_);
    }
    initialized_ = true;

    ble_hs_cfg.reset_cb = &Esp32NimBleCentralPort::onHostReset;
    ble_hs_cfg.sync_cb = &Esp32NimBleCentralPort::onHostSync;
    ble_hs_cfg.store_status_cb = nullptr;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 0U;
    ble_hs_cfg.sm_mitm = 0U;
    ble_hs_cfg.sm_sc = 0U;
    ble_hs_cfg.sm_sc_only = 0U;
    ble_hs_cfg.sm_our_key_dist = 0U;
    ble_hs_cfg.sm_their_key_dist = 0U;
    nimble_port_freertos_init(&Esp32NimBleCentralPort::hostTask);

    const uint32_t startedAt = millis();
    while (static_cast<uint32_t>(millis() - startedAt) <
           config_.hostReadyTimeoutMillis) {
        bool completed = false;
        ErrorCode error = ErrorCode::Ok;
        portENTER_CRITICAL(&lock_);
        completed = hostResultPending_;
        error = pendingHostError_;
        portEXIT_CRITICAL(&lock_);
        if (completed) {
            if (error == ErrorCode::Ok) {
                portENTER_CRITICAL(&lock_);
                hostResultPending_ = false;
                portEXIT_CRITICAL(&lock_);
                state_ = BleCentralPortState::Idle;
                return Result::success();
            }
            stop();
            state_ = BleCentralPortState::Error;
            lastError_ = error;
            return Result::failure(error);
        }
        delay(1U);
    }

    stop();
    state_ = BleCentralPortState::Error;
    lastError_ = ErrorCode::NotConnected;
    return Result::failure(lastError_);
}

inline void Esp32NimBleCentralPort::stop() {
    stopping_ = true;
    if (initialized_) {
        if (ble_gap_disc_active()) (void)ble_gap_disc_cancel();
        if (state_ == BleCentralPortState::Connecting &&
            connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
            (void)ble_gap_conn_cancel();
        }
        if (connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
            (void)ble_gap_terminate(
                connectionHandle_, BLE_ERR_REM_USER_CONN_TERM);
        }

        // GAP termination is asynchronous. Calling nimble_port_stop() while
        // the native connection still exists leaves the host task alive and
        // makes the next cloud-session generation unable to claim NimBLE.
        const uint32_t startedAt = millis();
        bool idle = false;
        do {
            ble_gap_conn_desc connection = {};
            const bool connected =
                connectionHandle_ != BLE_HS_CONN_HANDLE_NONE &&
                ble_gap_conn_find(connectionHandle_, &connection) == 0;
            idle = !connected && !ble_gap_disc_active();
            if (!idle) delay(1U);
        } while (!idle &&
                 static_cast<uint32_t>(millis() - startedAt) <
                     config_.hostStopTimeoutMillis);

        if (!idle || nimble_port_stop() != 0) {
            stopping_ = false;
            state_ = BleCentralPortState::Error;
            lastError_ = idle
                             ? ErrorCode::ProtocolError
                             : ErrorCode::WouldBlock;
            return;
        }
        (void)nimble_port_deinit();
        initialized_ = false;
    }
    if (activePort() == this) activePort() = nullptr;
    esp32_nimble_detail::release(this);
    clearAttempt();
    stopping_ = false;
    state_ = BleCentralPortState::Stopped;
    lastError_ = ErrorCode::Ok;
}

inline Result Esp32NimBleCentralPort::startScan() {
    ble_gap_disc_params parameters = {};
    parameters.itvl = BLE_GAP_SCAN_FAST_INTERVAL_MIN;
    parameters.window = BLE_GAP_SCAN_FAST_WINDOW;
    parameters.filter_policy = 0U;
    parameters.limited = 0U;
    parameters.passive = 0U;
    parameters.filter_duplicates = 0U;
    parameters.disable_observer_mode = 0U;
    const int result = ble_gap_disc(
        ownAddressType_, BLE_HS_FOREVER, &parameters,
        &Esp32NimBleCentralPort::onGapEvent, this);
    return result == 0
               ? Result::success()
               : Result::failure(
                     result == BLE_HS_EBUSY
                         ? ErrorCode::WouldBlock
                         : ErrorCode::ProtocolError);
}

inline Result Esp32NimBleCentralPort::connect(
    const BleCentralConnectRequest& request) {
    if (state_ != BleCentralPortState::Idle) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (request.attemptId == 0U || request.matcher == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    clearAttempt();
    request_ = request;
    lastError_ = ErrorCode::Ok;
    Result result = startScan();
    if (!result) {
        clearAttempt();
        lastError_ = result.code();
        return result;
    }
    state_ = BleCentralPortState::Scanning;
    return Result::success();
}

inline Result Esp32NimBleCentralPort::cancel(uint32_t attemptId) {
    if (attemptId == 0U || request_.attemptId != attemptId ||
        state_ == BleCentralPortState::Stopped ||
        state_ == BleCentralPortState::Idle ||
        state_ == BleCentralPortState::Error ||
        state_ == BleCentralPortState::Disconnecting) {
        return Result::failure(ErrorCode::NotFound);
    }

    const BleCentralPortState previous = state_;
    state_ = BleCentralPortState::Disconnecting;
    int nativeResult = 0;
    if (previous == BleCentralPortState::Scanning) {
        nativeResult = ble_gap_disc_cancel();
        portENTER_CRITICAL(&lock_);
        cancelCompletionPending_ = true;
        portEXIT_CRITICAL(&lock_);
    } else if (connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
        nativeResult = ble_gap_conn_cancel();
        if (nativeResult == BLE_HS_EALREADY) {
            portENTER_CRITICAL(&lock_);
            cancelCompletionPending_ = true;
            portEXIT_CRITICAL(&lock_);
        }
    } else {
        nativeResult = ble_gap_terminate(
            connectionHandle_, BLE_ERR_REM_USER_CONN_TERM);
    }
    if (nativeResult != 0 && nativeResult != BLE_HS_EALREADY) {
        state_ = previous;
        return Result::failure(ErrorCode::WouldBlock);
    }
    return Result::success();
}

inline Result Esp32NimBleCentralPort::connection(
    BleCentralConnectionInfo& output) const {
    if (state_ != BleCentralPortState::Ready ||
        !connection_.connected || !connection_.notifyEnabled) {
        return Result::failure(ErrorCode::NotConnected);
    }
    output = connection_;
    return Result::success();
}

inline Result Esp32NimBleCentralPort::sendPacket(
    uint32_t attemptId,
    ByteView packet) {
    if (state_ != BleCentralPortState::Ready ||
        attemptId != connection_.attemptId ||
        connectionHandle_ == BLE_HS_CONN_HANDLE_NONE ||
        receiveHandle_ == 0U) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.empty() ||
        packet.size > connection_.maxPacketSize ||
        packet.size > BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const int result = ble_gattc_write_no_rsp_flat(
        connectionHandle_, receiveHandle_, packet.data,
        static_cast<uint16_t>(packet.size));
    if (result == 0) return Result::success();
    return Result::failure(
        result == BLE_HS_ENOTCONN ? ErrorCode::NotConnected
                                  : ErrorCode::WouldBlock);
}

inline void Esp32NimBleCentralPort::setPacketReceiver(
    BleCentralPacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

inline void Esp32NimBleCentralPort::setConnectionHandlers(
    BleCentralConnectionHandler connected,
    BleCentralConnectionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    connectionContext_ = context;
}

inline void Esp32NimBleCentralPort::poll(uint32_t) {
    if (state_ == BleCentralPortState::Stopped) return;
    processNativeEvents();
    if (state_ == BleCentralPortState::Scanning) {
        processScanCandidates();
    }
    if (state_ == BleCentralPortState::Ready) drainPackets();
}

inline void Esp32NimBleCentralPort::processNativeEvents() {
    bool host = false;
    bool connectEvent = false;
    bool disconnectEvent = false;
    bool discovery = false;
    bool security = false;
    bool cancelComplete = false;
    uint16_t connectHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t disconnectHandle = BLE_HS_CONN_HANDLE_NONE;
    int connectStatus = 0;
    ErrorCode hostError = ErrorCode::Ok;
    ErrorCode discoveryError = ErrorCode::Ok;
    bool encrypted = false;
    bool bonded = false;

    portENTER_CRITICAL(&lock_);
    host = hostResultPending_;
    connectEvent = connectEventPending_;
    disconnectEvent = disconnectEventPending_;
    discovery = discoveryResultPending_;
    security = securityEventPending_;
    cancelComplete = cancelCompletionPending_;
    connectHandle = pendingConnectionHandle_;
    disconnectHandle = pendingDisconnectHandle_;
    connectStatus = pendingConnectStatus_;
    hostError = pendingHostError_;
    discoveryError = pendingDiscoveryError_;
    encrypted = pendingEncrypted_;
    bonded = pendingBonded_;
    hostResultPending_ = false;
    connectEventPending_ = false;
    disconnectEventPending_ = false;
    discoveryResultPending_ = false;
    securityEventPending_ = false;
    cancelCompletionPending_ = false;
    portEXIT_CRITICAL(&lock_);

    if (host && hostError != ErrorCode::Ok) {
        lastError_ = hostError;
        state_ = BleCentralPortState::Error;
        return;
    }
    if (disconnectEvent &&
        (disconnectHandle == connectionHandle_ ||
         disconnectHandle == connectHandle)) {
        completeAttempt(lastError_);
        return;
    }
    if (cancelComplete && state_ == BleCentralPortState::Disconnecting) {
        completeAttempt(ErrorCode::Ok);
        return;
    }
    if (connectEvent) {
        if (connectStatus != 0 ||
            state_ == BleCentralPortState::Disconnecting) {
            completeAttempt(
                state_ == BleCentralPortState::Disconnecting
                    ? ErrorCode::Ok
                    : ErrorCode::NotConnected);
            return;
        }
        if (state_ != BleCentralPortState::Connecting ||
            connectHandle == BLE_HS_CONN_HANDLE_NONE) {
            if (connectHandle != BLE_HS_CONN_HANDLE_NONE) {
                (void)ble_gap_terminate(
                    connectHandle, BLE_ERR_REM_USER_CONN_TERM);
            }
            return;
        }
        connectionHandle_ = connectHandle;
        state_ = BleCentralPortState::Discovering;
        beginDiscovery(connectHandle);
    }
    if (security && connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
        connection_.encrypted = encrypted;
        connection_.bonded = bonded;
    }
    if (discovery && state_ == BleCentralPortState::Discovering) {
        if (discoveryError != ErrorCode::Ok) {
            lastError_ = discoveryError;
            state_ = BleCentralPortState::Disconnecting;
            if (ble_gap_terminate(
                    connectionHandle_, BLE_ERR_REM_USER_CONN_TERM) != 0) {
                completeAttempt(discoveryError);
            }
            return;
        }
        ble_gap_conn_desc description = {};
        if (ble_gap_conn_find(connectionHandle_, &description) == 0) {
            connection_.encrypted = description.sec_state.encrypted;
            connection_.bonded = description.sec_state.bonded;
        }
        connection_.attemptId = request_.attemptId;
        connection_.maxPacketSize =
            BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE;
        connection_.connected = true;
        connection_.notifyEnabled = true;
        state_ = BleCentralPortState::Ready;
        lastError_ = ErrorCode::Ok;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(connectionContext_, connection_);
        }
    }
}

inline void Esp32NimBleCentralPort::processScanCandidates() {
    uint8_t processed = 0U;
    while (processed < config_.maxScanResultsPerPoll) {
        ScanCandidate candidate;
        bool available = false;
        portENTER_CRITICAL(&lock_);
        if (candidateCount_ != 0U) {
            candidate = candidates_[candidateHead_];
            candidateHead_ = static_cast<uint8_t>(
                (candidateHead_ + 1U) %
                BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH);
            --candidateCount_;
            available = true;
        }
        portEXIT_CRITICAL(&lock_);
        if (!available) return;
        ++processed;

        BleCentralAdvertisement advertisement;
        advertisement.modeServiceData = ByteView(
            candidate.serviceData, sizeof(candidate.serviceData));
        advertisement.rssi = candidate.rssi;
        if (request_.matcher != nullptr &&
            request_.matcher(request_.matcherContext, advertisement)) {
            beginNativeConnect(candidate);
            return;
        }
    }
}

inline void Esp32NimBleCentralPort::beginNativeConnect(
    const ScanCandidate& candidate) {
    state_ = BleCentralPortState::Connecting;
    portENTER_CRITICAL(&lock_);
    candidateHead_ = 0U;
    candidateTail_ = 0U;
    candidateCount_ = 0U;
    portEXIT_CRITICAL(&lock_);
    const int cancelResult = ble_gap_disc_cancel();
    if (cancelResult != 0 && cancelResult != BLE_HS_EALREADY) {
        completeAttempt(ErrorCode::ProtocolError);
        return;
    }
    const int result = ble_gap_connect(
        ownAddressType_, &candidate.address,
        static_cast<int32_t>(config_.nativeConnectTimeoutMillis),
        nullptr, &Esp32NimBleCentralPort::onGapEvent, this);
    if (result != 0) {
        completeAttempt(
            result == BLE_HS_EBUSY ? ErrorCode::WouldBlock
                                   : ErrorCode::NotConnected);
    }
}

inline void Esp32NimBleCentralPort::beginDiscovery(
    uint16_t connectionHandle) {
    serviceStartHandle_ = 0U;
    serviceEndHandle_ = 0U;
    receiveHandle_ = 0U;
    transmitHandle_ = 0U;
    cccdHandle_ = 0U;
    const int result = ble_gattc_disc_svc_by_uuid(
        connectionHandle, &serviceUuid_.u,
        &Esp32NimBleCentralPort::onService, this);
    if (result != 0) markDiscoveryResult(ErrorCode::ProtocolError);
}

inline void Esp32NimBleCentralPort::completeAttempt(ErrorCode error) {
    BleCentralConnectionInfo previous = connection_;
    if (previous.attemptId == 0U) previous.attemptId = request_.attemptId;
    clearAttempt();
    state_ = BleCentralPortState::Idle;
    lastError_ = error;
    if (previous.attemptId != 0U && disconnectedHandler_ != nullptr) {
        disconnectedHandler_(connectionContext_, previous);
    }
}

inline void Esp32NimBleCentralPort::drainPackets() {
    uint8_t delivered = 0U;
    while (delivered < config_.maxRxPacketsPerPoll) {
        QueuedPacket packet;
        bool available = false;
        portENTER_CRITICAL(&lock_);
        if (packetCount_ != 0U) {
            packet = packets_[packetHead_];
            packetHead_ = static_cast<uint8_t>(
                (packetHead_ + 1U) %
                BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH);
            --packetCount_;
            available = true;
        }
        portEXIT_CRITICAL(&lock_);
        if (!available) return;
        ++delivered;
        if (packet.connectionHandle == connectionHandle_ &&
            packetReceiver_ != nullptr) {
            packetReceiver_(
                packetContext_, connection_,
                ByteView(packet.data, packet.size));
        }
    }
}

inline void Esp32NimBleCentralPort::hostTask(void*) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

inline void Esp32NimBleCentralPort::onHostReset(int) {
    Esp32NimBleCentralPort* self = activePort();
    if (self != nullptr) self->markHostResult(ErrorCode::ProtocolError);
}

inline void Esp32NimBleCentralPort::onHostSync() {
    Esp32NimBleCentralPort* self = activePort();
    if (self == nullptr) return;
    const bool ready = ble_hs_util_ensure_addr(0) == 0 &&
                       ble_hs_id_infer_auto(
                           0, &self->ownAddressType_) == 0;
    self->markHostResult(
        ready ? ErrorCode::Ok : ErrorCode::NotConfigured);
}

inline int Esp32NimBleCentralPort::onGapEvent(
    ble_gap_event* event,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    return self != nullptr && event != nullptr
               ? self->handleGapEvent(*event)
               : 0;
}

inline int Esp32NimBleCentralPort::handleGapEvent(
    const ble_gap_event& event) {
    if (event.type == BLE_GAP_EVENT_DISC) {
        if (state_ == BleCentralPortState::Scanning) {
            (void)queueCandidate(event.disc);
        }
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_CONNECT) {
        portENTER_CRITICAL(&lock_);
        pendingConnectStatus_ = event.connect.status;
        pendingConnectionHandle_ = event.connect.conn_handle;
        connectEventPending_ = true;
        portEXIT_CRITICAL(&lock_);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_DISCONNECT) {
        portENTER_CRITICAL(&lock_);
        pendingDisconnectHandle_ = event.disconnect.conn.conn_handle;
        disconnectEventPending_ = true;
        packetHead_ = 0U;
        packetTail_ = 0U;
        packetCount_ = 0U;
        portEXIT_CRITICAL(&lock_);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_NOTIFY_RX &&
        event.notify_rx.conn_handle == connectionHandle_ &&
        event.notify_rx.attr_handle == transmitHandle_) {
        (void)queuePacket(
            event.notify_rx.conn_handle, event.notify_rx.om);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_ENC_CHANGE &&
        event.enc_change.status == 0) {
        ble_gap_conn_desc description = {};
        if (ble_gap_conn_find(
                event.enc_change.conn_handle, &description) == 0) {
            portENTER_CRITICAL(&lock_);
            pendingEncrypted_ = description.sec_state.encrypted;
            pendingBonded_ = description.sec_state.bonded;
            securityEventPending_ = true;
            portEXIT_CRITICAL(&lock_);
        }
    }
    return 0;
}

inline bool Esp32NimBleCentralPort::queueCandidate(
    const ble_gap_disc_desc& discovery) {
    ble_hs_adv_fields fields = {};
    if (discovery.data == nullptr ||
        ble_hs_adv_parse_fields(
            &fields, discovery.data, discovery.length_data) != 0 ||
        fields.svc_data_uuid128 == nullptr ||
        fields.svc_data_uuid128_len !=
            16U + ble::kModeServiceDataSize ||
        memcmp(
            fields.svc_data_uuid128,
            ble::kServiceUuidLittleEndian,
            16U) != 0) {
        return false;
    }

    bool queued = false;
    portENTER_CRITICAL(&lock_);
    if (candidateCount_ <
        BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH) {
        ScanCandidate& candidate = candidates_[candidateTail_];
        candidate.address = discovery.addr;
        candidate.rssi = discovery.rssi;
        memcpy(
            candidate.serviceData,
            fields.svc_data_uuid128 + 16U,
            sizeof(candidate.serviceData));
        candidateTail_ = static_cast<uint8_t>(
            (candidateTail_ + 1U) %
            BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH);
        ++candidateCount_;
        queued = true;
    }
    portEXIT_CRITICAL(&lock_);
    return queued;
}

inline bool Esp32NimBleCentralPort::queuePacket(
    uint16_t connectionHandle,
    const os_mbuf* buffer) {
    if (buffer == nullptr) return false;
    const uint16_t size = OS_MBUF_PKTLEN(buffer);
    if (size == 0U ||
        size > BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE) {
        return false;
    }
    uint8_t bytes[BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE] = {};
    uint16_t written = 0U;
    if (ble_hs_mbuf_to_flat(
            const_cast<os_mbuf*>(buffer), bytes, sizeof(bytes),
            &written) != 0 ||
        written != size) {
        return false;
    }
    bool queued = false;
    portENTER_CRITICAL(&lock_);
    if (packetCount_ < BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH) {
        QueuedPacket& packet = packets_[packetTail_];
        packet.connectionHandle = connectionHandle;
        packet.size = static_cast<uint8_t>(written);
        memcpy(packet.data, bytes, written);
        packetTail_ = static_cast<uint8_t>(
            (packetTail_ + 1U) %
            BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH);
        ++packetCount_;
        queued = true;
    }
    portEXIT_CRITICAL(&lock_);
    return queued;
}

inline int Esp32NimBleCentralPort::onService(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    const ble_gatt_svc* service,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && service != nullptr) {
        if (self->serviceStartHandle_ != 0U) {
            self->markDiscoveryResult(ErrorCode::ProtocolError);
            return BLE_HS_EDONE;
        }
        self->serviceStartHandle_ = service->start_handle;
        self->serviceEndHandle_ = service->end_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE ||
        self->serviceStartHandle_ == 0U) {
        self->markDiscoveryResult(ErrorCode::NotFound);
        return 0;
    }
    const int result = ble_gattc_disc_chrs_by_uuid(
        connectionHandle, self->serviceStartHandle_,
        self->serviceEndHandle_, &self->receiveUuid_.u,
        &Esp32NimBleCentralPort::onReceiveCharacteristic, self);
    if (result != 0) {
        self->markDiscoveryResult(ErrorCode::ProtocolError);
    }
    return 0;
}

inline int Esp32NimBleCentralPort::onReceiveCharacteristic(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    const ble_gatt_chr* characteristic,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && characteristic != nullptr) {
        if (self->receiveHandle_ != 0U ||
            (characteristic->properties &
             BLE_GATT_CHR_PROP_WRITE_NO_RSP) == 0U) {
            self->markDiscoveryResult(ErrorCode::ProtocolError);
            return BLE_HS_EDONE;
        }
        self->receiveHandle_ = characteristic->val_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || self->receiveHandle_ == 0U) {
        self->markDiscoveryResult(ErrorCode::NotFound);
        return 0;
    }
    const int result = ble_gattc_disc_chrs_by_uuid(
        connectionHandle, self->serviceStartHandle_,
        self->serviceEndHandle_, &self->transmitUuid_.u,
        &Esp32NimBleCentralPort::onTransmitCharacteristic, self);
    if (result != 0) {
        self->markDiscoveryResult(ErrorCode::ProtocolError);
    }
    return 0;
}

inline int Esp32NimBleCentralPort::onTransmitCharacteristic(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    const ble_gatt_chr* characteristic,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && characteristic != nullptr) {
        if (self->transmitHandle_ != 0U ||
            (characteristic->properties & BLE_GATT_CHR_PROP_NOTIFY) == 0U) {
            self->markDiscoveryResult(ErrorCode::ProtocolError);
            return BLE_HS_EDONE;
        }
        self->transmitHandle_ = characteristic->val_handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || self->transmitHandle_ == 0U) {
        self->markDiscoveryResult(ErrorCode::NotFound);
        return 0;
    }
    const int result = ble_gattc_disc_all_dscs(
        connectionHandle, self->transmitHandle_,
        self->serviceEndHandle_,
        &Esp32NimBleCentralPort::onDescriptor, self);
    if (result != 0) {
        self->markDiscoveryResult(ErrorCode::ProtocolError);
    }
    return 0;
}

inline int Esp32NimBleCentralPort::onDescriptor(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    uint16_t characteristicValueHandle,
    const ble_gatt_dsc* descriptor,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && descriptor != nullptr &&
        characteristicValueHandle == self->transmitHandle_ &&
        ble_uuid_u16(&descriptor->uuid.u) ==
            BLE_GATT_DSC_CLT_CFG_UUID16) {
        if (self->cccdHandle_ != 0U) {
            self->markDiscoveryResult(ErrorCode::ProtocolError);
            return BLE_HS_EDONE;
        }
        self->cccdHandle_ = descriptor->handle;
        return 0;
    }
    if (error->status != BLE_HS_EDONE || self->cccdHandle_ == 0U) {
        self->markDiscoveryResult(ErrorCode::NotFound);
        return 0;
    }
    const uint8_t enabled[2] = {1U, 0U};
    const int result = ble_gattc_write_flat(
        connectionHandle, self->cccdHandle_, enabled, sizeof(enabled),
        &Esp32NimBleCentralPort::onSubscribe, self);
    if (result != 0) {
        self->markDiscoveryResult(ErrorCode::ProtocolError);
    }
    return 0;
}

inline int Esp32NimBleCentralPort::onSubscribe(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    ble_gatt_attr*,
    void* context) {
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(context);
    if (self != nullptr && error != nullptr &&
        self->currentNativeConnection(connectionHandle)) {
        self->markDiscoveryResult(
            error->status == 0U ? ErrorCode::Ok
                                : ErrorCode::ProtocolError);
    }
    return 0;
}

inline void Esp32NimBleCentralPort::markHostResult(ErrorCode error) {
    portENTER_CRITICAL(&lock_);
    pendingHostError_ = error;
    hostResultPending_ = true;
    portEXIT_CRITICAL(&lock_);
}

inline void Esp32NimBleCentralPort::markDiscoveryResult(ErrorCode error) {
    portENTER_CRITICAL(&lock_);
    if (!discoveryResultPending_ || error != ErrorCode::Ok) {
        pendingDiscoveryError_ = error;
        discoveryResultPending_ = true;
    }
    portEXIT_CRITICAL(&lock_);
}

inline bool Esp32NimBleCentralPort::currentNativeConnection(
    uint16_t connectionHandle) const {
    return connectionHandle != BLE_HS_CONN_HANDLE_NONE &&
           connectionHandle == connectionHandle_ && !stopping_;
}

inline void Esp32NimBleCentralPort::initializeUuids() {
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

inline void Esp32NimBleCentralPort::clearQueuesLocked() {
    candidateHead_ = 0U;
    candidateTail_ = 0U;
    candidateCount_ = 0U;
    packetHead_ = 0U;
    packetTail_ = 0U;
    packetCount_ = 0U;
}

inline void Esp32NimBleCentralPort::clearAttempt() {
    portENTER_CRITICAL(&lock_);
    clearQueuesLocked();
    pendingConnectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingDisconnectHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingConnectStatus_ = 0;
    pendingDiscoveryError_ = ErrorCode::Ok;
    connectEventPending_ = false;
    disconnectEventPending_ = false;
    discoveryResultPending_ = false;
    securityEventPending_ = false;
    cancelCompletionPending_ = false;
    pendingEncrypted_ = false;
    pendingBonded_ = false;
    portEXIT_CRITICAL(&lock_);
    request_ = BleCentralConnectRequest();
    connection_ = BleCentralConnectionInfo();
    connectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
    serviceStartHandle_ = 0U;
    serviceEndHandle_ = 0U;
    receiveHandle_ = 0U;
    transmitHandle_ = 0U;
    cccdHandle_ = 0U;
}

} // namespace blinker
