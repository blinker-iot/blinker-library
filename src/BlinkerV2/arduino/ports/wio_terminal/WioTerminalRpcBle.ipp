#include <string.h>

extern T_GAP_DEV_STATE ble_gap_dev_state;

namespace blinker {

inline WioTerminalRpcBleLink::WioTerminalRpcBleLink(
    const WioTerminalRpcBleConfig& config)
    : config_(config),
      profile_(ble::makeDirectModeProfile()),
      server_(nullptr),
      service_(nullptr),
      receive_(nullptr),
      transmit_(nullptr),
      notifyDescriptor_(nullptr),
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
      pendingConnectionId_(0xffffU),
      connectionId_(0xffffU),
      nextSessionId_(1U),
      connectedAtMillis_(0U),
      nextConnectionReconcileAt_(0U),
      connectPending_(false),
      disconnectPending_(false),
      sessionAnnounced_(false),
      gattStarted_(false),
      advertisingConfigured_(false),
      advertisingDeadlineAt_(0U),
      nextAdvertisingAttemptAt_(0U),
      rxObserved_(false),
      notifySucceeded_(true),
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok),
      lock_("BlinkerBLE") {}

inline Result WioTerminalRpcBleLink::start() {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    state_ = BleLinkState::Starting;
    // A SAMD upload resets only the host MCU. Clear any GATT server left in
    // the RTL8720DN before rpcBLE registers this process' callbacks. Normal
    // stop/start cycles keep BLEDevice initialized and reuse the live stack.
    if (!BLEDevice::getInitialized()) ble_deinit();
    BLEDevice::init(config_.deviceName);
    BLEDevice::setMTU(23U);
    if (!BLEDevice::getInitialized()) {
        stop();
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    if (server_ == nullptr) server_ = BLEDevice::createServer();
    if (server_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    server_->setCallbacks(this);
    if (service_ == nullptr) {
        service_ = server_->createService(ble::kServiceUuid);
    }
    if (service_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    if (receive_ == nullptr) {
        receive_ = service_->createCharacteristic(
            ble::kReceiveUuid,
            BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_WRITE_NR);
    }
    if (transmit_ == nullptr) {
        transmit_ = service_->createCharacteristic(
            ble::kTransmitUuid,
            BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_NOTIFY);
    }
    if (receive_ == nullptr || transmit_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    receive_->setAccessPermissions(GATT_PERM_WRITE);
    transmit_->setAccessPermissions(
        GATT_PERM_READ | GATT_PERM_NOTIF_IND);
    receive_->setCallbacks(this);
    transmit_->setCallbacks(this);
    if (notifyDescriptor_ == nullptr) {
        notifyDescriptor_ = new BLE2902();
        if (notifyDescriptor_ == nullptr) {
            stop();
            lastError_ = ErrorCode::InternalError;
            state_ = BleLinkState::Error;
            return Result::failure(lastError_);
        }
        transmit_->addDescriptor(notifyDescriptor_);
    }
    notifyDescriptor_->setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);
    notifyDescriptor_->setNotifications(false);
    if (!gattStarted_) {
        service_->start();
        gattStarted_ = true;
    }

    if (BLEDevice::getAdvertising() == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    advertisingConfigured_ = false;
    advertisingDeadlineAt_ = millis() + 5000U;
    nextAdvertisingAttemptAt_ = 0U;
    advanceAdvertising();
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

inline void WioTerminalRpcBleLink::stop() {
    if (BLEDevice::getInitialized()) {
        BLEDevice::stopAdvertising();
    }
    session_ = BleSessionInfo();
    if (lock_.take()) {
        clearPackets();
        pendingConnectionId_ = 0xffffU;
        connectPending_ = false;
        disconnectPending_ = false;
        rxObserved_ = false;
        if (notifyDescriptor_ != nullptr) {
            notifyDescriptor_->setNotifications(false);
        }
        lock_.give();
    }
    connectionId_ = 0xffffU;
    connectedAtMillis_ = 0U;
    nextConnectionReconcileAt_ = 0U;
    sessionAnnounced_ = false;
    advertisingConfigured_ = false;
    advertisingDeadlineAt_ = 0U;
    nextAdvertisingAttemptAt_ = 0U;
    state_ = BleLinkState::Stopped;
}

inline void WioTerminalRpcBleLink::poll(uint32_t) {
    if (state_ == BleLinkState::Starting) advanceAdvertising();
    if (state_ != BleLinkState::Ready) return;
    reconcileConnection();
    processConnection();
    updateSessionReadiness();
    drainPackets();
}

inline size_t WioTerminalRpcBleLink::sessionCount() const {
    return sessionAnnounced_ ? 1U : 0U;
}

inline Result WioTerminalRpcBleLink::sessionAt(
    size_t index,
    BleSessionInfo& session) const {
    if (index != 0U || !sessionAnnounced_) {
        return Result::failure(ErrorCode::NotFound);
    }
    session = session_;
    return Result::success();
}

inline Result WioTerminalRpcBleLink::sendPacket(
    uint32_t sessionId,
    ByteView packet) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        !session_.notifyEnabled || transmit_ == nullptr) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.empty() ||
        packet.size > maximumPacketSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    notifySucceeded_ = true;
    transmit_->setValue(
        const_cast<uint8_t*>(packet.data), packet.size);
    transmit_->notify();
    return notifySucceeded_
               ? Result::success()
               : Result::failure(ErrorCode::WouldBlock);
}

inline Result WioTerminalRpcBleLink::disconnectSession(uint32_t sessionId) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        server_ == nullptr || connectionId_ == 0xffffU) {
        return Result::failure(ErrorCode::NotFound);
    }
    return requestDisconnect();
}

inline void WioTerminalRpcBleLink::setPacketReceiver(
    BlePacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

inline void WioTerminalRpcBleLink::setSessionHandlers(
    BleSessionHandler connected,
    BleSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

inline Result WioTerminalRpcBleLink::configureBleProfile(
    const ble::ModeProfile& profile) {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    profile_ = profile;
    return Result::success();
}

inline void WioTerminalRpcBleLink::onConnect(BLEServer* server) {
    if (server == nullptr || !lock_.take()) return;
    pendingConnectionId_ = server->getconnId();
    connectPending_ = true;
    lock_.give();
}

inline void WioTerminalRpcBleLink::onDisconnect(BLEServer*) {
    if (!lock_.take()) return;
    disconnectPending_ = true;
    rxObserved_ = false;
    clearPackets();
    if (notifyDescriptor_ != nullptr) {
        notifyDescriptor_->setNotifications(false);
    }
    lock_.give();
}

inline void WioTerminalRpcBleLink::onWrite(
    BLECharacteristic* characteristic) {
    if (characteristic != receive_) return;
    const std::string value = characteristic->getValue();
    if (value.empty() || value.size() > maximumPacketSize || !lock_.take()) {
        return;
    }
    if (packetCount_ < sizeof(packets_) / sizeof(packets_[0])) {
        QueuedPacket& packet = packets_[packetTail_];
        packet.size = static_cast<uint8_t>(value.size());
        memcpy(packet.data, value.data(), value.size());
        packetTail_ = static_cast<uint8_t>(
            (packetTail_ + 1U) %
            (sizeof(packets_) / sizeof(packets_[0])));
        ++packetCount_;
        // Receiving the first application write proves that Android has
        // completed its subscribe-then-write sequence even when rpcBLE misses
        // the CCCD callback. Authorization still happens above this bearer.
        rxObserved_ = true;
    }
    lock_.give();
}

inline void WioTerminalRpcBleLink::onStatus(
    BLECharacteristic* characteristic,
    Status status,
    uint32_t) {
    if (characteristic == transmit_ && status != SUCCESS_NOTIFY) {
        notifySucceeded_ = false;
    }
}

inline Result WioTerminalRpcBleLink::configureAdvertising() {
    uint8_t serviceDataBytes[ble::kModeServiceDataSize] = {};
    ByteView serviceData;
    Result result = ble::encodeModeServiceData(
        profile_,
        MutableByteSpan(serviceDataBytes, sizeof(serviceDataBytes)),
        serviceData);
    if (!result) return result;

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    if (advertising == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    BLEAdvertisementData advertisement;
    advertisement.setFlags(0x06U);
    advertisement.setCompleteServices(BLEUUID(ble::kServiceUuid));
    advertisement.setName(config_.deviceName);
    BLEAdvertisementData scanResponse;
    scanResponse.setServiceData(
        BLEUUID(ble::kServiceUuid),
        std::string(
            reinterpret_cast<const char*>(serviceData.data),
            serviceData.size));
    if (advertisement.getPayload().size() > 31U ||
        scanResponse.getPayload().size() > 31U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    advertising->setAdvertisementData(advertisement);
    advertising->setScanResponseData(scanResponse);
    advertising->setScanResponse(true);
    advertising->start();
    return Result::success();
}

inline void WioTerminalRpcBleLink::advanceAdvertising() {
    if (state_ != BleLinkState::Starting) return;
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - advertisingDeadlineAt_) >= 0) {
        lastError_ = ErrorCode::NotConnected;
        state_ = BleLinkState::Error;
        return;
    }

    if (!advertisingConfigured_) {
        if (::ble_gap_dev_state.gap_adv_state != GAP_ADV_STATE_IDLE) {
            if (static_cast<int32_t>(now - nextAdvertisingAttemptAt_) >= 0) {
                le_adv_stop();
                nextAdvertisingAttemptAt_ = now + 50U;
            }
            return;
        }
        const Result result = configureAdvertising();
        if (!result) {
            lastError_ = result.code();
            state_ = BleLinkState::Error;
            return;
        }
        advertisingConfigured_ = true;
        nextAdvertisingAttemptAt_ = now + 50U;
        return;
    }

    if (::ble_gap_dev_state.gap_adv_state == GAP_ADV_STATE_ADVERTISING) {
        lastError_ = ErrorCode::Ok;
        state_ = BleLinkState::Ready;
        return;
    }
    if (::ble_gap_dev_state.gap_adv_state == GAP_ADV_STATE_IDLE &&
        static_cast<int32_t>(now - nextAdvertisingAttemptAt_) >= 0) {
        le_adv_start();
        nextAdvertisingAttemptAt_ = now + 100U;
    }
}

inline Result WioTerminalRpcBleLink::requestDisconnect() {
    if (connectionId_ == 0xffffU || connectionId_ > 0xffU) {
        return Result::failure(ErrorCode::NotConnected);
    }
    // Seeed_Arduino_rpcBLE 1.0.0 declares BLEServer::disconnect(), but its
    // implementation is empty. Use the public RTL GAP RPC used by
    // BLEClient::disconnect() so timeout and fail-closed paths really release
    // the single Wio peripheral connection.
    const T_GAP_CAUSE cause = le_disconnect(
        static_cast<uint8_t>(connectionId_));
    return cause == GAP_CAUSE_SUCCESS || cause == GAP_CAUSE_ALREADY_IN_REQ
               ? Result::success()
               : Result::failure(ErrorCode::ProtocolError);
}

inline void WioTerminalRpcBleLink::reconcileConnection() {
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextConnectionReconcileAt_) < 0) return;
    nextConnectionReconcileAt_ = now + 250U;

    // rpcBLE can occasionally lose a GAP callback while the RTL8720DN still
    // reports the correct single-link state. Reconcile that platform fact so
    // a stale session cannot occupy Wio's only peripheral connection.
    const bool connected = le_get_active_link_num() != 0U;
    if (connected == session_.connected) return;
    if (!lock_.take()) return;
    if (connected) {
        pendingConnectionId_ = 0U;
        connectPending_ = true;
    } else {
        connectPending_ = false;
        disconnectPending_ = true;
        clearPackets();
        if (notifyDescriptor_ != nullptr) {
            notifyDescriptor_->setNotifications(false);
        }
    }
    lock_.give();
}

inline void WioTerminalRpcBleLink::processConnection() {
    bool connect = false;
    bool disconnect = false;
    uint16_t pendingId = 0xffffU;
    if (lock_.take()) {
        connect = connectPending_;
        disconnect = disconnectPending_;
        pendingId = pendingConnectionId_;
        connectPending_ = false;
        disconnectPending_ = false;
        lock_.give();
    }
    if (disconnect) {
        const BleSessionInfo old = session_;
        const bool announced = sessionAnnounced_;
        const uint16_t oldConnectionId = connectionId_;
        session_ = BleSessionInfo();
        connectionId_ = 0xffffU;
        connectedAtMillis_ = 0U;
        sessionAnnounced_ = false;
        rxObserved_ = false;
        if (announced && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, old);
        }
        if (server_ != nullptr && oldConnectionId != 0xffffU) {
            server_->removePeerDevice(oldConnectionId, false);
        }
        if (::ble_gap_dev_state.gap_adv_state == GAP_ADV_STATE_IDLE) {
            le_adv_start();
        }
    }
    if (connect && !disconnect && !session_.connected &&
        pendingId != 0xffffU) {
        connectionId_ = pendingId;
        session_ = BleSessionInfo();
        session_.sessionId = nextSessionId();
        session_.maxPacketSize = maximumPacketSize;
        session_.connected = true;
        connectedAtMillis_ = millis();
    }
}

