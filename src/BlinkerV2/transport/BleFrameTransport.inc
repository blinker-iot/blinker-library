#include "BleFrameTransport.h"

namespace blinker {

BleFrameTransport::BleFrameTransport(
    IBleLink& link,
    IClock& clock,
    IX25519AesGcmCryptoProvider& crypto,
    MutableByteSpan rxStorage,
    MutableByteSpan txStorage,
    MutableByteSpan packetScratch,
    MutableByteSpan plaintextScratch,
    const BleFrameTransportConfig& config)
    : records_(
          link,
          clock,
          frameFormat(),
          rxStorage,
          txStorage,
          packetScratch,
          recordConfig(config)),
      crypto_(crypto),
      plaintextScratch_(plaintextScratch),
      maxFrameSize_(config.maxFrameSize),
      secure_(),
      receiver_(nullptr),
      receiverContext_(nullptr),
      sessionConnected_(nullptr),
      sessionDisconnected_(nullptr),
      sessionContext_(nullptr) {
    records_.setReceiver(&BleFrameTransport::recordThunk, this);
    records_.setSessionHandlers(
        &BleFrameTransport::connectedThunk,
        &BleFrameTransport::disconnectedThunk,
        this);
    records_.setFaultHandler(&BleFrameTransport::faultThunk, this);
}

Result BleFrameTransport::start() {
    if (maxFrameSize_ < bbp2::kBaseHeaderSize ||
        plaintextScratch_.data == nullptr ||
        plaintextScratch_.size < maxFrameSize_) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    clearSessions();
    return records_.start();
}

void BleFrameTransport::stop() {
    records_.stop();
    clearSessions();
}

void BleFrameTransport::poll(uint32_t budgetMicros) {
    records_.poll(budgetMicros);
}

TransportState BleFrameTransport::state() const {
    return records_.state();
}

TransportCapabilities BleFrameTransport::capabilities() const {
    TransportCapabilities result;
    result.maxFrameSize = maxFrameSize_;
    result.features = TransportFeatureBinary |
                      TransportFeatureLocal |
                      TransportFeatureFragmented;
    return result;
}

Result BleFrameTransport::send(
    ByteView frame,
    const SendTarget& target) {
    Result result = validateFrame(frame);
    if (!result || frame.size > maxFrameSize_) {
        return result ? Result::failure(ErrorCode::CapacityExceeded) : result;
    }

    if (target.kind != SendTargetKind::Session) {
        return hasSecureSession()
                   ? Result::failure(ErrorCode::AuthenticationRequired)
                   : records_.send(frame, target);
    }

    SecureSlot* slot = findSecure(target.sessionId, false);
    if (slot == nullptr || slot->state != SecureState::Active) {
        return records_.send(frame, target);
    }

    const size_t recordSize = security::directSecureRecordSize(frame.size);
    if (recordSize == 0U) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    SealContext context;
    context.owner = this;
    context.slot = slot;
    context.frame = frame;
    result = records_.sendPrepared(
        recordSize,
        target,
        &BleFrameTransport::sealThunk,
        &context);
    if (!result && context.attempted) {
        retire(target.sessionId, result.code());
    }
    return result;
}

void BleFrameTransport::setReceiver(
    FrameReceiver receiver,
    void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void BleFrameTransport::setSessionHandlers(
    FrameSessionHandler connected,
    FrameSessionHandler disconnected,
    void* context) {
    sessionConnected_ = connected;
    sessionDisconnected_ = disconnected;
    sessionContext_ = context;
}

Result BleFrameTransport::prepare(
    uint32_t sessionId,
    ByteView controllerSecret,
    const security::DirectSecureContext& context) {
    if (sessionId == 0U || !records_.hasSession(sessionId)) {
        return Result::failure(ErrorCode::NotConnected);
    }
    SecureSlot* slot = findSecure(sessionId, true);
    if (slot == nullptr) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (slot->state == SecureState::Active) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    slot->session.clear();
    Result result = slot->session.begin(
        security::DirectSecureRole::Responder,
        controllerSecret,
        context);
    if (!result) {
        slot->clear();
        return result;
    }
    slot->state = SecureState::Prepared;
    return Result::success();
}

void BleFrameTransport::commit(uint32_t sessionId) {
    SecureSlot* slot = findSecure(sessionId, false);
    if (slot == nullptr || slot->state != SecureState::Prepared ||
        !slot->session.ready()) {
        retire(sessionId, ErrorCode::ProtocolError);
        return;
    }
    slot->state = SecureState::Active;
}

void BleFrameTransport::cancel(uint32_t sessionId) {
    SecureSlot* slot = findSecure(sessionId, false);
    if (slot != nullptr) slot->clear();
}

BleRecordFormat BleFrameTransport::frameFormat() {
    BleRecordFormat format;
    format.minimumHeaderSize = security::kDirectSecureHeaderSize;
    format.decodeSize = &BleFrameTransport::decodeFrameSize;
    format.validate = &BleFrameTransport::validateFrame;
    return format;
}

BleRecordTransportConfig BleFrameTransport::recordConfig(
    const BleFrameTransportConfig& config) {
    BleRecordTransportConfig result;
    const size_t maximum = security::directSecureRecordSize(
        config.maxFrameSize);
    result.maxRecordSize = maximum <= UINT16_MAX
                               ? static_cast<uint16_t>(maximum)
                               : 0U;
    result.reassemblyTimeoutMillis = config.reassemblyTimeoutMillis;
    result.maxPacketsPerPoll = config.maxPacketsPerPoll;
    return result;
}

Result BleFrameTransport::decodeFrameSize(
    ByteView prefix,
    size_t& frameSize) {
    frameSize = 0U;
    if (prefix.data == nullptr || prefix.empty()) {
        return Result::failure(ErrorCode::TruncatedInput);
    }
    if (prefix.data[0] == security::kDirectSecureMagic) {
        return security::decodeDirectSecureRecordSize(prefix, frameSize);
    }
    bbp2::FrameHeader header;
    Result result = bbp2::decodeHeader(prefix, header);
    if (!result) return result;
    frameSize = bbp2::encodedFrameSize(header);
    return Result::success();
}

Result BleFrameTransport::validateFrame(ByteView frame) {
    if (frame.data == nullptr || frame.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (frame.data[0] == security::kDirectSecureMagic) {
        security::DirectSecureRecordView parsed;
        return security::parseDirectSecureRecord(frame, parsed);
    }
    bbp2::FrameView parsed;
    return bbp2::parseFrame(frame, parsed);
}

void BleFrameTransport::recordThunk(
    void* context,
    ByteView record,
    const RxContext& rx) {
    BleFrameTransport* transport =
        static_cast<BleFrameTransport*>(context);
    if (transport != nullptr) transport->onRecord(record, rx);
}

void BleFrameTransport::connectedThunk(
    void* context,
    const RxContext& rx) {
    BleFrameTransport* transport =
        static_cast<BleFrameTransport*>(context);
    if (transport != nullptr) transport->onConnected(rx);
}

void BleFrameTransport::disconnectedThunk(
    void* context,
    const RxContext& rx) {
    BleFrameTransport* transport =
        static_cast<BleFrameTransport*>(context);
    if (transport != nullptr) transport->onDisconnected(rx);
}

void BleFrameTransport::faultThunk(
    void* context,
    ErrorCode error,
    const RxContext& rx) {
    BleFrameTransport* transport =
        static_cast<BleFrameTransport*>(context);
    if (transport != nullptr) transport->onFault(error, rx);
}

Result BleFrameTransport::sealThunk(
    void* context,
    MutableByteSpan storage,
    ByteView& record) {
    SealContext* seal = static_cast<SealContext*>(context);
    if (seal == nullptr || seal->owner == nullptr || seal->slot == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    seal->attempted = true;
    return seal->slot->session.seal(
        seal->owner->crypto_,
        seal->frame,
        storage,
        record);
}

void BleFrameTransport::onRecord(
    ByteView record,
    const RxContext& rx) {
    const bool secureRecord =
        record.data != nullptr && !record.empty() &&
        record.data[0] == security::kDirectSecureMagic;
    SecureSlot* slot = findSecure(rx.sessionId, false);
    const bool active =
        slot != nullptr && slot->state == SecureState::Active;
    if (active != secureRecord) {
        retire(rx.sessionId, ErrorCode::AuthenticationRequired);
        return;
    }

    ByteView frame = record;
    if (active) {
        Result result = slot->session.open(
            crypto_,
            record,
            plaintextScratch_,
            frame);
        if (result) result = validateFrame(frame);
        if (!result || frame.size > maxFrameSize_) {
            retire(
                rx.sessionId,
                result ? ErrorCode::CapacityExceeded : result.code());
            return;
        }
    }
    if (receiver_ != nullptr) receiver_(receiverContext_, frame, rx);
}

void BleFrameTransport::onConnected(const RxContext& rx) {
    cancel(rx.sessionId);
    if (sessionConnected_ != nullptr) {
        sessionConnected_(sessionContext_, rx);
    }
}

void BleFrameTransport::onDisconnected(const RxContext& rx) {
    cancel(rx.sessionId);
    if (sessionDisconnected_ != nullptr) {
        sessionDisconnected_(sessionContext_, rx);
    }
}

void BleFrameTransport::onFault(
    ErrorCode error,
    const RxContext& rx) {
    SecureSlot* slot = findSecure(rx.sessionId, false);
    if (slot != nullptr && slot->state != SecureState::Empty) {
        retire(rx.sessionId, error);
    }
}

void BleFrameTransport::retire(
    uint32_t sessionId,
    ErrorCode error) {
    cancel(sessionId);
    records_.failSession(sessionId, error);
}

void BleFrameTransport::clearSessions() {
    for (size_t index = 0U; index < BLINKER_BLE_MAX_SESSIONS; ++index) {
        secure_[index].clear();
    }
}

bool BleFrameTransport::hasSecureSession() const {
    for (size_t index = 0U; index < BLINKER_BLE_MAX_SESSIONS; ++index) {
        if (secure_[index].state != SecureState::Empty) return true;
    }
    return false;
}

BleFrameTransport::SecureSlot* BleFrameTransport::findSecure(
    uint32_t sessionId,
    bool create) {
    SecureSlot* empty = nullptr;
    for (size_t index = 0U; index < BLINKER_BLE_MAX_SESSIONS; ++index) {
        if (secure_[index].state != SecureState::Empty &&
            secure_[index].sessionId == sessionId) {
            return &secure_[index];
        }
        if (secure_[index].state == SecureState::Empty && empty == nullptr) {
            empty = &secure_[index];
        }
    }
    if (!create || empty == nullptr) return nullptr;
    empty->sessionId = sessionId;
    return empty;
}

const BleFrameTransport::SecureSlot* BleFrameTransport::findSecure(
    uint32_t sessionId) const {
    for (size_t index = 0U; index < BLINKER_BLE_MAX_SESSIONS; ++index) {
        if (secure_[index].state != SecureState::Empty &&
            secure_[index].sessionId == sessionId) {
            return &secure_[index];
        }
    }
    return nullptr;
}

} // namespace blinker
