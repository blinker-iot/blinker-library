#include <esp_err.h>
#include <host/ble_att.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_mbuf.h>
#include <os/os_mbuf.h>

#include <string.h>

namespace blinker {

inline Esp32NimBleCentralPort::Esp32NimBleCentralPort(
    Esp32NimBleCentralHost& host, const Esp32NimBleCentralPortConfig& config)
    : host_(host), callbackContext_(nullptr), config_(config), serviceUuid_(), receiveUuid_(), transmitUuid_(),
      request_(), connection_(), packetReceiver_(nullptr),
      packetContext_(nullptr), connectedHandler_(nullptr),
      disconnectedHandler_(nullptr), connectionContext_(nullptr),
      candidates_(), packets_(), pendingWrite_(), candidateHead_(0U),
      candidateTail_(0U),
      candidateCount_(0U), packetHead_(0U), packetTail_(0U),
      packetCount_(0U),
      connectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingConnectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingDisconnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      serviceStartHandle_(0U), serviceEndHandle_(0U), receiveHandle_(0U),
      transmitHandle_(0U), cccdHandle_(0U), pendingConnectStatus_(0),
      pendingDiscoveryError_(ErrorCode::Ok),
      hostResetPending_(false), connectEventPending_(false),
      disconnectEventPending_(false), discoveryResultPending_(false),
      securityEventPending_(false), pendingEncrypted_(false),
      pendingBonded_(false), cancelCompletionPending_(false),
      pendingWriteResult_(ErrorCode::Ok), writeActive_(false),
      writeComplete_(false),
      releasePending_(false), nativeConnectPending_(false), stopping_(false), terminationRequested_(false),
      state_(BleCentralPortState::Stopped), lastError_(ErrorCode::Ok),
      lock_(portMUX_INITIALIZER_UNLOCKED) {
    initializeUuids();
}

inline bool Esp32NimBleCentralPort::validConfig() const {
    return config_.closeTimeoutMillis != 0U &&
           config_.nativeConnectTimeoutMillis != 0U &&
           config_.connectionIntervalUnits >= 6U &&
           config_.connectionIntervalUnits <= 3200U &&
           static_cast<uint32_t>(config_.connectionIntervalUnits) * 5U <
               BLE_GAP_INITIAL_SUPERVISION_TIMEOUT * 20U &&
           config_.maxScanResultsPerPoll != 0U &&
           config_.maxRxPacketsPerPoll != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_SCAN_QUEUE_DEPTH != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_RX_QUEUE_DEPTH != 0U &&
           BLINKER_ESP32_NIMBLE_CENTRAL_PACKET_SIZE >=
               ble::kMinimumPacketSize;
}

inline Result Esp32NimBleCentralPort::start() {
    if (state_ != BleCentralPortState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        state_ = BleCentralPortState::Error;
        lastError_ = ErrorCode::NotConfigured;
        return Result::failure(lastError_);
    }

    clearAttempt();
    stopping_ = false;
    lastError_ = ErrorCode::Ok;
    portENTER_CRITICAL(&lock_);
    hostResetPending_ = false;
    portEXIT_CRITICAL(&lock_);
    const Result result = host_.attach(this, &Esp32NimBleCentralPort::onHostReset);
    if (!result) {
        state_ = BleCentralPortState::Error; lastError_ = result.code();
        return result;
    }
    state_ = BleCentralPortState::Idle;
    return Result::success();
}

inline void Esp32NimBleCentralPort::stop() {
    stopping_ = true;
    // Consume SDK reset before touching a handle: another Port may already
    // have reconnected using the same numeric handle after that reset.
    processNativeEvents();
    if (host_.ownsActivity(this)) {
        if (host_.ownsInitiation(this)) {
            if (ble_gap_disc_active()) (void)ble_gap_disc_cancel();
            if (ble_gap_conn_active()) (void)ble_gap_conn_cancel();
        }
        if (connectionHandle_ != BLE_HS_CONN_HANDLE_NONE && !terminationRequested_) {
            terminationRequested_ = ble_gap_terminate(
                connectionHandle_, BLE_ERR_REM_USER_CONN_TERM) == 0;
        }

        // GAP termination is asynchronous. Calling nimble_port_stop() while
        // the native connection still exists leaves the host task alive and
        // makes the next cloud-session generation unable to claim NimBLE.
        const uint32_t startedAt = millis();
        bool idle = false;
        do {
            // A successful CONNECT may already be queued while cancellation
            // is being requested. Adopt and terminate that ACL before reuse.
            processNativeEvents();
            ble_gap_conn_desc connection = {};
            const bool connected =
                connectionHandle_ != BLE_HS_CONN_HANDLE_NONE &&
                ble_gap_conn_find(connectionHandle_, &connection) == 0;
            const bool procedure = host_.ownsInitiation(this) &&
                (ble_gap_disc_active() || ble_gap_conn_active());
            idle = !host_.ownsActivity(this) || (!nativeConnectPending_ && !connected && !procedure &&
                   host_.releaseActivity(this));
            if (!idle) delay(1U);
        } while (!idle &&
                 static_cast<uint32_t>(millis() - startedAt) <
                     config_.closeTimeoutMillis);

        if (!idle) {
            stopping_ = false;
            state_ = BleCentralPortState::Error;
            lastError_ = ErrorCode::WouldBlock;
            return;
        }
    }
    // ESP-IDF's controller is process-lifetime infrastructure: repeatedly
    // deinitializing and reinitializing it can corrupt controller/IPC state.
    // A permit window therefore stops scan/ACL activity but keeps the single
    // lazy NimBLE host alive for the next window.
    if (!host_.detach(this)) {
        stopping_ = false; state_ = BleCentralPortState::Error; lastError_ = ErrorCode::WouldBlock;
        return;
    }
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
        host_.addressType(), BLE_HS_FOREVER, &parameters,
        &Esp32NimBleCentralPort::onGapEvent, callbackContext_);
    return result == 0
               ? Result::success()
               : Result::failure(
                     result == BLE_HS_EBUSY
                         ? ErrorCode::WouldBlock
                         : ErrorCode::ProtocolError);
}

