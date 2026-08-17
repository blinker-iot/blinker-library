#include "BleRecordTransport.h"

#include <string.h>

namespace blinker {

BleRecordTransport::BleRecordTransport(
    IBleLink& link,
    IClock& clock,
    const BleRecordFormat& format,
    MutableByteSpan rxStorage,
    MutableByteSpan txStorage,
    MutableByteSpan packetScratch,
    const BleRecordTransportConfig& config)
    : link_(link),
      clock_(clock),
      format_(format),
      rxStorage_(rxStorage),
      txStorage_(txStorage),
      packetScratch_(packetScratch),
      config_(config),
      rxSessions_(),
      txRecords_(),
      txHead_(0U),
      txTail_(0U),
      txCount_(0U),
      receiver_(nullptr),
      receiverContext_(nullptr),
      sessionConnected_(nullptr),
      sessionDisconnected_(nullptr),
      sessionContext_(nullptr),
      faultHandler_(nullptr),
      faultContext_(nullptr),
      droppedFragments_(0U),
      completedRecords_(0U),
      started_(false) {}

Result BleRecordTransport::start() {
    if (started_) return Result::success();
    if (format_.minimumHeaderSize == 0U ||
        format_.decodeSize == nullptr || format_.validate == nullptr ||
        config_.maxRecordSize < format_.minimumHeaderSize ||
        config_.maxRecordSize > ble::kMaximumRecordSizeAtMinimumPacket ||
        config_.reassemblyTimeoutMillis == 0U ||
        config_.maxPacketsPerPoll == 0U ||
        config_.sessionCapacity == 0U ||
        config_.sessionCapacity > BLINKER_BLE_MAX_SESSIONS ||
        config_.txRecordCapacity == 0U ||
        config_.txRecordCapacity > BLINKER_BLE_MAX_TX_FRAMES ||
        rxStorage_.data == nullptr ||
        rxStorage_.size <
            static_cast<size_t>(config_.maxRecordSize) *
                config_.sessionCapacity ||
        txStorage_.data == nullptr || txCapacity() == 0U ||
        packetScratch_.data == nullptr ||
        packetScratch_.size <
            static_cast<size_t>(ble::kFragmentHeaderSize) +
                format_.minimumHeaderSize) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    // Refuse before touching callbacks when another logical endpoint already
    // owns the physical link. This keeps an accidental second start from
    // clearing or replacing the live endpoint's callback set.
    if (link_.state() != BleLinkState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    // Binding belongs to start(), not construction. Multiple logical BLE
    // endpoints may therefore be constructed over one link without the last
    // constructor silently stealing callbacks from the active endpoint.
    link_.setPacketReceiver(&BleRecordTransport::packetThunk, this);
    link_.setSessionHandlers(
        &BleRecordTransport::connectedThunk,
        &BleRecordTransport::disconnectedThunk,
        this);
    Result result = link_.start();
    if (!result) {
        link_.setPacketReceiver(nullptr, nullptr);
        link_.setSessionHandlers(nullptr, nullptr, nullptr);
        // start() acquired the stopped link before installing callbacks. A
        // platform start may fail after partially initializing its stack, so
        // return the physical link to the stopped/configurable state here.
        link_.stop();
        return result;
    }
    started_ = true;
    return Result::success();
}

void BleRecordTransport::stop() {
    // Only the endpoint whose start() succeeded owns the callbacks/link.
    // Calling stop() on another constructed or failed endpoint must not tear
    // down the active owner's session.
    if (!started_) return;
    // Release callbacks before platform shutdown; a link must never emit a
    // disconnect into a logical endpoint that is already being retired.
    link_.setPacketReceiver(nullptr, nullptr);
    link_.setSessionHandlers(nullptr, nullptr, nullptr);
    link_.stop();
    for (size_t index = 0U; index < BLINKER_BLE_MAX_SESSIONS; ++index) {
        rxSessions_[index] = RxSession();
    }
    for (size_t index = 0U; index < BLINKER_BLE_MAX_TX_FRAMES; ++index) {
        txRecords_[index] = TxRecord();
    }
    txHead_ = 0U;
    txTail_ = 0U;
    txCount_ = 0U;
    started_ = false;
}

void BleRecordTransport::poll(uint32_t budgetMicros) {
    if (!started_) return;
    link_.poll(budgetMicros / 2U);
    reconcileSessions();
    expireReassembly(clock_.monotonicMillis());
    flushTx();
}

TransportState BleRecordTransport::state() const {
    if (!started_ || link_.state() == BleLinkState::Stopped) {
        return TransportState::Stopped;
    }
    if (link_.state() == BleLinkState::Starting) {
        return TransportState::Starting;
    }
    if (link_.state() == BleLinkState::Error) {
        return TransportState::Error;
    }
    return TransportState::Online;
}

Result BleRecordTransport::canSend(
    ByteView record,
    const SendTarget& target) const {
    if (record.data == nullptr ||
        record.size < format_.minimumHeaderSize ||
        record.size > config_.maxRecordSize || record.size > UINT16_MAX) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = format_.validate(record);
    if (!result) return result;

    return canQueue(record.size, target);
}

Result BleRecordTransport::canQueue(
    size_t recordSize,
    const SendTarget& target) const {
    if (!started_ || state() != TransportState::Online) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (recordSize < format_.minimumHeaderSize ||
        recordSize > config_.maxRecordSize || recordSize > UINT16_MAX) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    size_t recipientCount = 0U;
    const size_t count = link_.sessionCount();
    for (size_t index = 0U; index < count; ++index) {
        BleSessionInfo session;
        Result result = link_.sessionAt(index, session);
        if (!result || !usable(session)) continue;
        const RxSession* admitted = findRx(session.sessionId);
        if (admitted == nullptr) {
            if (target.kind == SendTargetKind::Session &&
                session.sessionId == target.sessionId) {
                return Result::failure(ErrorCode::NotConnected);
            }
            // Broadcast must remain atomic. A physical peer which could not
            // be admitted (capacity or quarantine) cannot be silently omitted.
            if (target.kind != SendTargetKind::Session) {
                return Result::failure(ErrorCode::CapacityExceeded);
            }
            continue;
        }
        if (securityDowngraded(admitted->info, session)) {
            if (target.kind == SendTargetKind::Session &&
                session.sessionId == target.sessionId) {
                return Result::failure(ErrorCode::NotConnected);
            }
            if (target.kind != SendTargetKind::Session) {
                return Result::failure(ErrorCode::AuthenticationRequired);
            }
            continue;
        }
        if (target.kind == SendTargetKind::Session &&
            session.sessionId != target.sessionId) {
            continue;
        }
        ++recipientCount;
        const size_t payloadCapacity = packetPayloadCapacity(session);
        if (payloadCapacity < format_.minimumHeaderSize ||
            fragmentCount(recordSize, payloadCapacity) >
                ble::kMaximumFragmentCount) {
            return Result::failure(ErrorCode::BufferTooSmall);
        }
    }
    if (recipientCount == 0U) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (target.kind == SendTargetKind::Session && recipientCount != 1U) {
        return Result::failure(ErrorCode::NotFound);
    }
    if (recipientCount > txCapacity() - txCount_) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    return Result::success();
}

Result BleRecordTransport::send(
    ByteView record,
    const SendTarget& target) {
    Result result = canSend(record, target);
    if (!result) return result;

    const size_t count = link_.sessionCount();
    for (size_t index = 0U; index < count; ++index) {
        BleSessionInfo session;
        result = link_.sessionAt(index, session);
        if (!result || !usable(session)) continue;
        const RxSession* admitted = findRx(session.sessionId);
        if (admitted == nullptr ||
            securityDowngraded(admitted->info, session)) {
            continue;
        }
        if (target.kind == SendTargetKind::Session &&
            session.sessionId != target.sessionId) {
            continue;
        }
        result = enqueue(session, record);
        if (!result) return result;
    }
    return Result::success();
}

void BleRecordTransport::resetSession(uint32_t sessionId) {
    RxSession* session = findRx(sessionId, false);
    if (session != nullptr) {
        const BleSessionInfo info = session->info;
        const uint8_t nextTx = session->nextTxRecordId;
        *session = RxSession();
        session->occupied = true;
        session->info = info;
        session->nextTxRecordId = nextTx;
    }
    dropQueuedForSession(sessionId);
}

void BleRecordTransport::setReceiver(
    BleRecordReceiver receiver,
    void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void BleRecordTransport::setSessionHandlers(
    BleRecordSessionHandler connected,
    BleRecordSessionHandler disconnected,
    void* context) {
    sessionConnected_ = connected;
    sessionDisconnected_ = disconnected;
    sessionContext_ = context;
}

void BleRecordTransport::setFaultHandler(
    BleRecordFaultHandler handler,
    void* context) {
    faultHandler_ = handler;
    faultContext_ = context;
}

void BleRecordTransport::packetThunk(
    void* context,
    const BleSessionInfo& session,
    ByteView packet) {
    BleRecordTransport* transport =
        static_cast<BleRecordTransport*>(context);
    if (transport != nullptr) transport->onPacket(session, packet);
}

void BleRecordTransport::connectedThunk(
    void* context,
    const BleSessionInfo& session) {
    BleRecordTransport* transport =
        static_cast<BleRecordTransport*>(context);
    if (transport != nullptr) transport->onConnected(session);
}

void BleRecordTransport::disconnectedThunk(
    void* context,
    const BleSessionInfo& session) {
    BleRecordTransport* transport =
        static_cast<BleRecordTransport*>(context);
    if (transport != nullptr) transport->onDisconnected(session);
}

void BleRecordTransport::onConnected(const BleSessionInfo& info) {
    if (!started_ || !info.connected || info.sessionId == 0U) {
        emitFault(info, ErrorCode::ProtocolError);
        return;
    }
    RxSession* session = findRx(info.sessionId, false);
    if (session != nullptr) {
        // A session ID identifies one physical connection until its matching
        // disconnect. Re-announcing it could otherwise splice a new peer into
        // the old reassembly/authentication state.
        retireSession(*session, ErrorCode::ProtocolError, info);
        return;
    }
    session = findRx(info.sessionId, true);
    if (session == nullptr) {
        emitFault(info, ErrorCode::CapacityExceeded);
        return;
    }
    session->info = info;
    if (sessionConnected_ != nullptr) {
        const RxContext rx = makeRxContext(info);
        sessionConnected_(sessionContext_, rx);
    }
}

void BleRecordTransport::onDisconnected(const BleSessionInfo& info) {
    RxSession* session = findRx(info.sessionId, false);
    if (session == nullptr) return;
    const BleSessionInfo admitted = session->info;
    *session = RxSession();
    dropQueuedForSession(info.sessionId);
    if (sessionDisconnected_ != nullptr) {
        const RxContext rx = makeRxContext(admitted);
        sessionDisconnected_(sessionContext_, rx);
    }
}

void BleRecordTransport::onPacket(
    const BleSessionInfo& info,
    ByteView packet) {
    if (!started_ || !info.connected || info.sessionId == 0U) {
        ++droppedFragments_;
        return;
    }
    RxSession* session = findRx(info.sessionId, false);
    if (session == nullptr) {
        ++droppedFragments_;
        emitFault(info, ErrorCode::ProtocolError);
        return;
    }
    if (securityDowngraded(session->info, info)) {
        ++droppedFragments_;
        retireSession(
            *session,
            ErrorCode::AuthenticationRequired,
            info);
        return;
    }
    session->info = info;
    if (packet.size > info.maxPacketSize) {
        abortRx(*session, ErrorCode::CapacityExceeded);
        return;
    }

    ble::FragmentView fragment;
    Result result = ble::parseFragment(packet, fragment);
    if (!result) {
        abortRx(*session, result.code());
        return;
    }

    const bool start =
        (fragment.header.flags & ble::FragmentFlagStart) != 0U;
    const bool end =
        (fragment.header.flags & ble::FragmentFlagEnd) != 0U;
    if (start) {
        if ((session->assembling &&
             session->recordId == fragment.header.frameId) ||
            (!session->assembling && session->hasLastRecordId &&
             session->lastRecordId == fragment.header.frameId) ||
            fragment.payload.size < format_.minimumHeaderSize) {
            abortRx(*session, ErrorCode::ProtocolError);
            return;
        }
        size_t totalLength = 0U;
        result = format_.decodeSize(fragment.payload, totalLength);
        const size_t payloadCapacity = fragment.payload.size;
        if (!result || totalLength < fragment.payload.size ||
            totalLength > config_.maxRecordSize || totalLength > UINT16_MAX ||
            fragmentCount(totalLength, payloadCapacity) >
                ble::kMaximumFragmentCount) {
            abortRx(*session, result ? ErrorCode::CapacityExceeded
                                     : result.code());
            return;
        }
        session->assembling = true;
        session->recordId = fragment.header.frameId;
        session->totalLength = static_cast<uint16_t>(totalLength);
        session->receivedLength = 0U;
        session->fragmentPayloadCapacity =
            static_cast<uint16_t>(payloadCapacity);
        session->nextFragmentIndex = 0U;
    } else if (!session->assembling ||
               session->recordId != fragment.header.frameId) {
        abortRx(*session, ErrorCode::ProtocolError);
        return;
    }

    if (session->nextFragmentIndex > UINT8_MAX ||
        fragment.header.index != session->nextFragmentIndex ||
        session->receivedLength >= session->totalLength) {
        abortRx(*session, ErrorCode::ProtocolError);
        return;
    }

    const size_t remaining =
        session->totalLength - session->receivedLength;
    const bool expectedEnd =
        remaining <= session->fragmentPayloadCapacity;
    const size_t expectedPayloadSize =
        expectedEnd ? remaining : session->fragmentPayloadCapacity;
    if (end != expectedEnd || fragment.payload.size != expectedPayloadSize) {
        abortRx(*session, ErrorCode::ProtocolError);
        return;
    }

    const size_t index = rxIndex(session);
    MutableByteSpan destination = rxBuffer(index);
    memcpy(
        destination.data + session->receivedLength,
        fragment.payload.data,
        fragment.payload.size);
    session->receivedLength = static_cast<uint16_t>(
        session->receivedLength + fragment.payload.size);
    ++session->nextFragmentIndex;
    session->lastFragmentMillis = clock_.monotonicMillis();

    if (!end) return;
    if (session->receivedLength != session->totalLength) {
        abortRx(*session, ErrorCode::ProtocolError);
        return;
    }
    const ByteView record(destination.data, session->totalLength);
    result = format_.validate(record);
    session->assembling = false;
    if (!result) {
        abortRx(*session, result.code());
        return;
    }
    session->lastRecordId = session->recordId;
    session->hasLastRecordId = true;
    ++completedRecords_;
    if (receiver_ != nullptr) {
        const RxContext rx = makeRxContext(info);
        receiver_(receiverContext_, record, rx);
    }
}

void BleRecordTransport::reconcileSessions() {
    for (size_t slot = 0U; slot < config_.sessionCapacity; ++slot) {
        RxSession& admitted = rxSessions_[slot];
        if (!admitted.occupied) continue;

        BleSessionInfo current;
        bool found = false;
        const size_t count = link_.sessionCount();
        for (size_t index = 0U; index < count; ++index) {
            BleSessionInfo candidate;
            if (link_.sessionAt(index, candidate) &&
                candidate.sessionId == admitted.info.sessionId) {
                current = candidate;
                found = true;
                break;
            }
        }
        if (!found || !current.connected) {
            const BleSessionInfo faultInfo = admitted.info;
            retireSession(admitted, ErrorCode::NotConnected, faultInfo);
            continue;
        }
        if (securityDowngraded(admitted.info, current)) {
            retireSession(
                admitted,
                ErrorCode::AuthenticationRequired,
                current);
            continue;
        }
        // MTU/notify/security upgrades are live link facts. Security bits may
        // only move from false to true during one admitted session.
        admitted.info = current;
    }
}

void BleRecordTransport::retireSession(
    RxSession& session,
    ErrorCode error,
    const BleSessionInfo& faultInfo) {
    const BleSessionInfo admitted = session.info;
    const uint32_t sessionId = admitted.sessionId;
    session = RxSession();
    dropQueuedForSession(sessionId);
    if (sessionDisconnected_ != nullptr) {
        const RxContext rx = makeRxContext(admitted);
        sessionDisconnected_(sessionContext_, rx);
    }
    emitFault(faultInfo, error);
}

void BleRecordTransport::flushTx() {
    uint8_t packets = 0U;
    while (txCount_ != 0U && packets < config_.maxPacketsPerPoll) {
        TxRecord& item = txRecords_[txHead_];
        BleSessionInfo info;
        bool found = false;
        for (size_t index = 0U; index < link_.sessionCount(); ++index) {
            if (link_.sessionAt(index, info) && usable(info) &&
                info.sessionId == item.sessionId) {
                found = true;
                break;
            }
        }
        if (!found) {
            popTx();
            continue;
        }
        RxSession* sessionState = findRx(item.sessionId, false);
        const size_t availablePayloadCapacity = packetPayloadCapacity(info);
        if (sessionState == nullptr ||
            availablePayloadCapacity < format_.minimumHeaderSize) {
            emitFault(info, ErrorCode::BufferTooSmall);
            popTx();
            continue;
        }
        if (item.offset == 0U) {
            item.fragmentPayloadCapacity = static_cast<uint16_t>(
                availablePayloadCapacity);
            item.fragmentIndex = 0U;
        } else if (availablePayloadCapacity <
                   item.fragmentPayloadCapacity) {
            item.recordId = nextRecordId(*sessionState);
            item.offset = 0U;
            item.fragmentIndex = 0U;
            item.fragmentPayloadCapacity = static_cast<uint16_t>(
                availablePayloadCapacity);
        }
        if (fragmentCount(
                item.recordLength,
                item.fragmentPayloadCapacity) >
            ble::kMaximumFragmentCount) {
            emitFault(info, ErrorCode::CapacityExceeded);
            popTx();
            continue;
        }
        const size_t remaining = item.recordLength - item.offset;
        const size_t payloadSize =
            remaining < item.fragmentPayloadCapacity
                ? remaining
                : item.fragmentPayloadCapacity;
        ble::FragmentHeader header;
        header.flags = item.offset == 0U ? ble::FragmentFlagStart
                                        : ble::FragmentFlagNone;
        if (payloadSize == remaining) {
            header.flags = static_cast<uint8_t>(
                header.flags | ble::FragmentFlagEnd);
        }
        header.frameId = item.recordId;
        header.index = item.fragmentIndex;
        Result result = ble::encodeFragmentHeader(header, packetScratch_);
        if (!result) {
            emitFault(info, result.code());
            popTx();
            continue;
        }
        const MutableByteSpan recordStorage = txBuffer(txHead_);
        memcpy(
            packetScratch_.data + ble::kFragmentHeaderSize,
            recordStorage.data + item.offset,
            payloadSize);
        result = link_.sendPacket(
            item.sessionId,
            ByteView(
                packetScratch_.data,
                ble::kFragmentHeaderSize + payloadSize));
        if (!result) {
            if (result.code() == ErrorCode::WouldBlock) return;
            emitFault(info, result.code());
            popTx();
            continue;
        }
        ++packets;
        item.offset = static_cast<uint16_t>(item.offset + payloadSize);
        if (item.offset == item.recordLength) {
            popTx();
        } else if (item.fragmentIndex == UINT8_MAX) {
            emitFault(info, ErrorCode::CapacityExceeded);
            popTx();
        } else {
            ++item.fragmentIndex;
        }
    }
}

void BleRecordTransport::expireReassembly(uint32_t now) {
    for (size_t index = 0U; index < config_.sessionCapacity; ++index) {
        RxSession& session = rxSessions_[index];
        if (session.occupied && session.assembling &&
            static_cast<uint32_t>(now - session.lastFragmentMillis) >=
                config_.reassemblyTimeoutMillis) {
            abortRx(session, ErrorCode::ProtocolError);
        }
    }
}

void BleRecordTransport::abortRx(
    RxSession& session,
    ErrorCode error) {
    session.assembling = false;
    ++droppedFragments_;
    emitFault(session.info, error);
}

void BleRecordTransport::emitFault(
    const BleSessionInfo& session,
    ErrorCode error) {
    if (faultHandler_ == nullptr) return;
    const RxContext rx = makeRxContext(session);
    faultHandler_(faultContext_, error, rx);
}

BleRecordTransport::RxSession* BleRecordTransport::findRx(
    uint32_t sessionId,
    bool create) {
    RxSession* empty = nullptr;
    for (size_t index = 0U; index < config_.sessionCapacity; ++index) {
        if (rxSessions_[index].occupied &&
            rxSessions_[index].info.sessionId == sessionId) {
            return &rxSessions_[index];
        }
        if (!rxSessions_[index].occupied && empty == nullptr) {
            empty = &rxSessions_[index];
        }
    }
    if (!create || empty == nullptr) return nullptr;
    empty->occupied = true;
    empty->info.sessionId = sessionId;
    return empty;
}

const BleRecordTransport::RxSession* BleRecordTransport::findRx(
    uint32_t sessionId) const {
    for (size_t index = 0U; index < config_.sessionCapacity; ++index) {
        if (rxSessions_[index].occupied &&
            rxSessions_[index].info.sessionId == sessionId) {
            return &rxSessions_[index];
        }
    }
    return nullptr;
}

size_t BleRecordTransport::rxIndex(const RxSession* session) const {
    return static_cast<size_t>(session - rxSessions_);
}

MutableByteSpan BleRecordTransport::rxBuffer(size_t index) const {
    return MutableByteSpan(
        rxStorage_.data + index * config_.maxRecordSize,
        config_.maxRecordSize);
}

MutableByteSpan BleRecordTransport::txBuffer(size_t index) const {
    return MutableByteSpan(
        txStorage_.data + index * config_.maxRecordSize,
        config_.maxRecordSize);
}

Result BleRecordTransport::enqueue(
    const BleSessionInfo& session,
    ByteView record) {
    if (txCount_ >= txCapacity()) {
        return Result::failure(ErrorCode::WouldBlock);
    }
    RxSession* sessionState = findRx(session.sessionId, false);
    const size_t payloadCapacity = packetPayloadCapacity(session);
    if (sessionState == nullptr ||
        payloadCapacity < format_.minimumHeaderSize ||
        fragmentCount(record.size, payloadCapacity) >
            ble::kMaximumFragmentCount) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    TxRecord& item = txRecords_[txTail_];
    // The producer may build directly in an otherwise-empty TX slot. memmove
    // keeps that zero-copy preparation path well-defined while preserving the
    // ordinary borrowed-record behavior.
    memmove(txBuffer(txTail_).data, record.data, record.size);
    item.sessionId = session.sessionId;
    item.recordId = nextRecordId(*sessionState);
    item.recordLength = static_cast<uint16_t>(record.size);
    item.offset = 0U;
    item.fragmentPayloadCapacity = static_cast<uint16_t>(payloadCapacity);
    item.fragmentIndex = 0U;
    item.occupied = true;
    txTail_ = (txTail_ + 1U) % txCapacity();
    ++txCount_;
    return Result::success();
}

void BleRecordTransport::popTx() {
    if (txCount_ == 0U) return;
    txRecords_[txHead_] = TxRecord();
    txHead_ = (txHead_ + 1U) % txCapacity();
    --txCount_;
}

void BleRecordTransport::dropQueuedForSession(uint32_t sessionId) {
    const size_t originalCount = txCount_;
    for (size_t index = 0U; index < originalCount; ++index) {
        const TxRecord item = txRecords_[txHead_];
        if (item.sessionId == sessionId) {
            popTx();
            continue;
        }
        const size_t source = txHead_;
        popTx();
        MutableByteSpan destination = txBuffer(txTail_);
        const MutableByteSpan sourceBuffer = txBuffer(source);
        memmove(destination.data, sourceBuffer.data, item.recordLength);
        txRecords_[txTail_] = item;
        txTail_ = (txTail_ + 1U) % txCapacity();
        ++txCount_;
    }
}

uint8_t BleRecordTransport::nextRecordId(RxSession& session) {
    const uint8_t result = session.nextTxRecordId;
    ++session.nextTxRecordId;
    if (session.nextTxRecordId == 0U) ++session.nextTxRecordId;
    return result;
}

size_t BleRecordTransport::txCapacity() const {
    if (config_.maxRecordSize == 0U) return 0U;
    size_t capacity = txStorage_.size / config_.maxRecordSize;
    if (capacity > config_.txRecordCapacity) {
        capacity = config_.txRecordCapacity;
    }
    if (capacity > BLINKER_BLE_MAX_TX_FRAMES) {
        capacity = BLINKER_BLE_MAX_TX_FRAMES;
    }
    return capacity;
}

size_t BleRecordTransport::packetPayloadCapacity(
    const BleSessionInfo& session) const {
    const size_t packetLimit =
        session.maxPacketSize < packetScratch_.size
            ? session.maxPacketSize
            : packetScratch_.size;
    return packetLimit > ble::kFragmentHeaderSize
               ? packetLimit - ble::kFragmentHeaderSize
               : 0U;
}

size_t BleRecordTransport::fragmentCount(
    size_t recordSize,
    size_t payloadCapacity) {
    if (payloadCapacity == 0U) return 0U;
    return recordSize / payloadCapacity +
           (recordSize % payloadCapacity != 0U ? 1U : 0U);
}

bool BleRecordTransport::usable(const BleSessionInfo& session) {
    return session.connected && session.notifyEnabled &&
           session.sessionId != 0U &&
           session.maxPacketSize >= ble::kMinimumPacketSize;
}

bool BleRecordTransport::securityDowngraded(
    const BleSessionInfo& admitted,
    const BleSessionInfo& current) {
    return (admitted.encrypted && !current.encrypted) ||
           (admitted.bonded && !current.bonded) ||
           (admitted.authenticated && !current.authenticated);
}

RxContext BleRecordTransport::makeRxContext(
    const BleSessionInfo& session) {
    RxContext rx;
    rx.sessionId = session.sessionId;
    rx.encrypted = session.encrypted;
    rx.bonded = session.bonded;
    rx.authenticated = session.authenticated;
    return rx;
}

} // namespace blinker
