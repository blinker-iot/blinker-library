#include "GattDirectChildLink.h"

#include <limits.h>

namespace blinker {

GattDirectChildLink::CentralAdapter::CentralAdapter(
    IBleCentralPort& port)
    : port_(port), session_(), packetReceiver_(nullptr),
      packetContext_(nullptr), connectedHandler_(nullptr),
      disconnectedHandler_(nullptr), sessionContext_(nullptr),
      attemptId_(0U), lastError_(ErrorCode::Ok), started_(false),
      announced_(false), closing_(false) {}

Result GattDirectChildLink::CentralAdapter::start() {
    if (started_) return Result::success();
    if (port_.state() != BleCentralPortState::Stopped) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    port_.setPacketReceiver(&CentralAdapter::packetThunk, this);
    port_.setConnectionHandlers(
        &CentralAdapter::connectedThunk,
        &CentralAdapter::disconnectedThunk,
        this);
    Result result = port_.start();
    if (!result) {
        port_.setPacketReceiver(nullptr, nullptr);
        port_.setConnectionHandlers(nullptr, nullptr, nullptr);
        port_.stop();
        return result;
    }
    started_ = true;
    lastError_ = ErrorCode::Ok;
    return Result::success();
}

void GattDirectChildLink::CentralAdapter::stop() {
    if (!started_) return;
    port_.setPacketReceiver(nullptr, nullptr);
    port_.setConnectionHandlers(nullptr, nullptr, nullptr);
    port_.stop();
    clearConnection();
    lastError_ = ErrorCode::Ok;
    started_ = false;
}

void GattDirectChildLink::CentralAdapter::poll(uint32_t budgetMicros) {
    if (!started_) return;
    port_.poll(budgetMicros);
    if (port_.state() == BleCentralPortState::Error) {
        lastError_ = port_.lastError() == ErrorCode::Ok
                         ? ErrorCode::InternalError
                         : port_.lastError();
    }
    // Current Port state is authoritative even if a native stack loses its
    // completion callback. BleRecordTransport will reconcile any previously
    // admitted session after this physical slot is cleared.
    if (attemptId_ != 0U &&
        (port_.state() == BleCentralPortState::Idle ||
         port_.state() == BleCentralPortState::Stopped)) {
        clearConnection();
    }
}

BleLinkState GattDirectChildLink::CentralAdapter::state() const {
    if (!started_ || port_.state() == BleCentralPortState::Stopped) {
        return BleLinkState::Stopped;
    }
    if (lastError_ != ErrorCode::Ok ||
        port_.state() == BleCentralPortState::Error) {
        return BleLinkState::Error;
    }
    if (port_.state() == BleCentralPortState::Idle ||
        port_.state() == BleCentralPortState::Ready) {
        return BleLinkState::Ready;
    }
    return BleLinkState::Starting;
}

ErrorCode GattDirectChildLink::CentralAdapter::lastError() const {
    return lastError_ != ErrorCode::Ok ? lastError_ : port_.lastError();
}

size_t GattDirectChildLink::CentralAdapter::sessionCount() const {
    return announced_ && !closing_ ? 1U : 0U;
}

Result GattDirectChildLink::CentralAdapter::sessionAt(
    size_t index,
    BleSessionInfo& session) const {
    if (index != 0U || !announced_ || closing_) {
        return Result::failure(ErrorCode::NotFound);
    }
    BleCentralConnectionInfo current;
    Result result = port_.connection(current);
    if (!result || current.attemptId != attemptId_ ||
        !validConnection(current)) {
        return Result::failure(ErrorCode::NotConnected);
    }
    session = session_;
    session.maxPacketSize = current.maxPacketSize;
    session.encrypted = current.encrypted;
    session.bonded = current.bonded;
    return Result::success();
}

Result GattDirectChildLink::CentralAdapter::sendPacket(
    uint32_t sessionId,
    ByteView packet) {
    if (!announced_ || closing_ || sessionId != attemptId_) {
        return Result::failure(ErrorCode::NotConnected);
    }
    return port_.sendPacket(sessionId, packet);
}

Result GattDirectChildLink::CentralAdapter::disconnectSession(
    uint32_t sessionId) {
    if (!announced_ || closing_ || sessionId != attemptId_) {
        return Result::failure(ErrorCode::NotFound);
    }
    Result result = port_.cancel(sessionId);
    if (result) closing_ = true;
    return result;
}

void GattDirectChildLink::CentralAdapter::setPacketReceiver(
    BlePacketReceiver receiver,
    void* context) {
    packetReceiver_ = receiver;
    packetContext_ = context;
}

void GattDirectChildLink::CentralAdapter::setSessionHandlers(
    BleSessionHandler connected,
    BleSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

Result GattDirectChildLink::CentralAdapter::connect(
    const BleCentralConnectRequest& request) {
    if (!started_ || port_.state() != BleCentralPortState::Idle) {
        return Result::failure(ErrorCode::NotConnected);
    }
    if (attemptId_ != 0U || request.attemptId == 0U ||
        request.matcher == nullptr) {
        return Result::failure(
            attemptId_ != 0U ? ErrorCode::AlreadyExists
                             : ErrorCode::InvalidArgument);
    }
    Result result = port_.connect(request);
    if (result) {
        attemptId_ = request.attemptId;
        lastError_ = ErrorCode::Ok;
        closing_ = false;
    }
    return result;
}

Result GattDirectChildLink::CentralAdapter::cancelCurrent() {
    if (attemptId_ == 0U || closing_) {
        return Result::failure(ErrorCode::NotFound);
    }
    Result result = port_.cancel(attemptId_);
    if (result) closing_ = true;
    return result;
}

void GattDirectChildLink::CentralAdapter::packetThunk(
    void* context,
    const BleCentralConnectionInfo& connection,
    ByteView packet) {
    CentralAdapter* adapter = static_cast<CentralAdapter*>(context);
    if (adapter != nullptr) adapter->onPacket(connection, packet);
}

void GattDirectChildLink::CentralAdapter::connectedThunk(
    void* context,
    const BleCentralConnectionInfo& connection) {
    CentralAdapter* adapter = static_cast<CentralAdapter*>(context);
    if (adapter != nullptr) adapter->onConnected(connection);
}

void GattDirectChildLink::CentralAdapter::disconnectedThunk(
    void* context,
    const BleCentralConnectionInfo& connection) {
    CentralAdapter* adapter = static_cast<CentralAdapter*>(context);
    if (adapter != nullptr) adapter->onDisconnected(connection);
}

void GattDirectChildLink::CentralAdapter::onPacket(
    const BleCentralConnectionInfo& connection,
    ByteView packet) {
    if (!announced_ || closing_ ||
        connection.attemptId != attemptId_ ||
        !validConnection(connection) ||
        packet.size > connection.maxPacketSize) {
        return;
    }
    if (packetReceiver_ != nullptr) {
        BleSessionInfo current = session_;
        current.maxPacketSize = connection.maxPacketSize;
        current.encrypted = connection.encrypted;
        current.bonded = connection.bonded;
        packetReceiver_(packetContext_, current, packet);
    }
}

void GattDirectChildLink::CentralAdapter::onConnected(
    const BleCentralConnectionInfo& connection) {
    // A late callback from a cancelled generation is not evidence against the
    // current peer. Ignore it without cancelling or poisoning the new attempt.
    if (connection.attemptId == 0U ||
        connection.attemptId != attemptId_) {
        return;
    }
    if (!started_ || announced_ || closing_ ||
        !validConnection(connection)) {
        lastError_ = ErrorCode::ProtocolError;
        (void)port_.cancel(connection.attemptId);
        return;
    }
    session_ = BleSessionInfo();
    session_.sessionId = connection.attemptId;
    session_.maxPacketSize = connection.maxPacketSize;
    session_.connected = true;
    session_.notifyEnabled = true;
    session_.encrypted = connection.encrypted;
    session_.bonded = connection.bonded;
    // Native BLE security is metadata only. Method 2/DirectSecure establishes
    // application authentication above this Port.
    session_.authenticated = false;
    announced_ = true;
    if (connectedHandler_ != nullptr) {
        connectedHandler_(sessionContext_, session_);
    }
}

void GattDirectChildLink::CentralAdapter::onDisconnected(
    const BleCentralConnectionInfo& connection) {
    if (connection.attemptId == 0U ||
        connection.attemptId != attemptId_) {
        return;
    }
    const BleSessionInfo previous = session_;
    const bool notify = announced_;
    clearConnection();
    if (notify && disconnectedHandler_ != nullptr) {
        disconnectedHandler_(sessionContext_, previous);
    }
}

void GattDirectChildLink::CentralAdapter::clearConnection() {
    session_ = BleSessionInfo();
    attemptId_ = 0U;
    announced_ = false;
    closing_ = false;
}

bool GattDirectChildLink::CentralAdapter::validConnection(
    const BleCentralConnectionInfo& connection) {
    return connection.attemptId != 0U && connection.connected &&
           connection.notifyEnabled &&
           connection.maxPacketSize >= ble::kMinimumPacketSize;
}

GattDirectChildLink::GattDirectChildLink(
    IBleCentralPort& port,
    IClock& clock,
    MutableByteSpan rxStorage,
    MutableByteSpan txStorage,
    MutableByteSpan packetScratch,
    const GattDirectChildLinkConfig& config)
    : adapter_(port),
      records_(
          adapter_, clock, directRecordFormat(), rxStorage,
          txStorage, packetScratch, recordConfig(config)),
      clock_(clock), config_(config), receiver_(nullptr),
      receiverContext_(nullptr), connectedHandler_(nullptr),
      disconnectedHandler_(nullptr), sessionContext_(nullptr),
      faultHandler_(nullptr), faultContext_(nullptr), nextAttemptId_(0U),
      connectStartedMillis_(0U), connectingAttemptId_(0U),
      connectedAttemptId_(0U),
      retireAttemptId_(0U), faultRetirementAttemptId_(0U),
      reportedPortError_(ErrorCode::Ok),
      started_(false), connecting_(false) {
    records_.setReceiver(&GattDirectChildLink::recordThunk, this);
    records_.setSessionHandlers(
        &GattDirectChildLink::connectedThunk,
        &GattDirectChildLink::disconnectedThunk,
        this);
    records_.setFaultHandler(&GattDirectChildLink::faultThunk, this);
}

BleRecordTransportConfig GattDirectChildLink::recordConfig(
    const GattDirectChildLinkConfig& config) {
    BleRecordTransportConfig output;
    const size_t maximum = security::directSecureRecordSize(
        config.maxFrameSize);
    output.maxRecordSize = maximum <= UINT16_MAX
                               ? static_cast<uint16_t>(maximum)
                               : 0U;
    output.reassemblyTimeoutMillis = config.reassemblyTimeoutMillis;
    output.maxPacketsPerPoll = config.maxPacketsPerPoll;
    output.sessionCapacity = 1U;
    output.txRecordCapacity = 1U;
    return output;
}

Result GattDirectChildLink::validateConfig() const {
    const size_t maximum = security::directSecureRecordSize(
        config_.maxFrameSize);
    if (config_.maxFrameSize < bbp2::kBaseHeaderSize || maximum == 0U ||
        maximum > UINT16_MAX ||
        maximum > ble::kMaximumRecordSizeAtMinimumPacket ||
        config_.connectTimeoutMillis == 0U ||
        config_.connectTimeoutMillis > INT32_MAX ||
        config_.reassemblyTimeoutMillis == 0U ||
        config_.maxPacketsPerPoll == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    return Result::success();
}

Result GattDirectChildLink::start() {
    if (started_) return Result::success();
    Result result = validateConfig();
    if (result) result = records_.start();
    if (result) {
        started_ = true;
        reportedPortError_ = ErrorCode::Ok;
    }
    return result;
}

void GattDirectChildLink::stop() {
    if (!started_) return;
    records_.stop();
    connectStartedMillis_ = 0U;
    connectingAttemptId_ = 0U;
    connectedAttemptId_ = 0U;
    retireAttemptId_ = 0U;
    faultRetirementAttemptId_ = 0U;
    reportedPortError_ = ErrorCode::Ok;
    connecting_ = false;
    started_ = false;
}

void GattDirectChildLink::poll(uint32_t budgetMicros) {
    if (!started_) return;
    records_.poll(budgetMicros);

    // A native connect or discovery failure can retire the attempt before a
    // record session exists. Reconcile that fact immediately; otherwise the
    // caller waits for the outer deadline and loses the failed generation.
    if (connecting_ && adapter_.attemptId() == 0U) {
        const uint32_t failed = connectingAttemptId_;
        const ErrorCode error = adapter_.lastError() == ErrorCode::Ok
                                    ? ErrorCode::NotConnected
                                    : adapter_.lastError();
        connecting_ = false;
        connectingAttemptId_ = 0U;
        if (adapter_.state() == BleLinkState::Error) {
            reportedPortError_ = error;
        }
        emitFault(error, failed);
    }

    if (retireAttemptId_ != 0U) {
        const uint32_t retiring = retireAttemptId_;
        retireAttemptId_ = 0U;
        if (connectedAttemptId_ == retiring) {
            // BleRecordTransport reports the requested retirement as a fault
            // as well. The original fragment/protocol fault was already
            // delivered, so suppress only this synchronous duplicate.
            faultRetirementAttemptId_ = retiring;
            (void)disconnect();
            faultRetirementAttemptId_ = 0U;
        }
    }

    if (connecting_ &&
        static_cast<uint32_t>(
            clock_.monotonicMillis() - connectStartedMillis_) >=
            config_.connectTimeoutMillis) {
        const uint32_t expired = adapter_.attemptId();
        (void)adapter_.cancelCurrent();
        connecting_ = false;
        connectingAttemptId_ = 0U;
        emitFault(ErrorCode::NotConnected, expired);
    }

    if (adapter_.state() == BleLinkState::Error) {
        const ErrorCode error = adapter_.lastError() == ErrorCode::Ok
                                    ? ErrorCode::InternalError
                                    : adapter_.lastError();
        if (reportedPortError_ != error) {
            reportedPortError_ = error;
            connecting_ = false;
            emitFault(error, adapter_.attemptId());
        }
    }
}

Result GattDirectChildLink::connect(
    BleCentralAdvertisementMatcher matcher,
    void* matcherContext) {
    if (!started_) return Result::failure(ErrorCode::NotConnected);
    if (matcher == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (connecting_ || connectedAttemptId_ != 0U ||
        adapter_.attemptId() != 0U) {
        return Result::failure(ErrorCode::AlreadyExists);
    }
    ++nextAttemptId_;
    if (nextAttemptId_ == 0U) ++nextAttemptId_;
    BleCentralConnectRequest request;
    request.attemptId = nextAttemptId_;
    request.matcher = matcher;
    request.matcherContext = matcherContext;
    Result result = adapter_.connect(request);
    if (result) {
        connectStartedMillis_ = clock_.monotonicMillis();
        connectingAttemptId_ = request.attemptId;
        connecting_ = true;
        reportedPortError_ = ErrorCode::Ok;
    }
    return result;
}

Result GattDirectChildLink::disconnect() {
    if (connectedAttemptId_ != 0U) {
        const uint32_t retiring = connectedAttemptId_;
        const uint32_t previousSuppression = faultRetirementAttemptId_;
        faultRetirementAttemptId_ = retiring;
        const Result result = records_.disconnectSession(
            retiring, ErrorCode::NotConnected);
        faultRetirementAttemptId_ = previousSuppression;
        return result;
    }
    if (adapter_.attemptId() != 0U) {
        Result result = adapter_.cancelCurrent();
        if (result) {
            connecting_ = false;
            connectingAttemptId_ = 0U;
        }
        return result;
    }
    return Result::failure(ErrorCode::NotFound);
}

Result GattDirectChildLink::sendRecord(ByteView record) {
    if (connectedAttemptId_ == 0U) {
        return Result::failure(ErrorCode::NotConnected);
    }
    SendTarget target;
    target.kind = SendTargetKind::Session;
    target.sessionId = connectedAttemptId_;
    return records_.send(record, target);
}

void GattDirectChildLink::setReceiver(
    BleRecordReceiver receiver,
    void* context) {
    receiver_ = receiver;
    receiverContext_ = context;
}

void GattDirectChildLink::setSessionHandlers(
    BleRecordSessionHandler connected,
    BleRecordSessionHandler disconnected,
    void* context) {
    connectedHandler_ = connected;
    disconnectedHandler_ = disconnected;
    sessionContext_ = context;
}

void GattDirectChildLink::setFaultHandler(
    GattDirectChildFaultHandler handler,
    void* context) {
    faultHandler_ = handler;
    faultContext_ = context;
}

void GattDirectChildLink::emitFault(
    ErrorCode error,
    uint32_t attemptId) {
    if (faultHandler_ != nullptr) {
        faultHandler_(faultContext_, error, attemptId);
    }
}

void GattDirectChildLink::recordThunk(
    void* context,
    ByteView record,
    const RxContext& rx) {
    GattDirectChildLink* link =
        static_cast<GattDirectChildLink*>(context);
    if (link != nullptr) link->onRecord(record, rx);
}

void GattDirectChildLink::connectedThunk(
    void* context,
    const RxContext& rx) {
    GattDirectChildLink* link =
        static_cast<GattDirectChildLink*>(context);
    if (link != nullptr) link->onConnected(rx);
}

void GattDirectChildLink::disconnectedThunk(
    void* context,
    const RxContext& rx) {
    GattDirectChildLink* link =
        static_cast<GattDirectChildLink*>(context);
    if (link != nullptr) link->onDisconnected(rx);
}

void GattDirectChildLink::faultThunk(
    void* context,
    ErrorCode error,
    const RxContext& rx) {
    GattDirectChildLink* link =
        static_cast<GattDirectChildLink*>(context);
    if (link != nullptr) link->onRecordFault(error, rx);
}

void GattDirectChildLink::onRecord(
    ByteView record,
    const RxContext& rx) {
    if (receiver_ != nullptr) {
        receiver_(receiverContext_, record, rx);
    }
}

void GattDirectChildLink::onConnected(const RxContext& rx) {
    connecting_ = false;
    connectingAttemptId_ = 0U;
    connectedAttemptId_ = rx.sessionId;
    if (connectedHandler_ != nullptr) {
        connectedHandler_(sessionContext_, rx);
    }
}

void GattDirectChildLink::onDisconnected(const RxContext& rx) {
    if (connectedAttemptId_ == rx.sessionId) {
        connectedAttemptId_ = 0U;
    }
    if (disconnectedHandler_ != nullptr) {
        disconnectedHandler_(sessionContext_, rx);
    }
}

void GattDirectChildLink::onRecordFault(
    ErrorCode error,
    const RxContext& rx) {
    if (rx.sessionId != 0U &&
        rx.sessionId == faultRetirementAttemptId_) {
        return;
    }
    emitFault(error, rx.sessionId);
    if (rx.sessionId != 0U) retireAttemptId_ = rx.sessionId;
}

} // namespace blinker
