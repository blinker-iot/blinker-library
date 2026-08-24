#include "BleNoiseProvisioningChannel.h"

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

Result validateLocalRecord(ByteView record) {
    noise::LocalSecureRecordView parsed;
    return noise::parseLocalSecureRecord(record, parsed);
}

bool noiseChannelOverlaps(ByteView input, MutableByteSpan storage) {
    if (input.empty() || storage.empty() || input.data == nullptr ||
        storage.data == nullptr) {
        return false;
    }
    const uintptr_t inputBegin = reinterpret_cast<uintptr_t>(input.data);
    const uintptr_t storageBegin = reinterpret_cast<uintptr_t>(storage.data);
    if (input.size > UINTPTR_MAX - inputBegin ||
        storage.size > UINTPTR_MAX - storageBegin) {
        return true;
    }
    const uintptr_t inputEnd = inputBegin + input.size;
    const uintptr_t storageEnd = storageBegin + storage.size;
    return inputBegin < storageEnd && storageBegin < inputEnd;
}

} // namespace

BleRecordFormat BleNoiseProvisioningChannel::recordFormat() {
    BleRecordFormat format;
    format.minimumHeaderSize =
        static_cast<uint16_t>(noise::kLocalSecureRecordHeaderSize);
    format.decodeSize = &noise::decodeLocalSecureRecordSize;
    format.validate = &validateLocalRecord;
    return format;
}

BleRecordTransportConfig BleNoiseProvisioningChannel::recordConfig(
    const BleNoiseProvisioningConfig& config) {
    BleRecordTransportConfig records;
    records.maxRecordSize = static_cast<uint16_t>(kBleNoiseRecordSize);
    records.reassemblyTimeoutMillis = config.reassemblyTimeoutMillis;
    records.maxPacketsPerPoll = config.maxPacketsPerPoll;
    records.sessionCapacity = 1U;
    records.txRecordCapacity = 1U;
    return records;
}

BleNoiseProvisioningChannel::BleNoiseProvisioningChannel(
    IBleLink& link,
    IClock& clock,
    INoiseCryptoProvider& crypto,
    IRandom& random,
    MutableByteSpan rxStorage,
    MutableByteSpan txStorage,
    MutableByteSpan packetScratch,
    MutableByteSpan plaintextScratch,
    const BleNoiseProvisioningConfig& config)
    : clock_(clock), rxStorage_(rxStorage), txStorage_(txStorage),
      plaintextScratch_(plaintextScratch), config_(config),
      records_(link,
               clock,
               recordFormat(),
               rxStorage,
               txStorage,
               packetScratch,
               recordConfig(config)),
      noise_(crypto, random), receiver_(nullptr), receiverContext_(nullptr),
      sessionConnected_(nullptr), sessionDisconnected_(nullptr),
      sessionContext_(nullptr), faultHandler_(nullptr), faultContext_(nullptr),
      activeSessionId_(0U), lastActivityMillis_(0U), armed_(false) {
    records_.setReceiver(&BleNoiseProvisioningChannel::recordThunk, this);
    records_.setSessionHandlers(&BleNoiseProvisioningChannel::connectedThunk,
                                &BleNoiseProvisioningChannel::disconnectedThunk,
                                this);
    records_.setFaultHandler(&BleNoiseProvisioningChannel::faultThunk, this);
}

BleNoiseProvisioningChannel::~BleNoiseProvisioningChannel() {
    clearSecureState(false);
}

Result BleNoiseProvisioningChannel::start() {
    if (!noise::validNoiseNnPattern(config_.pattern) ||
        config_.secureSessionTimeoutMillis == 0U ||
        plaintextScratch_.data == nullptr ||
        plaintextScratch_.size < kBleNoiseMaxTransportPayloadSize) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    return records_.start();
}

void BleNoiseProvisioningChannel::stop() {
    clearSecureState(true);
    records_.stop();
    activeSessionId_ = 0U;
}

void BleNoiseProvisioningChannel::poll(uint32_t budgetMicros) {
    records_.poll(budgetMicros);
    if (!armed_)
        return;
    const uint32_t now = clock_.monotonicMillis();
    if (static_cast<uint32_t>(now - lastActivityMillis_) <
        config_.secureSessionTimeoutMillis) {
        return;
    }
    RxContext rx;
    rx.sessionId = activeSessionId_;
    failActive(ErrorCode::ProtocolError, rx);
}

Result BleNoiseProvisioningChannel::arm(ByteView psk) {
    clearSecureState(true);
    Result result =
        noise_.begin(security::NoiseRole::Responder,
                     config_.pattern,
                     psk,
                     noise::localSecureNoisePrologue(config_.pattern));
    if (!result) {
        noise_.clear();
        return result;
    }
    armed_ = true;
    lastActivityMillis_ = clock_.monotonicMillis();
    return Result::success();
}

