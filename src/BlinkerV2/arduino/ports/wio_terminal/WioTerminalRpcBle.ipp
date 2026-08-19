#include <string.h>

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
      connectPending_(false),
      disconnectPending_(false),
      sessionAnnounced_(false),
      notifySucceeded_(true),
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok),
      lock_("BlinkerBLE") {}

inline bool WioTerminalRpcBleLink::configureSecurity() const {
    uint8_t pairable = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t requirements = config_.bonding
                                ? GAP_AUTHEN_BIT_BONDING_FLAG
                                : GAP_AUTHEN_BIT_NONE;
    uint8_t ioCapability = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t securityRequest = 1U;
    return gap_set_param(
               GAP_PARAM_BOND_PAIRING_MODE,
               sizeof(pairable),
               &pairable) == GAP_CAUSE_SUCCESS &&
           gap_set_param(
               GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS,
               sizeof(requirements),
               &requirements) == GAP_CAUSE_SUCCESS &&
           gap_set_param(
               GAP_PARAM_BOND_IO_CAPABILITIES,
               sizeof(ioCapability),
               &ioCapability) == GAP_CAUSE_SUCCESS &&
           le_bond_set_param(
               GAP_PARAM_BOND_SEC_REQ_ENABLE,
               sizeof(securityRequest),
               &securityRequest) == GAP_CAUSE_SUCCESS &&
           le_bond_set_param(
               GAP_PARAM_BOND_SEC_REQ_REQUIREMENT,
               sizeof(requirements),
               &requirements) == GAP_CAUSE_SUCCESS;
}

inline Result WioTerminalRpcBleLink::start() {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    uint8_t serviceDataBytes[ble::kModeServiceDataSize] = {};
    ByteView serviceData;
    Result result = ble::encodeModeServiceData(
        profile_,
        MutableByteSpan(serviceDataBytes, sizeof(serviceDataBytes)),
        serviceData);
    if (!result) return result;

    state_ = BleLinkState::Starting;
    BLEDevice::init(config_.deviceName);
    BLEDevice::setMTU(23U);
    if (!BLEDevice::getInitialized() || !configureSecurity()) {
        stop();
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    server_ = BLEDevice::createServer();
    if (server_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    server_->setCallbacks(this);
    service_ = server_->createService(ble::kServiceUuid);
    if (service_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    receive_ = service_->createCharacteristic(
        ble::kReceiveUuid,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR);
    transmit_ = service_->createCharacteristic(
        ble::kTransmitUuid,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);
    if (receive_ == nullptr || transmit_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    receive_->setAccessPermissions(GATT_PERM_WRITE_ENCRYPTED_REQ);
    transmit_->setAccessPermissions(
        GATT_PERM_READ_ENCRYPTED_REQ |
        GATT_PERM_NOTIF_IND_ENCRYPTED_REQ);
    receive_->setCallbacks(this);
    transmit_->setCallbacks(this);
    notifyDescriptor_ = new BLE2902();
    if (notifyDescriptor_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    notifyDescriptor_->setAccessPermissions(
        GATT_PERM_READ_ENCRYPTED_REQ |
        GATT_PERM_WRITE_ENCRYPTED_REQ);
    transmit_->addDescriptor(notifyDescriptor_);
    service_->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    if (advertising == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    BLEAdvertisementData advertisement;
    advertisement.setFlags(0x06U);
    advertisement.setCompleteServices(BLEUUID(ble::kServiceUuid));
    BLEAdvertisementData scanResponse;
    scanResponse.setServiceData(
        BLEUUID(ble::kServiceUuid),
        std::string(
            reinterpret_cast<const char*>(serviceData.data),
            serviceData.size));
    if (advertisement.getPayload().size() > 31U ||
        scanResponse.getPayload().size() > 31U) {
        stop();
        lastError_ = ErrorCode::CapacityExceeded;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    advertising->setAdvertisementData(advertisement);
    advertising->setScanResponseData(scanResponse);
    advertising->setScanResponse(true);
    advertising->start();
    lastError_ = ErrorCode::Ok;
    state_ = BleLinkState::Ready;
    return Result::success();
}

inline void WioTerminalRpcBleLink::stop() {
    if (BLEDevice::getInitialized()) {
        BLEDevice::stopAdvertising();
        BLEDevice::deinit();
    }
    server_ = nullptr;
    service_ = nullptr;
    receive_ = nullptr;
    transmit_ = nullptr;
    notifyDescriptor_ = nullptr;
    session_ = BleSessionInfo();
    if (lock_.take()) {
        clearPackets();
        pendingConnectionId_ = 0xffffU;
        connectPending_ = false;
        disconnectPending_ = false;
        lock_.give();
    }
    connectionId_ = 0xffffU;
    sessionAnnounced_ = false;
    state_ = BleLinkState::Stopped;
}

inline void WioTerminalRpcBleLink::poll(uint32_t) {
    if (state_ != BleLinkState::Ready) return;
    processConnection();
    updateSecurity();
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
        !session_.encrypted || !session_.notifyEnabled ||
        transmit_ == nullptr) {
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
    clearPackets();
    lock_.give();
}

inline void WioTerminalRpcBleLink::onWrite(
    BLECharacteristic* characteristic) {
    if (characteristic != receive_) return;
    const std::string value = characteristic->getValue();
    if (value.empty() || value.size() > maximumPacketSize ||
        !lock_.take()) {
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
    if (disconnect && session_.connected) {
        const BleSessionInfo old = session_;
        session_ = BleSessionInfo();
        connectionId_ = 0xffffU;
        if (sessionAnnounced_ && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, old);
        }
        sessionAnnounced_ = false;
    }
    if (connect && !disconnect && !session_.connected &&
        pendingId != 0xffffU) {
        connectionId_ = pendingId;
        session_ = BleSessionInfo();
        session_.sessionId = nextSessionId();
        session_.maxPacketSize = maximumPacketSize;
        session_.connected = true;
    }
}

inline void WioTerminalRpcBleLink::updateSecurity() {
    if (!session_.connected || connectionId_ == 0xffffU) return;
    T_GAP_SEC_LEVEL level = GAP_SEC_LEVEL_NO;
    const bool encrypted = le_bond_get_sec_level(
                               static_cast<uint8_t>(connectionId_),
                               &level) == GAP_CAUSE_SUCCESS &&
                           level != GAP_SEC_LEVEL_NO;
    session_.encrypted = encrypted;
    // rpcBLE exposes the exact encrypted level but not an exact persisted-key
    // query for the active peer. ControllerCredential authentication is the
    // durable identity, so do not invent a bonded=true platform fact.
    session_.bonded = false;
    session_.notifyEnabled = encrypted && notifyDescriptor_ != nullptr &&
                             notifyDescriptor_->getNotifications();
    if (sessionAnnounced_ && !encrypted) {
        if (server_ != nullptr) server_->disconnect(connectionId_);
        return;
    }
    if (!sessionAnnounced_ && encrypted && session_.notifyEnabled) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }
}

inline void WioTerminalRpcBleLink::drainPackets() {
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
        if (sessionAnnounced_ && packetReceiver_ != nullptr) {
            packetReceiver_(
                packetContext_,
                session_,
                ByteView(packet.data, packet.size));
        }
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
           config_.maxRxPacketsPerPoll != 0U;
}

} // namespace blinker