inline Result Esp32NimBleCentralPort::connect(
    const BleCentralConnectRequest& request) {
    if (stopping_) return Result::failure(ErrorCode::WouldBlock);
    if (state_ != BleCentralPortState::Idle) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (request.attemptId == 0U || request.matcher == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    void* context = nullptr;
    const Result acquired = host_.acquireActivity(this, context);
    if (!acquired) return acquired;
    clearAttempt();
    callbackContext_ = context;
    request_ = request;
    lastError_ = ErrorCode::Ok;
    state_ = BleCentralPortState::Scanning; // SDK can deliver an advertisement before disc() returns.
    Result result = startScan();
    if (!result) {
        (void)host_.releaseActivity(this);
        clearAttempt();
        state_ = BleCentralPortState::Idle;
        lastError_ = result.code();
        return result;
    }
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
        if (nativeResult == 0 || nativeResult == BLE_HS_EALREADY) {
            portENTER_CRITICAL(&lock_);
            cancelCompletionPending_ = true;
            portEXIT_CRITICAL(&lock_);
        }
    } else if (connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
        nativeResult = ble_gap_conn_cancel();
        // EALREADY can mean successful CONNECT is queued, not "no ACL".
        // Its GAP completion, rather than a synthetic cancel, owns release.
    } else {
        nativeResult = ble_gap_terminate(
            connectionHandle_, BLE_ERR_REM_USER_CONN_TERM);
        terminationRequested_ = nativeResult == 0;
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

    bool active = false;
    bool complete = false;
    bool samePacket = false;
    ErrorCode completedResult = ErrorCode::Ok;
    portENTER_CRITICAL(&lock_);
    active = writeActive_;
    if (active) {
        samePacket = pendingWrite_.attemptId == attemptId &&
            pendingWrite_.size == packet.size &&
            memcmp(pendingWrite_.data, packet.data, packet.size) == 0;
        complete = writeComplete_;
        completedResult = pendingWriteResult_;
        if (samePacket && complete) {
            pendingWrite_ = PendingWrite();
            pendingWriteResult_ = ErrorCode::Ok;
            writeActive_ = false;
            writeComplete_ = false;
        }
    }
    portEXIT_CRITICAL(&lock_);

    if (active) {
        if (!samePacket || !complete) {
            return Result::failure(ErrorCode::WouldBlock);
        }
        if (completedResult == ErrorCode::Ok) return Result::success();
        if (completedResult == ErrorCode::NotConnected) {
            completeAttempt(ErrorCode::NotConnected);
        }
        return Result::failure(completedResult);
    }

    portENTER_CRITICAL(&lock_);
    pendingWrite_.attemptId = attemptId;
    pendingWrite_.size = static_cast<uint8_t>(packet.size);
    memcpy(pendingWrite_.data, packet.data, packet.size);
    pendingWriteResult_ = ErrorCode::Ok;
    writeActive_ = true;
    writeComplete_ = false;
    portEXIT_CRITICAL(&lock_);
    const int result = ble_gattc_write_flat(
        connectionHandle_, receiveHandle_, packet.data,
        static_cast<uint16_t>(packet.size),
        &Esp32NimBleCentralPort::onWrite, callbackContext_);
    if (result == 0) {
        // Success is reported only after the remote ATT server accepted the
        // write. The Broker retries this exact packet to observe completion.
        return Result::failure(ErrorCode::WouldBlock);
    }
    portENTER_CRITICAL(&lock_);
    pendingWrite_ = PendingWrite();
    pendingWriteResult_ = ErrorCode::Ok;
    writeActive_ = false;
    writeComplete_ = false;
    portEXIT_CRITICAL(&lock_);
    if (result == BLE_HS_ENOTCONN) {
        // NimBLE can report the vanished ACL link before its queued GAP
        // disconnect event reaches poll(). Reconcile the public port state
        // immediately so callers can rescan instead of remaining in a stale
        // Ready state forever.
        completeAttempt(ErrorCode::NotConnected);
        return Result::failure(ErrorCode::NotConnected);
    }
    return Result::failure(ErrorCode::WouldBlock);
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
    if (state_ == BleCentralPortState::Stopped || state_ == BleCentralPortState::Error) return;
    processNativeEvents();
    if (state_ == BleCentralPortState::Error) return;
    if (releasePending_) { completeAttempt(lastError_); return; }
    // A local terminate can race the peer's disconnect. NimBLE then has no
    // live ACL to report, and some controller versions do not enqueue a
    // second GAP disconnect event for our request. Reconcile against the
    // authoritative connection table so the logical attempt cannot remain
    // stuck in Disconnecting forever.
    if (state_ == BleCentralPortState::Disconnecting &&
        connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_conn_desc connection = {};
        if (ble_gap_conn_find(connectionHandle_, &connection) != 0) {
            completeAttempt(ErrorCode::Ok);
            return;
        }
        if (!terminationRequested_) {
            terminationRequested_ = ble_gap_terminate(
                connectionHandle_, BLE_ERR_REM_USER_CONN_TERM) == 0;
        }
    }
    if (state_ == BleCentralPortState::Scanning) {
        processScanCandidates();
    }
    if (state_ == BleCentralPortState::Ready) drainPackets();
}

inline void Esp32NimBleCentralPort::processNativeEvents() {
    bool hostReset = false;
    bool connectEvent = false;
    bool disconnectEvent = false;
    bool discovery = false;
    bool security = false;
    bool cancelComplete = false;
    uint16_t connectHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t disconnectHandle = BLE_HS_CONN_HANDLE_NONE;
    int connectStatus = 0;
    ErrorCode discoveryError = ErrorCode::Ok;
    bool encrypted = false;
    bool bonded = false;

    portENTER_CRITICAL(&lock_);
    hostReset = hostResetPending_;
    connectEvent = connectEventPending_;
    disconnectEvent = disconnectEventPending_;
    discovery = discoveryResultPending_;
    security = securityEventPending_;
    cancelComplete = cancelCompletionPending_;
    connectHandle = pendingConnectionHandle_;
    disconnectHandle = pendingDisconnectHandle_;
    connectStatus = pendingConnectStatus_;
    discoveryError = pendingDiscoveryError_;
    encrypted = pendingEncrypted_;
    bonded = pendingBonded_;
    hostResetPending_ = false;
    connectEventPending_ = false;
    disconnectEventPending_ = false;
    discoveryResultPending_ = false;
    securityEventPending_ = false;
    cancelCompletionPending_ = false;
    if (hostReset || connectEvent) nativeConnectPending_ = false;
    portEXIT_CRITICAL(&lock_);

    if (hostReset) {
        // SDK reset retires its procedures and all ACLs.
        connectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
        terminationRequested_ = false;
        lastError_ = ErrorCode::ProtocolError;
        state_ = BleCentralPortState::Error;
        return;
    }
    if (connectEvent) {
        host_.finishInitiation(this);
    }
    if (disconnectEvent &&
        (disconnectHandle == connectionHandle_ ||
         disconnectHandle == connectHandle)) {
        completeAttempt(lastError_);
        return;
    }
    if (connectEvent) {
        if (connectStatus != 0) {
            completeAttempt(
                state_ == BleCentralPortState::Disconnecting
                    ? ErrorCode::Ok
                    : ErrorCode::NotConnected);
            return;
        }
        if (connectHandle != BLE_HS_CONN_HANDLE_NONE &&
            (stopping_ || state_ == BleCentralPortState::Disconnecting)) {
            connectionHandle_ = connectHandle;
            state_ = BleCentralPortState::Disconnecting;
            terminationRequested_ = ble_gap_terminate(
                connectHandle, BLE_ERR_REM_USER_CONN_TERM) == 0;
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
    if (cancelComplete && state_ == BleCentralPortState::Disconnecting &&
        connectionHandle_ == BLE_HS_CONN_HANDLE_NONE &&
        !ble_gap_disc_active() && !ble_gap_conn_active()) {
        completeAttempt(ErrorCode::Ok);
        return;
    }
    if (security && connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
        connection_.encrypted = encrypted;
        connection_.bonded = bonded;
    }
    if (discovery && state_ == BleCentralPortState::Discovering) {
        if (discoveryError != ErrorCode::Ok || stopping_) {
            lastError_ = discoveryError;
            state_ = BleCentralPortState::Disconnecting;
            // Failure to request termination is not proof that the ACL is
            // gone. poll() reconciles only an absent native connection.
            terminationRequested_ = ble_gap_terminate(
                connectionHandle_, BLE_ERR_REM_USER_CONN_TERM) == 0;
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
        // The observer is still active. Keep this exact attempt scanning;
        // another advertisement can retry, or its owner can cancel/expire it.
        state_ = BleCentralPortState::Scanning;
        lastError_ = ErrorCode::WouldBlock;
        return;
    }
    // Keep NimBLE's scan/supervision defaults, but explicitly request a low
    // latency interval for the single stop-and-wait GATT link. This changes
    // neither ATT reliability nor the fixed packet/queue memory footprint.
    ble_gap_conn_params params = {};
    params.scan_itvl = 0x0010U;
    params.scan_window = 0x0010U;
    params.itvl_min = config_.connectionIntervalUnits;
    params.itvl_max = config_.connectionIntervalUnits;
    params.latency = BLE_GAP_INITIAL_CONN_LATENCY;
    params.supervision_timeout = BLE_GAP_INITIAL_SUPERVISION_TIMEOUT;
    params.min_ce_len = BLE_GAP_INITIAL_CONN_MIN_CE_LEN;
    params.max_ce_len = BLE_GAP_INITIAL_CONN_MAX_CE_LEN;
    // Native initiating state can disappear BEFORE CONNECT is delivered.
    // Keep ownership until that completion is consumed, even during stop.
    portENTER_CRITICAL(&lock_); nativeConnectPending_ = true; portEXIT_CRITICAL(&lock_);
    const int result = ble_gap_connect(
        host_.addressType(), &candidate.address,
        static_cast<int32_t>(config_.nativeConnectTimeoutMillis),
        &params, &Esp32NimBleCentralPort::onGapEvent, callbackContext_);
    if (result != 0) {
        portENTER_CRITICAL(&lock_); nativeConnectPending_ = false; portEXIT_CRITICAL(&lock_);
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
        &Esp32NimBleCentralPort::onService, callbackContext_);
    if (result != 0) markDiscoveryResult(ErrorCode::ProtocolError);
}

inline void Esp32NimBleCentralPort::completeAttempt(ErrorCode error) {
    if (!host_.releaseActivity(this)) {
        releasePending_ = true; state_ = BleCentralPortState::Disconnecting; lastError_ = error;
        return;
    }
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

inline void Esp32NimBleCentralPort::onHostReset(void* context) {
    auto* self = static_cast<Esp32NimBleCentralPort*>(context);
    portENTER_CRITICAL(&self->lock_);
    self->hostResetPending_ = true;
    portEXIT_CRITICAL(&self->lock_);
}

inline int Esp32NimBleCentralPort::onGapEvent(
    ble_gap_event* event,
    void* context) {
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
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
        if (nativeConnectPending_) {
            pendingConnectStatus_ = event.connect.status;
            pendingConnectionHandle_ = event.connect.conn_handle;
            connectEventPending_ = true;
        }
        portEXIT_CRITICAL(&lock_);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_DISCONNECT) {
        portENTER_CRITICAL(&lock_);
        const uint16_t handle = event.disconnect.conn.conn_handle;
        if (handle != BLE_HS_CONN_HANDLE_NONE && (handle == connectionHandle_ ||
            (connectEventPending_ && pendingConnectStatus_ == 0 && handle == pendingConnectionHandle_))) {
            pendingDisconnectHandle_ = handle;
            disconnectEventPending_ = true;
            packetHead_ = 0U;
            packetTail_ = 0U;
            packetCount_ = 0U;
        }
        portEXIT_CRITICAL(&lock_);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_NOTIFY_RX &&
        event.notify_rx.conn_handle == connectionHandle_ &&
        event.notify_rx.attr_handle == transmitHandle_ &&
        event.notify_rx.indication != 0U) {
        (void)queuePacket(
            event.notify_rx.conn_handle, event.notify_rx.om);
        return 0;
    }
    if (event.type == BLE_GAP_EVENT_ENC_CHANGE &&
        event.enc_change.status == 0 && event.enc_change.conn_handle == connectionHandle_) {
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
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
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
        &Esp32NimBleCentralPort::onReceiveCharacteristic, self->callbackContext_);
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
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && characteristic != nullptr) {
        if (self->receiveHandle_ != 0U ||
            (characteristic->properties &
             BLE_GATT_CHR_PROP_WRITE) == 0U) {
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
        &Esp32NimBleCentralPort::onTransmitCharacteristic, self->callbackContext_);
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
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
    if (self == nullptr || error == nullptr ||
        !self->currentNativeConnection(connectionHandle)) {
        return 0;
    }
    if (error->status == 0U && characteristic != nullptr) {
        if (self->transmitHandle_ != 0U ||
            (characteristic->properties & BLE_GATT_CHR_PROP_INDICATE) == 0U) {
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
        &Esp32NimBleCentralPort::onDescriptor, self->callbackContext_);
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
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
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
    const uint8_t enabled[2] = {2U, 0U};
    const int result = ble_gattc_write_flat(
        connectionHandle, self->cccdHandle_, enabled, sizeof(enabled),
        &Esp32NimBleCentralPort::onSubscribe, self->callbackContext_);
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
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
    if (self != nullptr && error != nullptr &&
        self->currentNativeConnection(connectionHandle)) {
        self->markDiscoveryResult(
            error->status == 0U ? ErrorCode::Ok
                                : ErrorCode::ProtocolError);
    }
    return 0;
}

inline int Esp32NimBleCentralPort::onWrite(
    uint16_t connectionHandle,
    const ble_gatt_error* error,
    ble_gatt_attr*,
    void* context) {
    Esp32NimBleCentralHost::Callback callback(context);
    Esp32NimBleCentralPort* self =
        static_cast<Esp32NimBleCentralPort*>(callback.owner());
    if (self == nullptr || error == nullptr) return 0;
    portENTER_CRITICAL(&self->lock_);
    if (self->writeActive_ &&
        connectionHandle == self->connectionHandle_) {
        self->pendingWriteResult_ =
            error->status == 0U
                ? ErrorCode::Ok
                : (error->status == BLE_HS_ENOTCONN
                       ? ErrorCode::NotConnected
                       : ErrorCode::WouldBlock);
        self->writeComplete_ = true;
    }
    portEXIT_CRITICAL(&self->lock_);
    return 0;
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
    callbackContext_ = nullptr; releasePending_ = false;
    portENTER_CRITICAL(&lock_);
    nativeConnectPending_ = false;
    clearQueuesLocked();
    pendingWrite_ = PendingWrite();
    pendingWriteResult_ = ErrorCode::Ok;
    writeActive_ = false;
    writeComplete_ = false;
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
    terminationRequested_ = false;
    serviceStartHandle_ = 0U;
    serviceEndHandle_ = 0U;
    receiveHandle_ = 0U;
    transmitHandle_ = 0U;
    cccdHandle_ = 0U;
}

} // namespace blinker
