#include <string.h>

namespace blinker {

inline Esp32NimBleLink::Esp32NimBleLink(
    const Esp32NimBleLinkConfig& config)
    : config_(config),
      profile_(ble::makeDirectModeProfile()),
      server_(nullptr),
      service_(nullptr),
      receive_(nullptr),
      transmit_(nullptr),
      advertising_(nullptr),
      session_(),
      packetReceiver_(nullptr),
      packetContext_(nullptr),
      connectedHandler_(nullptr),
      disconnectedHandler_(nullptr),
      sessionContext_(nullptr),
      packets_(),
      packetHead_(0),
      packetTail_(0),
      packetCount_(0),
      connectionHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingConnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingDisconnectHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingSubscribeHandle_(BLE_HS_CONN_HANDLE_NONE),
      pendingSecurityHandle_(BLE_HS_CONN_HANDLE_NONE),
      nextSessionId_(1),
      connectedAtMillis_(0U),
      pendingNotifyEnabled_(false),
      pendingEncrypted_(false),
      pendingBonded_(false),
      connectPending_(false),
      disconnectPending_(false),
      subscribePending_(false),
      securityPending_(false),
      sessionAnnounced_(false),
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok),
      lock_(portMUX_INITIALIZER_UNLOCKED) {}

