#include "BlinkerEsp32BleAdapter.h"

#include <string.h>

namespace blinker {

Esp32BleLink::Esp32BleLink(const Esp32BleLinkConfig& config)
    : config_(config),
      profile_(ble::makeDirectModeProfile()),
      server_(nullptr),
      service_(nullptr),
      receive_(nullptr),
      transmit_(nullptr),
      transmitDescriptor_(nullptr),
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
      connectionHandle_(invalidConnectionHandle),
      pendingConnectHandle_(invalidConnectionHandle),
      pendingDisconnectHandle_(invalidConnectionHandle),
      pendingMtuHandle_(invalidConnectionHandle),
      pendingSubscribeHandle_(invalidConnectionHandle),
      pendingSecurityHandle_(invalidConnectionHandle),
      pendingPacketSize_(20),
      nextSessionId_(1),
      pendingNotifyEnabled_(false),
      pendingEncrypted_(false),
      pendingBonded_(false),
      connectPending_(false),
      disconnectPending_(false),
      mtuPending_(false),
      subscribePending_(false),
      securityPending_(false),
      sessionAnnounced_(false),
#if defined(CONFIG_BLUEDROID_ENABLED)
      pendingRemoteAddress_(),
#endif
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok),
      lock_(portMUX_INITIALIZER_UNLOCKED) {}