void BleNoiseProvisioningChannel::cancel() { clearSecureState(true); }

Result BleNoiseProvisioningChannel::sendHandshakeResponse(ByteView payload) {
    return queueHandshake(payload);
}

Result BleNoiseProvisioningChannel::sendTransport(ByteView plaintext) {
    return queueTransport(plaintext);
}

Result BleNoiseProvisioningChannel::handshakeHash(ByteView& output) const {
    return noise_.handshakeHash(output);
}

void BleNoiseProvisioningChannel::setReceiver(BleNoisePayloadReceiver receiver,
                                              void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void BleNoiseProvisioningChannel::setSessionHandlers(
    FrameSessionHandler connected,
    FrameSessionHandler disconnected,
    void* context) {
    sessionConnected_ = connected;
    sessionDisconnected_ = disconnected;
    sessionContext_ = context;
}

void BleNoiseProvisioningChannel::setFaultHandler(BleNoiseFaultHandler handler,
                                                  void* context) {
    faultHandler_ = handler;
    faultContext_ = context;
}

void BleNoiseProvisioningChannel::recordThunk(void* context,
                                              ByteView record,
                                              const RxContext& rx) {
    BleNoiseProvisioningChannel* channel =
        static_cast<BleNoiseProvisioningChannel*>(context);
    if (channel != nullptr)
        channel->onRecord(record, rx);
}

void BleNoiseProvisioningChannel::connectedThunk(void* context,
                                                 const RxContext& rx) {
    BleNoiseProvisioningChannel* channel =
        static_cast<BleNoiseProvisioningChannel*>(context);
    if (channel != nullptr)
        channel->onConnected(rx);
}

void BleNoiseProvisioningChannel::disconnectedThunk(void* context,
                                                    const RxContext& rx) {
    BleNoiseProvisioningChannel* channel =
        static_cast<BleNoiseProvisioningChannel*>(context);
    if (channel != nullptr)
        channel->onDisconnected(rx);
}

void BleNoiseProvisioningChannel::faultThunk(void* context,
                                             ErrorCode error,
                                             const RxContext& rx) {
    BleNoiseProvisioningChannel* channel =
        static_cast<BleNoiseProvisioningChannel*>(context);
    if (channel != nullptr)
        channel->onRecordFault(error, rx);
}

void BleNoiseProvisioningChannel::onRecord(ByteView encoded,
                                           const RxContext& rx) {
    if (!armed_ || activeSessionId_ == 0U || rx.sessionId != activeSessionId_) {
        if (rx.sessionId == activeSessionId_ || activeSessionId_ == 0U) {
            failActive(ErrorCode::AuthenticationRequired, rx);
        } else {
            emitFault(ErrorCode::CapacityExceeded, rx);
        }
        return;
    }

    noise::LocalSecureRecordView record;
    Result result = noise::parseLocalSecureRecord(encoded, record);
    ByteView payload;
    BleNoisePayloadKind kind = BleNoisePayloadKind::Transport;
    if (result &&
        record.type == noise::LocalSecureRecordType::InitiatorHandshake &&
        noise_.state() == security::NoiseSessionState::ReadInitiatorHandshake) {
        result = noise_.readHandshake(record.body, plaintextScratch_, payload);
        kind = BleNoisePayloadKind::InitiatorHandshake;
    } else if (result &&
               record.type == noise::LocalSecureRecordType::Transport &&
               noise_.state() == security::NoiseSessionState::Transport) {
        result =
            noise_.decryptTransport(record.body, plaintextScratch_, payload);
        kind = BleNoisePayloadKind::Transport;
    } else if (result) {
        result = Result::failure(ErrorCode::ProtocolError);
    }

    if (!result) {
        failActive(result.code(), rx);
        return;
    }
    lastActivityMillis_ = clock_.monotonicMillis();
    if (receiver_ != nullptr) {
        receiver_(receiverContext_, kind, payload, rx);
    }
    secureZero(plaintextScratch_);
    secureZero(rxStorage_);
}

void BleNoiseProvisioningChannel::onConnected(const RxContext& rx) {
    if (activeSessionId_ == 0U) {
        activeSessionId_ = rx.sessionId;
        if (armed_)
            lastActivityMillis_ = clock_.monotonicMillis();
    }
    if (sessionConnected_ != nullptr) {
        sessionConnected_(sessionContext_, rx);
    }
}

void BleNoiseProvisioningChannel::onDisconnected(const RxContext& rx) {
    if (rx.sessionId == activeSessionId_) {
        clearSecureState(false);
        activeSessionId_ = 0U;
    }
    if (sessionDisconnected_ != nullptr) {
        sessionDisconnected_(sessionContext_, rx);
    }
}

void BleNoiseProvisioningChannel::onRecordFault(ErrorCode error,
                                                const RxContext& rx) {
    if (activeSessionId_ != 0U && rx.sessionId != activeSessionId_) {
        emitFault(error, rx);
        return;
    }
    failActive(error, rx);
}

Result BleNoiseProvisioningChannel::targetForActive(SendTarget& target) const {
    target = SendTarget();
    if (!armed_ || activeSessionId_ == 0U) {
        return Result::failure(ErrorCode::NotConnected);
    }
    target.kind = SendTargetKind::Session;
    target.sessionId = activeSessionId_;
    return Result::success();
}

Result BleNoiseProvisioningChannel::queueHandshake(ByteView payload) {
    if ((payload.data == nullptr && !payload.empty()) ||
        noiseChannelOverlaps(payload, txStorage_)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (payload.size > kBleNoiseMaxHandshakePayloadSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (noise_.state() !=
        security::NoiseSessionState::WriteResponderHandshake) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    SendTarget target;
    Result result = targetForActive(target);
    size_t messageSize = 0U;
    if (result) {
        result = noise_.handshakeMessageSize(payload.size, messageSize);
    }
    const size_t recordSize = noise::kLocalSecureRecordHeaderSize + messageSize;
    if (result)
        result = records_.canQueue(recordSize, target);
    bool noiseAttempted = false;
    ByteView message;
    if (result) {
        secureZero(txStorage_);
        noiseAttempted = true;
        result = noise_.writeHandshake(
            payload,
            MutableByteSpan(
                txStorage_.data + noise::kLocalSecureRecordHeaderSize,
                txStorage_.size - noise::kLocalSecureRecordHeaderSize),
            message);
    }
    ByteView encoded;
    if (result) {
        result = noise::encodeLocalSecureRecord(
            noise::LocalSecureRecordType::ResponderHandshake,
            message,
            txStorage_,
            encoded);
    }
    if (result)
        result = records_.send(encoded, target);
    if (!result && noiseAttempted) {
        clearSecureState(true);
    }
    if (result)
        lastActivityMillis_ = clock_.monotonicMillis();
    return result;
}

Result BleNoiseProvisioningChannel::queueTransport(ByteView plaintext) {
    if ((plaintext.data == nullptr && !plaintext.empty()) ||
        noiseChannelOverlaps(plaintext, txStorage_)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (plaintext.size > kBleNoiseMaxTransportPayloadSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (noise_.state() != security::NoiseSessionState::Transport) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    SendTarget target;
    Result result = targetForActive(target);
    const size_t recordSize = noise::kLocalSecureRecordHeaderSize +
                              security::kNoiseTagSize + plaintext.size;
    if (result)
        result = records_.canQueue(recordSize, target);
    bool noiseAttempted = false;
    ByteView ciphertext;
    if (result) {
        secureZero(txStorage_);
        noiseAttempted = true;
        result = noise_.encryptTransport(
            plaintext,
            MutableByteSpan(
                txStorage_.data + noise::kLocalSecureRecordHeaderSize,
                txStorage_.size - noise::kLocalSecureRecordHeaderSize),
            ciphertext);
    }
    ByteView encoded;
    if (result) {
        result = noise::encodeLocalSecureRecord(
            noise::LocalSecureRecordType::Transport,
            ciphertext,
            txStorage_,
            encoded);
    }
    if (result)
        result = records_.send(encoded, target);
    if (!result && noiseAttempted) {
        clearSecureState(true);
    }
    if (result)
        lastActivityMillis_ = clock_.monotonicMillis();
    return result;
}

void BleNoiseProvisioningChannel::clearSecureState(bool resetRecord) {
    if (resetRecord && activeSessionId_ != 0U) {
        records_.resetSession(activeSessionId_);
    }
    noise_.clear();
    armed_ = false;
    lastActivityMillis_ = 0U;
    wipeWorkspaces();
}

void BleNoiseProvisioningChannel::failActive(ErrorCode error,
                                             const RxContext& rx) {
    clearSecureState(true);
    emitFault(error, rx);
}

void BleNoiseProvisioningChannel::emitFault(ErrorCode error,
                                            const RxContext& rx) {
    if (faultHandler_ != nullptr) {
        faultHandler_(faultContext_, error, rx);
    }
}

void BleNoiseProvisioningChannel::wipeWorkspaces() {
    secureZero(rxStorage_);
    secureZero(txStorage_);
    secureZero(plaintextScratch_);
}

} // namespace blinker