inline Result Esp32NimBleLink::start() {
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
    Result profileResult = ble::encodeModeServiceData(
        profile_,
        MutableByteSpan(serviceDataBytes, sizeof(serviceDataBytes)),
        serviceData);
    if (!profileResult) {
        lastError_ = profileResult.code();
        state_ = BleLinkState::Error;
        return profileResult;
    }
    state_ = BleLinkState::Starting;
    if (!NimBLEDevice::init(config_.deviceName) ||
        !NimBLEDevice::setMTU(BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE + 3U)) {
        NimBLEDevice::deinit(true);
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    // BBP/2 Method 2 authenticates an installed ControllerCredential, but
    // only after the platform has established an encrypted link. A no-I/O
    // peripheral therefore uses LE Secure Connections without link-layer
    // MITM; the application proof supplies the identity that Just Works lacks.
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityAuth(config_.bonding, false, true);

    server_ = NimBLEDevice::createServer();
    if (server_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    server_->setCallbacks(this, false);
    server_->advertiseOnDisconnect(true);
    service_ = server_->createService(config_.serviceUuid);
    if (service_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    receive_ = service_->createCharacteristic(
        config_.receiveUuid,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR |
            NIMBLE_PROPERTY::WRITE_ENC,
        BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE);
    transmit_ = service_->createCharacteristic(
        config_.transmitUuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY |
            NIMBLE_PROPERTY::READ_ENC,
        BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE);
    if (receive_ == nullptr || transmit_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    receive_->setCallbacks(this);
    transmit_->setCallbacks(this);
    if (!server_->start()) {
        stop();
        lastError_ = ErrorCode::ProtocolError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    advertising_ = NimBLEDevice::getAdvertising();
    if (advertising_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    NimBLEAdvertisementData advertisementData;
    NimBLEAdvertisementData scanResponseData;
    const NimBLEUUID serviceUuid(config_.serviceUuid);
    if (!advertisementData.setFlags(0x06U) ||
        !advertisementData.addServiceUUID(serviceUuid) ||
        !scanResponseData.setServiceData(
            serviceUuid,
            serviceData.data,
            serviceData.size) ||
        !advertising_->setAdvertisementData(advertisementData) ||
        !advertising_->setScanResponseData(scanResponseData)) {
        stop();
        lastError_ = ErrorCode::CapacityExceeded;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    advertising_->enableScanResponse(true);
    if (!advertising_->start()) {
        stop();
        lastError_ = ErrorCode::ProtocolError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    lastError_ = ErrorCode::Ok;
    state_ = BleLinkState::Ready;
    return Result::success();
}

inline void Esp32NimBleLink::stop() {
    if (NimBLEDevice::isInitialized()) {
        NimBLEDevice::stopAdvertising();
        NimBLEDevice::deinit(true);
    }
    server_ = nullptr;
    service_ = nullptr;
    receive_ = nullptr;
    transmit_ = nullptr;
    advertising_ = nullptr;
    session_ = BleSessionInfo();
    connectedAtMillis_ = 0U;
    portENTER_CRITICAL(&lock_);
    clearPacketQueueLocked();
    connectionHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingConnectHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingDisconnectHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingSubscribeHandle_ = BLE_HS_CONN_HANDLE_NONE;
    pendingSecurityHandle_ = BLE_HS_CONN_HANDLE_NONE;
    connectPending_ = false;
    disconnectPending_ = false;
    subscribePending_ = false;
    securityPending_ = false;
    sessionAnnounced_ = false;
    portEXIT_CRITICAL(&lock_);
    state_ = BleLinkState::Stopped;
}

inline void Esp32NimBleLink::poll(uint32_t) {
    if (state_ != BleLinkState::Ready) return;
    processPendingEvents();
    if (session_.connected && !sessionAnnounced_ &&
        static_cast<uint32_t>(millis() - connectedAtMillis_) >=
            config_.sessionReadyTimeoutMillis &&
        server_ != nullptr &&
        connectionHandle_ != BLE_HS_CONN_HANDLE_NONE) {
        server_->disconnect(connectionHandle_);
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
        !session_.encrypted || !session_.notifyEnabled || transmit_ == nullptr ||
        connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.size == 0U ||
        packet.size > session_.maxPacketSize ||
        packet.size > BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return transmit_->notify(
               packet.data,
               packet.size,
               connectionHandle_)
               ? Result::success()
               : Result::failure(ErrorCode::WouldBlock);
}

inline Result Esp32NimBleLink::disconnectSession(uint32_t sessionId) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        server_ == nullptr || connectionHandle_ == BLE_HS_CONN_HANDLE_NONE) {
        return Result::failure(ErrorCode::NotFound);
    }
    server_->disconnect(connectionHandle_);
    return Result::success();
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
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    profile_ = profile;
    return Result::success();
}

inline Result Esp32NimBleLink::setSessionSecurity(
    uint32_t sessionId,
    bool encrypted,
    bool bonded,
    bool authenticated) {
    if (!session_.connected || session_.sessionId != sessionId) {
        return Result::failure(ErrorCode::NotFound);
    }
    // Encryption and bonding are platform facts populated from
    // NimBLEConnInfo. Do not allow application code to spoof either value;
    // this API may only promote the application-authenticated bit.
    if (encrypted != session_.encrypted || bonded != session_.bonded ||
        (authenticated && !session_.encrypted)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    session_.encrypted = encrypted;
    session_.bonded = bonded;
    session_.authenticated = authenticated;
    return Result::success();
}

inline void Esp32NimBleLink::onConnect(
    NimBLEServer* server,
    NimBLEConnInfo& connection) {
    const uint16_t handle = connection.getConnHandle();
    bool reject = false;
    portENTER_CRITICAL(&lock_);
    const bool activeConnectionRetiring =
        disconnectPending_ &&
        pendingDisconnectHandle_ == connectionHandle_;
    const bool pendingConnectionRetiring =
        disconnectPending_ &&
        pendingDisconnectHandle_ == pendingConnectHandle_;
    if ((connectionHandle_ != BLE_HS_CONN_HANDLE_NONE &&
         connectionHandle_ != handle &&
         !activeConnectionRetiring) ||
        (connectPending_ && pendingConnectHandle_ != handle &&
         !pendingConnectionRetiring)) {
        reject = true;
    } else {
        pendingConnectHandle_ = handle;
        connectPending_ = true;
    }
    portEXIT_CRITICAL(&lock_);
    if (reject && server != nullptr) server->disconnect(handle);
}

inline void Esp32NimBleLink::onDisconnect(
    NimBLEServer*,
    NimBLEConnInfo& connection,
    int) {
    const uint16_t handle = connection.getConnHandle();
    portENTER_CRITICAL(&lock_);
    if (connectionHandle_ == handle ||
        (connectPending_ && pendingConnectHandle_ == handle)) {
        pendingDisconnectHandle_ = handle;
        disconnectPending_ = true;
        clearPacketQueueLocked();
    }
    portEXIT_CRITICAL(&lock_);
}

inline void Esp32NimBleLink::onAuthenticationComplete(
    NimBLEConnInfo& connection) {
    portENTER_CRITICAL(&lock_);
    pendingSecurityHandle_ = connection.getConnHandle();
    pendingEncrypted_ = connection.isEncrypted();
    pendingBonded_ = connection.isBonded();
    securityPending_ = true;
    portEXIT_CRITICAL(&lock_);
}

inline void Esp32NimBleLink::onWrite(
    NimBLECharacteristic* characteristic,
    NimBLEConnInfo& connection) {
    if (characteristic == nullptr) return;
    const NimBLEAttValue& value = characteristic->getValue();
    const size_t size = value.size();
    const uint8_t* data = value.data();
    if (data == nullptr || size == 0U ||
        size > BLINKER_ESP32_NIMBLE_MAX_PACKET_SIZE) {
        return;
    }

    const uint16_t handle = connection.getConnHandle();
    portENTER_CRITICAL(&lock_);
    const bool acceptedConnection =
        connectionHandle_ == handle ||
        (connectPending_ && pendingConnectHandle_ == handle);
    if (acceptedConnection &&
        packetCount_ < BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH) {
        QueuedPacket& packet = packets_[packetTail_];
        packet.connectionHandle = handle;
        packet.size = static_cast<uint16_t>(size);
        memcpy(packet.data, data, size);
        packetTail_ = static_cast<uint8_t>(
            (packetTail_ + 1U) % BLINKER_ESP32_NIMBLE_RX_QUEUE_DEPTH);
        ++packetCount_;
    }
    portEXIT_CRITICAL(&lock_);
}

inline void Esp32NimBleLink::onSubscribe(
    NimBLECharacteristic* characteristic,
    NimBLEConnInfo& connection,
    uint16_t subscription) {
    if (characteristic != transmit_) return;
    portENTER_CRITICAL(&lock_);
    pendingSubscribeHandle_ = connection.getConnHandle();
    pendingNotifyEnabled_ = (subscription & 1U) != 0U;
    subscribePending_ = true;
    portEXIT_CRITICAL(&lock_);
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

    if (disconnect &&
        connectionHandle_ == disconnectHandle &&
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
        if (!encrypted) session_.authenticated = false;
    }

    // Do not expose a Device V2 session until it is usable and encrypted.
    // This makes HELLO the first application packet on the secure link and
    // prevents a transient plaintext session from reaching the runtime.
    if (!sessionAnnounced_ && session_.connected && session_.encrypted &&
        session_.notifyEnabled) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }

    if (security && !encrypted && server_ != nullptr &&
        securityHandle != BLE_HS_CONN_HANDLE_NONE) {
        server_->disconnect(securityHandle);
    }
}

inline void Esp32NimBleLink::drainPackets() {
    uint8_t delivered = 0;
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
        if (sessionAnnounced_ && session_.encrypted &&
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
    packetHead_ = 0;
    packetTail_ = 0;
    packetCount_ = 0;
}

inline uint32_t Esp32NimBleLink::nextSessionId() {
    const uint32_t result = nextSessionId_;
    ++nextSessionId_;
    if (nextSessionId_ == 0U) ++nextSessionId_;
    return result;
}

inline bool Esp32NimBleLink::validConfig() const {
    return config_.deviceName != nullptr && config_.deviceName[0] != '\0' &&
           config_.serviceUuid != nullptr && config_.serviceUuid[0] != '\0' &&
           config_.receiveUuid != nullptr && config_.receiveUuid[0] != '\0' &&
           config_.transmitUuid != nullptr && config_.transmitUuid[0] != '\0' &&
           strcmp(config_.serviceUuid, ble::kServiceUuid) == 0 &&
           strcmp(config_.receiveUuid, ble::kReceiveUuid) == 0 &&
           strcmp(config_.transmitUuid, ble::kTransmitUuid) == 0 &&
           config_.sessionReadyTimeoutMillis != 0U &&
           config_.maxRxPacketsPerPoll != 0U;
}

} // namespace blinker