Result Esp32BleLink::start() {
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
    BLEDevice::init(config_.deviceName);
    if (BLEDevice::setMTU(config_.preferredMtu) != ESP_OK) {
        lastError_ = ErrorCode::NotConfigured;
        BLEDevice::deinit(true);
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    // A no-I/O peripheral uses LE Secure Connections without link-layer
    // MITM. BBP/2 Method 2 authenticates the controller on that encrypted
    // channel, while bonding makes later reconnects durable.
    BLEDevice::setSecurityCallbacks(this);
    BLESecurity::setCapability(ESP_IO_CAP_NONE);
    BLESecurity::setKeySize(16);
    BLESecurity::setInitEncryptionKey();
    BLESecurity::setRespEncryptionKey();
    BLESecurity::setAuthenticationMode(config_.bonding, false, true);

    server_ = BLEDevice::createServer();
    if (server_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    server_->setCallbacks(this);
    service_ = server_->createService(config_.serviceUuid);
    if (service_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    uint32_t receiveProperties = BLECharacteristic::PROPERTY_WRITE |
                                 BLECharacteristic::PROPERTY_WRITE_NR;
    uint32_t transmitProperties = BLECharacteristic::PROPERTY_READ |
                                  BLECharacteristic::PROPERTY_NOTIFY;
#if defined(CONFIG_NIMBLE_ENABLED)
    receiveProperties |= BLECharacteristic::PROPERTY_WRITE_ENC;
    transmitProperties |= BLECharacteristic::PROPERTY_READ_ENC;
#endif
    receive_ = service_->createCharacteristic(
        config_.receiveUuid,
        receiveProperties);
    transmit_ = service_->createCharacteristic(
        config_.transmitUuid,
        transmitProperties);
    if (receive_ == nullptr || transmit_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

#if defined(CONFIG_BLUEDROID_ENABLED)
    receive_->setAccessPermissions(
        ESP_GATT_PERM_WRITE_ENCRYPTED);
    transmit_->setAccessPermissions(
        ESP_GATT_PERM_READ_ENCRYPTED);
#endif
    receive_->setCallbacks(this);
    transmit_->setCallbacks(this);
    transmitDescriptor_ = new BLE2902();
#if defined(CONFIG_BLUEDROID_ENABLED)
    transmitDescriptor_->setAccessPermissions(
        ESP_GATT_PERM_READ_ENCRYPTED |
        ESP_GATT_PERM_WRITE_ENCRYPTED);
#endif
    transmit_->addDescriptor(transmitDescriptor_);
    if (!service_->start()) {
        stop();
        lastError_ = ErrorCode::ProtocolError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }

    advertising_ = BLEDevice::getAdvertising();
    if (advertising_ == nullptr) {
        stop();
        lastError_ = ErrorCode::InternalError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData;
    const BLEUUID serviceUuid(config_.serviceUuid);
    advertisementData.setFlags(0x06U);
    advertisementData.setCompleteServices(serviceUuid);
    scanResponseData.setServiceData(
        serviceUuid,
        String(
            reinterpret_cast<const char*>(serviceData.data),
            static_cast<unsigned int>(serviceData.size)));
    if (!advertising_->setAdvertisementData(advertisementData) ||
        !advertising_->setScanResponseData(scanResponseData)) {
        stop();
        lastError_ = ErrorCode::CapacityExceeded;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    advertising_->setScanResponse(true);
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

void Esp32BleLink::stop() {
    if (advertising_ != nullptr) advertising_->stop();
    if (service_ != nullptr) service_->stop();
    if (BLEDevice::getInitialized()) {
        BLEDevice::setSecurityCallbacks(nullptr);
        BLEDevice::deinit(true);
    }
    server_ = nullptr;
    service_ = nullptr;
    receive_ = nullptr;
    transmit_ = nullptr;
    transmitDescriptor_ = nullptr;
    advertising_ = nullptr;
    session_ = BleSessionInfo();
    portENTER_CRITICAL(&lock_);
    clearPacketQueueLocked();
    connectionHandle_ = invalidConnectionHandle;
    pendingConnectHandle_ = invalidConnectionHandle;
    pendingDisconnectHandle_ = invalidConnectionHandle;
    pendingMtuHandle_ = invalidConnectionHandle;
    pendingSubscribeHandle_ = invalidConnectionHandle;
    pendingSecurityHandle_ = invalidConnectionHandle;
    connectPending_ = false;
    disconnectPending_ = false;
    mtuPending_ = false;
    subscribePending_ = false;
    securityPending_ = false;
    sessionAnnounced_ = false;
    portEXIT_CRITICAL(&lock_);
    state_ = BleLinkState::Stopped;
}

void Esp32BleLink::poll(uint32_t) {
    if (state_ != BleLinkState::Ready) return;
    processPendingEvents();
    refreshSubscription();
    announceSessionIfReady();
    drainPackets();
}

BleLinkState Esp32BleLink::state() const { return state_; }
ErrorCode Esp32BleLink::lastError() const { return lastError_; }
size_t Esp32BleLink::sessionCount() const {
    return sessionAnnounced_ ? 1U : 0U;
}

Result Esp32BleLink::sessionAt(
    size_t index,
    BleSessionInfo& session) const {
    if (index != 0U || !sessionAnnounced_) {
        return Result::failure(ErrorCode::NotFound);
    }
    session = session_;
    return Result::success();
}

Result Esp32BleLink::sendPacket(
    uint32_t sessionId,
    ByteView packet) {
    refreshSubscription();
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        !session_.encrypted || !session_.notifyEnabled ||
        transmit_ == nullptr) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.size == 0U ||
        packet.size > session_.maxPacketSize ||
        packet.size > BLINKER_ESP32_BLE_MAX_PACKET_SIZE) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    transmit_->setValue(packet.data, packet.size);
    transmit_->notify();
    return Result::success();
}

void Esp32BleLink::setPacketReceiver(
    BlePacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

void Esp32BleLink::setSessionHandlers(
    BleSessionHandler connected,
    BleSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

Result Esp32BleLink::configureBleProfile(
    const ble::ModeProfile& profile) {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    profile_ = profile;
    return Result::success();
}

Result Esp32BleLink::setSessionSecurity(
    uint32_t sessionId,
    bool encrypted,
    bool bonded,
    bool authenticated) {
    if (!session_.connected || session_.sessionId != sessionId) {
        return Result::failure(ErrorCode::NotFound);
    }
    // Encryption and bonding are facts supplied by the BLE stack. This
    // compatibility API may only promote application authentication.
    if (encrypted != session_.encrypted || bonded != session_.bonded ||
        (authenticated && !session_.encrypted)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    session_.authenticated = authenticated;
    return Result::success();
}

void Esp32BleLink::onWrite(BLECharacteristic* characteristic) {
    uint16_t handle = invalidConnectionHandle;
    portENTER_CRITICAL(&lock_);
    handle = connectionHandle_;
    if (handle == invalidConnectionHandle && connectPending_) {
        handle = pendingConnectHandle_;
    }
    portEXIT_CRITICAL(&lock_);
    queuePacket(characteristic, handle);
}

#if defined(CONFIG_BLUEDROID_ENABLED)
void Esp32BleLink::onConnect(
    BLEServer* server,
    esp_ble_gatts_cb_param_t* parameter) {
    if (parameter == nullptr) return;
    const uint16_t handle = parameter->connect.conn_id;
    bool reject = false;
    portENTER_CRITICAL(&lock_);
    if ((connectionHandle_ != invalidConnectionHandle &&
         connectionHandle_ != handle) ||
        (connectPending_ && pendingConnectHandle_ != handle)) {
        reject = true;
    } else {
        pendingConnectHandle_ = handle;
        pendingPacketSize_ = 20;
        memcpy(
            pendingRemoteAddress_,
            parameter->connect.remote_bda,
            ESP_BD_ADDR_LEN);
        connectPending_ = true;
    }
    portEXIT_CRITICAL(&lock_);
    if (reject && server != nullptr) server->disconnect(handle);
}

void Esp32BleLink::onDisconnect(
    BLEServer*,
    esp_ble_gatts_cb_param_t* parameter) {
    if (parameter == nullptr) return;
    const uint16_t handle = parameter->disconnect.conn_id;
    portENTER_CRITICAL(&lock_);
    if (connectionHandle_ == handle ||
        (connectPending_ && pendingConnectHandle_ == handle)) {
        pendingDisconnectHandle_ = handle;
        disconnectPending_ = true;
        clearPacketQueueLocked();
    }
    portEXIT_CRITICAL(&lock_);
}

void Esp32BleLink::onMtuChanged(
    BLEServer*,
    esp_ble_gatts_cb_param_t* parameter) {
    if (parameter != nullptr) {
        queueMtu(parameter->mtu.conn_id, parameter->mtu.mtu);
    }
}

void Esp32BleLink::onWrite(
    BLECharacteristic* characteristic,
    esp_ble_gatts_cb_param_t* parameter) {
    if (parameter != nullptr) {
        queuePacket(characteristic, parameter->write.conn_id);
    }
}

void Esp32BleLink::onAuthenticationComplete(
    esp_ble_auth_cmpl_t result) {
    portENTER_CRITICAL(&lock_);
    if ((connectPending_ || connectionHandle_ != invalidConnectionHandle) &&
        memcmp(result.bd_addr, pendingRemoteAddress_, ESP_BD_ADDR_LEN) == 0) {
        pendingSecurityHandle_ = connectPending_
                                     ? pendingConnectHandle_
                                     : connectionHandle_;
        pendingEncrypted_ = result.success;
        pendingBonded_ = result.success &&
                         (result.auth_mode & ESP_LE_AUTH_BOND) != 0U;
        securityPending_ = true;
    }
    portEXIT_CRITICAL(&lock_);
}
#endif

#if defined(CONFIG_NIMBLE_ENABLED)
void Esp32BleLink::onConnect(
    BLEServer* server,
    ble_gap_conn_desc* description) {
    if (description == nullptr) return;
    const uint16_t handle = description->conn_handle;
    bool reject = false;
    portENTER_CRITICAL(&lock_);
    if ((connectionHandle_ != invalidConnectionHandle &&
         connectionHandle_ != handle) ||
        (connectPending_ && pendingConnectHandle_ != handle)) {
        reject = true;
    } else {
        pendingConnectHandle_ = handle;
        pendingPacketSize_ = 20;
        connectPending_ = true;
    }
    portEXIT_CRITICAL(&lock_);
    if (reject && server != nullptr) server->disconnect(handle);
}

void Esp32BleLink::onDisconnect(
    BLEServer*,
    ble_gap_conn_desc* description) {
    if (description == nullptr) return;
    const uint16_t handle = description->conn_handle;
    portENTER_CRITICAL(&lock_);
    if (connectionHandle_ == handle ||
        (connectPending_ && pendingConnectHandle_ == handle)) {
        pendingDisconnectHandle_ = handle;
        disconnectPending_ = true;
        clearPacketQueueLocked();
    }
    portEXIT_CRITICAL(&lock_);
}

void Esp32BleLink::onMtuChanged(
    BLEServer*,
    ble_gap_conn_desc* description,
    uint16_t mtu) {
    if (description != nullptr) queueMtu(description->conn_handle, mtu);
}

void Esp32BleLink::onWrite(
    BLECharacteristic* characteristic,
    ble_gap_conn_desc* description) {
    if (description != nullptr) {
        queuePacket(characteristic, description->conn_handle);
    }
}

void Esp32BleLink::onSubscribe(
    BLECharacteristic* characteristic,
    ble_gap_conn_desc* description,
    uint16_t subscription) {
    if (characteristic != transmit_ || description == nullptr) return;
    portENTER_CRITICAL(&lock_);
    pendingSubscribeHandle_ = description->conn_handle;
    pendingNotifyEnabled_ = (subscription & 1U) != 0U;
    subscribePending_ = true;
    portEXIT_CRITICAL(&lock_);
}

void Esp32BleLink::onAuthenticationComplete(
    ble_gap_conn_desc* description) {
    if (description == nullptr) return;
    portENTER_CRITICAL(&lock_);
    pendingSecurityHandle_ = description->conn_handle;
    pendingEncrypted_ = description->sec_state.encrypted;
    pendingBonded_ = description->sec_state.bonded;
    securityPending_ = true;
    portEXIT_CRITICAL(&lock_);
}
#endif

void Esp32BleLink::queueMtu(
    uint16_t connectionHandle,
    uint16_t mtu) {
    portENTER_CRITICAL(&lock_);
    pendingMtuHandle_ = connectionHandle;
    pendingPacketSize_ = packetSizeForMtu(mtu);
    mtuPending_ = true;
    portEXIT_CRITICAL(&lock_);
}

void Esp32BleLink::queuePacket(
    BLECharacteristic* characteristic,
    uint16_t connectionHandle) {
    if (characteristic == nullptr || connectionHandle == invalidConnectionHandle) {
        return;
    }
    const size_t size = characteristic->getLength();
    const uint8_t* data = characteristic->getData();
    if (data == nullptr || size == 0U ||
        size > BLINKER_ESP32_BLE_MAX_PACKET_SIZE) {
        return;
    }
    portENTER_CRITICAL(&lock_);
    const bool acceptedConnection =
        connectionHandle_ == connectionHandle ||
        (connectPending_ && pendingConnectHandle_ == connectionHandle);
    if (acceptedConnection &&
        packetCount_ < BLINKER_ESP32_BLE_RX_QUEUE_DEPTH) {
        QueuedPacket& packet = packets_[packetTail_];
        packet.connectionHandle = connectionHandle;
        packet.size = static_cast<uint16_t>(size);
        memcpy(packet.data, data, size);
        packetTail_ = static_cast<uint8_t>(
            (packetTail_ + 1U) % BLINKER_ESP32_BLE_RX_QUEUE_DEPTH);
        ++packetCount_;
    }
    portEXIT_CRITICAL(&lock_);
}

void Esp32BleLink::processPendingEvents() {
    bool connect = false;
    bool disconnect = false;
    bool mtu = false;
    bool subscribe = false;
    bool security = false;
    bool notifyEnabled = false;
    bool encrypted = false;
    bool bonded = false;
    uint16_t connectHandle = invalidConnectionHandle;
    uint16_t disconnectHandle = invalidConnectionHandle;
    uint16_t mtuHandle = invalidConnectionHandle;
    uint16_t subscribeHandle = invalidConnectionHandle;
    uint16_t securityHandle = invalidConnectionHandle;
    uint16_t packetSize = 20;

    portENTER_CRITICAL(&lock_);
    connect = connectPending_;
    disconnect = disconnectPending_;
    mtu = mtuPending_;
    subscribe = subscribePending_;
    security = securityPending_;
    notifyEnabled = pendingNotifyEnabled_;
    encrypted = pendingEncrypted_;
    bonded = pendingBonded_;
    connectHandle = pendingConnectHandle_;
    disconnectHandle = pendingDisconnectHandle_;
    mtuHandle = pendingMtuHandle_;
    subscribeHandle = pendingSubscribeHandle_;
    securityHandle = pendingSecurityHandle_;
    packetSize = pendingPacketSize_;
    connectPending_ = false;
    disconnectPending_ = false;
    mtuPending_ = false;
    subscribePending_ = false;
    securityPending_ = false;
    portEXIT_CRITICAL(&lock_);

    if (disconnect && connectionHandle_ == disconnectHandle &&
        session_.connected) {
        const BleSessionInfo oldSession = session_;
        session_ = BleSessionInfo();
        portENTER_CRITICAL(&lock_);
        connectionHandle_ = invalidConnectionHandle;
        portEXIT_CRITICAL(&lock_);
        if (sessionAnnounced_ && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, oldSession);
        }
        sessionAnnounced_ = false;
        if (advertising_ != nullptr) advertising_->start();
    }

    const bool connectWasAlsoDisconnected =
        disconnect && connectHandle == disconnectHandle;
    if (connect && !connectWasAlsoDisconnected && !session_.connected) {
        portENTER_CRITICAL(&lock_);
        connectionHandle_ = connectHandle;
        portEXIT_CRITICAL(&lock_);
        session_ = BleSessionInfo();
        session_.sessionId = nextSessionId();
        session_.maxPacketSize = packetSize;
        session_.connected = true;
    }

    if (mtu && session_.connected && mtuHandle == connectionHandle_) {
        session_.maxPacketSize = packetSize;
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

    if (security && !encrypted && server_ != nullptr &&
        securityHandle != invalidConnectionHandle) {
        server_->disconnect(securityHandle);
    }
}

void Esp32BleLink::drainPackets() {
    uint8_t delivered = 0;
    while (delivered < config_.maxRxPacketsPerPoll) {
        QueuedPacket packet;
        bool available = false;
        portENTER_CRITICAL(&lock_);
        if (packetCount_ != 0U) {
            packet = packets_[packetHead_];
            packetHead_ = static_cast<uint8_t>(
                (packetHead_ + 1U) % BLINKER_ESP32_BLE_RX_QUEUE_DEPTH);
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

void Esp32BleLink::clearPacketQueueLocked() {
    packetHead_ = 0;
    packetTail_ = 0;
    packetCount_ = 0;
}

void Esp32BleLink::refreshSubscription() {
    if (session_.connected && transmitDescriptor_ != nullptr) {
        session_.notifyEnabled = transmitDescriptor_->getNotifications();
    }
}

void Esp32BleLink::announceSessionIfReady() {
    if (!sessionAnnounced_ && session_.connected && session_.encrypted &&
        session_.notifyEnabled) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }
}

uint32_t Esp32BleLink::nextSessionId() {
    const uint32_t result = nextSessionId_;
    ++nextSessionId_;
    if (nextSessionId_ == 0U) ++nextSessionId_;
    return result;
}

uint16_t Esp32BleLink::packetSizeForMtu(uint16_t mtu) {
    if (mtu <= 3U) return 0U;
    uint16_t result = static_cast<uint16_t>(mtu - 3U);
    if (result > BLINKER_ESP32_BLE_MAX_PACKET_SIZE) {
        result = BLINKER_ESP32_BLE_MAX_PACKET_SIZE;
    }
    return result;
}

bool Esp32BleLink::validConfig() const {
    return config_.deviceName != nullptr && config_.deviceName[0] != '\0' &&
           config_.serviceUuid != nullptr && config_.serviceUuid[0] != '\0' &&
           config_.receiveUuid != nullptr && config_.receiveUuid[0] != '\0' &&
           config_.transmitUuid != nullptr && config_.transmitUuid[0] != '\0' &&
           strcmp(config_.serviceUuid, ble::kServiceUuid) == 0 &&
           strcmp(config_.receiveUuid, ble::kReceiveUuid) == 0 &&
           strcmp(config_.transmitUuid, ble::kTransmitUuid) == 0 &&
           config_.preferredMtu >= 23U &&
           config_.preferredMtu <= BLINKER_ESP32_BLE_MAX_PACKET_SIZE + 3U &&
           config_.maxRxPacketsPerPoll != 0U;
}

} // namespace blinker
