#include <limits.h>
#include <string.h>

namespace blinker {

inline ArduinoBleLink*& ArduinoBleLink::active() {
    static ArduinoBleLink* instance = nullptr;
    return instance;
}

inline ArduinoBleLink::ArduinoBleLink(const ArduinoBleLinkConfig& config)
    : ArduinoBleLink(nullptr, config) {}

inline ArduinoBleLink::ArduinoBleLink(
    IBleConnectionSecuritySource& security,
    const ArduinoBleLinkConfig& config)
    : ArduinoBleLink(&security, config) {}

inline ArduinoBleLink::ArduinoBleLink(
    IBleConnectionSecuritySource* security,
    const ArduinoBleLinkConfig& config)
    : config_(config),
      profile_(ble::makeDirectModeProfile()),
      modeScanResponse_(),
      service_(config.serviceUuid != nullptr ? config.serviceUuid : ""),
      receive_(
          config.receiveUuid != nullptr ? config.receiveUuid : "",
          static_cast<uint16_t>(
              BLEWrite | BLEWriteWithoutResponse |
        (config.requireEncryption
             ? static_cast<uint16_t>(BLEEncryption)
             : static_cast<uint16_t>(0U))),
          config.maxPacketSize,
          false),
      transmit_(
          config.transmitUuid != nullptr ? config.transmitUuid : "",
          static_cast<uint16_t>(
              BLERead | BLENotify |
        (config.requireEncryption
             ? static_cast<uint16_t>(BLEEncryption)
             : static_cast<uint16_t>(0U))),
          config.maxPacketSize,
          false),
      security_(security),
      central_(),
      session_(),
      packetReceiver_(nullptr),
      packetContext_(nullptr),
      connectedHandler_(nullptr),
      disconnectedHandler_(nullptr),
      sessionContext_(nullptr),
      nextSessionId_(1),
      connectedAtMillis_(0U),
      connectPending_(false),
      disconnectPending_(false),
      sessionAnnounced_(false),
      state_(BleLinkState::Stopped),
      lastError_(ErrorCode::Ok) {}

inline Result ArduinoBleLink::start() {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    if (!validConfig()) {
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    if (active() != nullptr && active() != this) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    ByteView scanResponse;
    Result profileResult = ble::encodeModeScanResponse(
        profile_,
        MutableByteSpan(modeScanResponse_, sizeof(modeScanResponse_)),
        scanResponse);
    if (!profileResult) {
        lastError_ = profileResult.code();
        state_ = BleLinkState::Error;
        return profileResult;
    }
    state_ = BleLinkState::Starting;
    if (!BLE.begin()) {
        lastError_ = ErrorCode::NotConfigured;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    active() = this;
    BLE.setPairable(config_.requireEncryption ? YES : NO);
    BLE.setEventHandler(BLEConnected, &ArduinoBleLink::connectedThunk);
    BLE.setEventHandler(BLEDisconnected, &ArduinoBleLink::disconnectedThunk);
    BLEAdvertisingData advertisementData;
    BLEAdvertisingData scanResponseData;
    if (!advertisementData.setLocalName(config_.deviceName) ||
        !advertisementData.setAdvertisedServiceUuid(config_.serviceUuid) ||
        !scanResponseData.setRawData(
            scanResponse.data,
            static_cast<int>(scanResponse.size))) {
        BLE.setEventHandler(BLEConnected, nullptr);
        BLE.setEventHandler(BLEDisconnected, nullptr);
        BLE.end();
        active() = nullptr;
        lastError_ = ErrorCode::CapacityExceeded;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    BLE.setAdvertisingData(advertisementData);
    BLE.setScanResponseData(scanResponseData);
    service_.addCharacteristic(receive_);
    service_.addCharacteristic(transmit_);
    BLE.addService(service_);
    if (!BLE.advertise()) {
        BLE.setEventHandler(BLEConnected, nullptr);
        BLE.setEventHandler(BLEDisconnected, nullptr);
        BLE.end();
        active() = nullptr;
        lastError_ = ErrorCode::ProtocolError;
        state_ = BleLinkState::Error;
        return Result::failure(lastError_);
    }
    lastError_ = ErrorCode::Ok;
    state_ = BleLinkState::Ready;
    return Result::success();
}

inline void ArduinoBleLink::stop() {
    if (state_ != BleLinkState::Stopped) {
        BLE.setEventHandler(BLEConnected, nullptr);
        BLE.setEventHandler(BLEDisconnected, nullptr);
        BLE.disconnect();
        BLE.poll();
        BLE.end();
    }
    if (active() == this) active() = nullptr;
    if (security_ != nullptr) security_->disconnected();
    session_ = BleSessionInfo();
    central_ = BLEDevice();
    connectedAtMillis_ = 0U;
    connectPending_ = false;
    disconnectPending_ = false;
    sessionAnnounced_ = false;
    state_ = BleLinkState::Stopped;
}

inline void ArduinoBleLink::poll(uint32_t) {
    if (state_ != BleLinkState::Ready) return;
    BLE.poll();
    updateConnection();
    if (!session_.connected) return;
    if (config_.requireEncryption) {
        session_.encrypted = security_->encrypted();
        session_.bonded = session_.encrypted && security_->bonded();
        if (sessionAnnounced_ && !session_.encrypted) {
            const BleSessionInfo oldSession = session_;
            sessionAnnounced_ = false;
            if (disconnectedHandler_ != nullptr) {
                disconnectedHandler_(sessionContext_, oldSession);
            }
            central_.disconnect();
            return;
        }
    }
    session_.notifyEnabled = transmit_.subscribed();
    if (!sessionAnnounced_ &&
        static_cast<uint32_t>(millis() - connectedAtMillis_) >=
            config_.sessionReadyTimeoutMillis) {
        central_.disconnect();
        return;
    }
    if (!sessionAnnounced_ && session_.notifyEnabled &&
        (!config_.requireEncryption || session_.encrypted)) {
        sessionAnnounced_ = true;
        if (connectedHandler_ != nullptr) {
            connectedHandler_(sessionContext_, session_);
        }
    }
    if (!sessionAnnounced_) return;
    if (receive_.written() && packetReceiver_ != nullptr) {
        const int length = receive_.valueLength();
        const uint8_t* value = receive_.value();
        if (length > 0 && value != nullptr &&
            static_cast<uint16_t>(length) <= config_.maxPacketSize) {
            packetReceiver_(
                packetContext_,
                session_,
                ByteView(value, static_cast<size_t>(length)));
        }
    }
}

inline BleLinkState ArduinoBleLink::state() const { return state_; }
inline ErrorCode ArduinoBleLink::lastError() const { return lastError_; }
inline size_t ArduinoBleLink::sessionCount() const {
    return sessionAnnounced_ ? 1U : 0U;
}

inline Result ArduinoBleLink::sessionAt(
    size_t index,
    BleSessionInfo& session) const {
    if (index != 0U || !sessionAnnounced_) {
        return Result::failure(ErrorCode::NotFound);
    }
    session = session_;
    return Result::success();
}

inline Result ArduinoBleLink::sendPacket(
    uint32_t sessionId,
    ByteView packet) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId ||
        !transmit_.subscribed() ||
        (config_.requireEncryption && !session_.encrypted)) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (packet.data == nullptr || packet.size == 0U ||
        packet.size > config_.maxPacketSize || packet.size > INT_MAX) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return transmit_.writeValue(
               packet.data,
               static_cast<int>(packet.size))
               ? Result::success()
               : Result::failure(ErrorCode::WouldBlock);
}

inline Result ArduinoBleLink::disconnectSession(uint32_t sessionId) {
    if (!sessionAnnounced_ || session_.sessionId != sessionId || !central_ ||
        !central_.connected()) {
        return Result::failure(ErrorCode::NotFound);
    }
    central_.disconnect();
    return Result::success();
}

inline void ArduinoBleLink::setPacketReceiver(
    BlePacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

inline void ArduinoBleLink::setSessionHandlers(
    BleSessionHandler connected,
    BleSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

inline Result ArduinoBleLink::configureBleProfile(
    const ble::ModeProfile& profile) {
    if (state_ != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::StateConflict);
    }
    Result result = ble::validateModeProfile(profile);
    if (!result) return result;
    profile_ = profile;
    return Result::success();
}

inline void ArduinoBleLink::updateConnection() {
    const bool connected = static_cast<bool>(central_) && central_.connected();
    const bool connectionCycle = connectPending_ && disconnectPending_;
    if (session_.connected &&
        (!connected || disconnectPending_ || connectionCycle)) {
        const BleSessionInfo oldSession = session_;
        session_ = BleSessionInfo();
        connectedAtMillis_ = 0U;
        if (sessionAnnounced_ && disconnectedHandler_ != nullptr) {
            disconnectedHandler_(sessionContext_, oldSession);
        }
        sessionAnnounced_ = false;
        if (!connected) {
            central_ = BLEDevice();
            BLE.advertise();
        }
    }
    if (!session_.connected && connected &&
        (connectPending_ || connectionCycle)) {
        session_ = BleSessionInfo();
        session_.sessionId = nextSessionId();
        session_.maxPacketSize = config_.maxPacketSize;
        session_.connected = true;
        connectedAtMillis_ = millis();
        session_.notifyEnabled = transmit_.subscribed();
    }
    connectPending_ = false;
    disconnectPending_ = false;
}

inline void ArduinoBleLink::connectedThunk(BLEDevice device) {
    if (active() != nullptr) active()->onConnected(device);
}

inline void ArduinoBleLink::disconnectedThunk(BLEDevice device) {
    if (active() != nullptr) active()->onDisconnected(device);
}

inline void ArduinoBleLink::onConnected(BLEDevice device) {
    if (!device) return;
    if ((session_.connected || connectPending_) && central_ &&
        central_ != device) {
        device.disconnect();
        return;
    }
    central_ = device;
    if (security_ != nullptr) {
        const String address = device.address();
        security_->connected(StringView(address.c_str(), address.length()));
    }
    connectPending_ = true;
}

inline void ArduinoBleLink::onDisconnected(BLEDevice device) {
    if (central_ && central_ == device) {
        if (security_ != nullptr) security_->disconnected();
        disconnectPending_ = true;
    }
}

inline uint32_t ArduinoBleLink::nextSessionId() {
    const uint32_t result = nextSessionId_;
    ++nextSessionId_;
    if (nextSessionId_ == 0U) ++nextSessionId_;
    return result;
}

inline bool ArduinoBleLink::validConfig() const {
    return config_.deviceName != nullptr && config_.deviceName[0] != '\0' &&
           strlen(config_.deviceName) <= ble::kLegacyLocalNameMaxSize &&
           config_.serviceUuid != nullptr && config_.serviceUuid[0] != '\0' &&
           config_.receiveUuid != nullptr && config_.receiveUuid[0] != '\0' &&
           config_.transmitUuid != nullptr && config_.transmitUuid[0] != '\0' &&
           strcmp(config_.serviceUuid, ble::kServiceUuid) == 0 &&
           strcmp(config_.receiveUuid, ble::kReceiveUuid) == 0 &&
           strcmp(config_.transmitUuid, ble::kTransmitUuid) == 0 &&
           config_.maxPacketSize >= ble::kMinimumPacketSize &&
           config_.sessionReadyTimeoutMillis != 0U &&
           (!config_.requireEncryption || security_ != nullptr);
}

} // namespace blinker