inline void WioTerminalRpcBleLink::updateSessionReadiness() {
    if (!session_.connected || connectionId_ == 0xffffU) return;
    // RTL8720DN firmware 2.1.3 cannot restore Android RPA bonds reliably.
    // Keep the native link unpaired and report its facts exactly. Enrollment
    // is protected by Noise; normal traffic becomes AES-GCM DirectSecure
    // immediately after Method 2 controller authentication.
    const bool ready = rxObserved_ ||
                       (notifyDescriptor_ != nullptr &&
                        notifyDescriptor_->getNotifications());
    session_.encrypted = false;
    session_.bonded = false;
    session_.notifyEnabled = ready;
    if (!sessionAnnounced_ && !ready &&
        static_cast<uint32_t>(millis() - connectedAtMillis_) >=
            config_.sessionReadyTimeoutMillis) {
        requestDisconnect();
        return;
    }
    if (sessionAnnounced_ && !ready) {
        requestDisconnect();
        return;
    }
    if (!sessionAnnounced_ && ready) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }
}

inline void WioTerminalRpcBleLink::drainPackets() {
    if (!sessionAnnounced_ || packetReceiver_ == nullptr) return;

    uint8_t delivered = 0U;
    while (delivered < config_.maxRxPacketsPerPoll) {
        QueuedPacket packet;
        bool available = false;
        if (lock_.take()) {
            if (packetCount_ != 0U) {
                packet = packets_[packetHead_];
                packetHead_ = static_cast<uint8_t>(
                    (packetHead_ + 1U) %
                    (sizeof(packets_) / sizeof(packets_[0])));
                --packetCount_;
                available = true;
            }
            lock_.give();
        }
        if (!available) return;
        ++delivered;
        packetReceiver_(
            packetContext_,
            session_,
            ByteView(packet.data, packet.size));
    }
}

inline void WioTerminalRpcBleLink::clearPackets() {
    packetHead_ = 0U;
    packetTail_ = 0U;
    packetCount_ = 0U;
}

inline uint32_t WioTerminalRpcBleLink::nextSessionId() {
    const uint32_t result = nextSessionId_;
    ++nextSessionId_;
    if (nextSessionId_ == 0U) ++nextSessionId_;
    return result;
}

inline bool WioTerminalRpcBleLink::validConfig() const {
    return config_.deviceName != nullptr && config_.deviceName[0] != '\0' &&
           strlen(config_.deviceName) <= ble::kLegacyLocalNameMaxSize &&
           config_.sessionReadyTimeoutMillis != 0U &&
           config_.maxRxPacketsPerPoll != 0U;
}

} // namespace blinker
