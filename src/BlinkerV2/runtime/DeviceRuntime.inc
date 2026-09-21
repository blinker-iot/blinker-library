#include "DeviceRuntime.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../model/Manifest.h"
#include "../model/EndpointValue.h"
#include "../protocol/bbp2/TimeSync.h"

namespace blinker {

namespace {

Result validatePatchValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessRead | AccessNotify);
}

Result validateIdPatchValue(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessRead | AccessNotify);
}

Result validateApplyValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessWrite);
}

Result validateCommandValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        static_cast<uint8_t>(AccessCommand | AccessWrite));
}

Result validateIdCommandValue(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        static_cast<uint8_t>(AccessCommand | AccessWrite));
}

Result validateIdApplyValue(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessWrite);
}

Result validateEventValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->find(endpointKey);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessEvent);
}

Result validateIdEventValue(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    const EndpointRegistry* endpoints =
        static_cast<const EndpointRegistry*>(context);
    if (endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const EndpointDescriptor* endpoint = endpoints->findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return validateEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        AccessEvent);
}

struct StatePageValidationContext {
    const EndpointRegistry* endpoints;
    uint16_t cursor;
    uint16_t nextCursor;

    StatePageValidationContext()
        : endpoints(nullptr), cursor(0), nextCursor(0) {}
};

Result validateStatePageKeyedValue(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    StatePageValidationContext* page =
        static_cast<StatePageValidationContext*>(context);
    if (page == nullptr || page->endpoints == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    uint16_t id = 0U;
    Result result = page->endpoints->idOf(endpointKey, id);
    if (!result) return result;
    const uint16_t ordinal = static_cast<uint16_t>(id - 1U);
    if (ordinal < page->cursor || ordinal >= page->nextCursor) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return validatePatchValue(
        const_cast<EndpointRegistry*>(page->endpoints),
        endpointKey,
        encodedValue,
        valueType);
}

Result validateStatePageIdValue(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    StatePageValidationContext* page =
        static_cast<StatePageValidationContext*>(context);
    if (page == nullptr || page->endpoints == nullptr || endpointId == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const uint16_t ordinal = static_cast<uint16_t>(endpointId - 1U);
    if (ordinal < page->cursor || ordinal >= page->nextCursor) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    return validateIdPatchValue(
        const_cast<EndpointRegistry*>(page->endpoints),
        endpointId,
        encodedValue,
        valueType);
}

void incrementDiagnostic(uint16_t& counter) {
    if (counter != UINT16_MAX) ++counter;
}

void incrementDiagnostic(uint32_t& counter) {
    if (counter != UINT32_MAX) ++counter;
}

} // namespace

DeviceRuntime::DeviceRuntime(
    TransportHub& transports,
    EndpointRegistry& endpoints,
    MutableByteSpan transmitBuffer,
    const DeviceRuntimeConfig& config)
    : transports_(transports),
      endpoints_(endpoints),
      transmitBuffer_(transmitBuffer),
      config_(config),
      commandHandler_(nullptr),
      commandContext_(nullptr),
      stateApplyHandler_(nullptr),
      stateApplyContext_(nullptr),
      stateApplyTransactionHandler_(nullptr),
      stateApplyTransactionContext_(nullptr),
      stateSettlementHandler_(nullptr),
      stateSettlementContext_(nullptr),
      remoteErrorHandler_(nullptr),
      remoteErrorContext_(nullptr),
      eventHandler_(nullptr),
      eventContext_(nullptr),
      stateEncoder_(nullptr),
      statePageEncoder_(nullptr),
      stateContext_(nullptr),
      statePageContext_(nullptr),
      reliableOutbox_(nullptr),
      timeSync_(nullptr),
      telemetryClock_(nullptr),
      telemetrySampleEncoder_(nullptr),
      telemetrySampleContext_(nullptr),
      telemetryLeases_(),
      telemetryCounters_(),
      peers_(),
      helloSent_(),
      nextSequence_(1),
      receivedFrameCount_(0),
      rejectedFrameCount_(0),
      duplicateCommandCount_(0),
      duplicateRequestCount_(0),
      sequenceConflictCount_(0),
      stateRevision_(config.initialStateRevision),
      manifestFingerprint_(),
      hasManifestFingerprint_(false),
      started_(false) {}

Result DeviceRuntime::start() {
    if (started_) return Result::failure(ErrorCode::AlreadyExists);
    // Access services are validated per transport, never pooled into one
    // global advertised method list. Explicit feature requests still fail
    // if no binding supplies them.
    uint32_t suppliedFeatures = 0U;
    for (size_t transportId = 0; transportId < transports_.size(); ++transportId) {
        const TransportAccess access = transports_.accessAt(transportId);
        if ((access.authorization != nullptr || access.control != nullptr) &&
            !localTransport(static_cast<uint8_t>(transportId))) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (access.authorization != nullptr) {
            const size_t count = access.authorization->methodCount();
            if (count == 0U || count > bbp2::kMaxAuthorizationMethods) {
                return Result::failure(ErrorCode::NotConfigured);
            }
            for (size_t index = 0; index < count; ++index) {
                const uint16_t method = access.authorization->methodAt(index);
                if (method == 0U) return Result::failure(ErrorCode::NotConfigured);
                for (size_t prior = 0; prior < index; ++prior) {
                    if (access.authorization->methodAt(prior) == method) {
                        return Result::failure(ErrorCode::DuplicateField);
                    }
                }
            }
        }
        if (access.control != nullptr &&
            config_.maxFrameSize < bbp2::kControllerMutationMaximumFrameSize) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        suppliedFeatures |= transportFeatures(static_cast<uint8_t>(transportId));
    }
    const uint32_t accessFeatures = bbp2::FeatureAuthentication |
        bbp2::FeatureControllerControl | bbp2::FeaturePresenceKeyControl;
    if ((config_.protocolFeatures & accessFeatures & ~suppliedFeatures) != 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    const bool reliableFeature =
        (config_.protocolFeatures &
         bbp2::FeatureReliableDelivery) != 0U;
    if (transmitBuffer_.data == nullptr ||
        transmitBuffer_.size < bbp2::kBaseHeaderSize ||
        config_.maxFrameSize < bbp2::kBaseHeaderSize ||
        config_.maxFrameSize > transmitBuffer_.size ||
        config_.maxReassemblySize < config_.maxFrameSize ||
        config_.maxAuthorizationAttempts == 0U ||
        (reliableFeature &&
         (config_.reliableReceiveWindow == 0U ||
          config_.reliableReceiveWindow >
              BLINKER_RELIABLE_RECEIVE_WINDOW ||
          config_.reliableReceiveWindow >
              bbp2::kMaxReliableReceiveWindow)) ||
        ((config_.protocolFeatures & bbp2::FeatureEndpointIds) != 0U &&
         (config_.protocolFeatures & bbp2::FeatureManifest) == 0U) ||
        ((config_.protocolFeatures & bbp2::FeatureTelemetry) != 0U &&
         (config_.protocolFeatures &
          (bbp2::FeatureManifest | bbp2::FeatureEndpointIds)) !=
             (bbp2::FeatureManifest | bbp2::FeatureEndpointIds)) ||
        transports_.size() == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (reliableOutbox_ != nullptr) {
        const Result outboxResult =
            reliableOutbox_->validate(config_.maxFrameSize);
        if (!outboxResult) return outboxResult;
        reliableOutbox_->reset();
    }
    Result registryResult = endpoints_.seal();
    if (!registryResult) return registryResult;
    if ((config_.protocolFeatures & bbp2::FeatureManifest) != 0U) {
        Result fingerprintResult = computeManifestFingerprint(
            config_.manifestRevision,
            endpoints_,
            MutableByteSpan(
                manifestFingerprint_,
                sizeof(manifestFingerprint_)));
        if (!fingerprintResult) return fingerprintResult;
        hasManifestFingerprint_ = true;
    }
    resetAuthorizationSessions();
    transports_.setReceiver(&DeviceRuntime::receiveThunk, this);
    transports_.setSessionHandlers(
        &DeviceRuntime::connectedThunk,
        &DeviceRuntime::disconnectedThunk,
        this);
    Result result = transports_.startAll();
    if (!result) {
        transports_.setReceiver(nullptr, nullptr);
        transports_.setSessionHandlers(nullptr, nullptr, nullptr);
        return result;
    }
    started_ = true;
    return Result::success();
}

void DeviceRuntime::stop() {
    if (timeSync_ != nullptr) timeSync_->reset(true);
    transports_.stopAll();
    transports_.setReceiver(nullptr, nullptr);
    transports_.setSessionHandlers(nullptr, nullptr, nullptr);
    if (reliableOutbox_ != nullptr) reliableOutbox_->reset();
    telemetryLeases_.reset();
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        if (peers_[index].occupied) retirePeer(peers_[index]);
    }
    resetAuthorizationSessions();
    for (size_t index = 0; index < BLINKER_MAX_TRANSPORTS; ++index) {
        helloSent_[index] = false;
    }
    memset(manifestFingerprint_, 0, sizeof(manifestFingerprint_));
    hasManifestFingerprint_ = false;
    started_ = false;
}

void DeviceRuntime::poll(uint32_t totalBudgetMicros) {
    if (!started_) return;
    transports_.poll(totalBudgetMicros);
    for (size_t index = 0; index < transports_.size(); ++index) {
        IFrameTransport* transport = transports_.at(index);
        if (transport == nullptr) continue;
        if (transport->state() != TransportState::Online) {
            if (reliableOutbox_ != nullptr) {
                reliableOutbox_->transportClosed(
                    static_cast<uint8_t>(index));
            }
            helloSent_[index] = false;
            telemetryLeases_.transportClosed(static_cast<uint8_t>(index));
            for (size_t peerIndex = 0;
                 peerIndex < BLINKER_MAX_PEER_SESSIONS;
                 ++peerIndex) {
                if (peers_[peerIndex].occupied &&
                    peers_[peerIndex].transportId == index) {
                    retirePeer(peers_[peerIndex]);
                }
            }
            continue;
        }
        // Transport progress releases its existing TX slot. Service a bounded
        // cached result before automatic Hello/outbox/telemetry traffic.
        pollRequestAcks(static_cast<uint8_t>(index));
        const bool peerInitiatesHello =
            (transport->capabilities().features &
             TransportFeaturePeerInitiatesHello) != 0U;
        bool hasExplicitSession = false;
        for (size_t peerIndex = 0;
             peerIndex < BLINKER_MAX_PEER_SESSIONS;
             ++peerIndex) {
            PeerSession& session = peers_[peerIndex];
            if (!session.occupied || session.transportId != index ||
                session.sessionId == 0U) {
                continue;
            }
            hasExplicitSession = true;
            if (session.helloSent || peerInitiatesHello) continue;
            SendTarget target;
            target.kind = SendTargetKind::Session;
            target.transportId = static_cast<uint8_t>(index);
            target.sessionId = session.sessionId;
            const Result result = sendHello(
                target,
                bbp2::FlagAckRequired,
                nextSequence());
            if (result) session.helloSent = true;
        }
        if (hasExplicitSession || peerInitiatesHello) continue;
        if (!helloSent_[index]) {
            SendTarget target;
            target.kind = SendTargetKind::Transport;
            target.transportId = static_cast<uint8_t>(index);
            const Result result = sendHello(
                target,
                bbp2::FlagAckRequired,
                nextSequence());
            if (result) helloSent_[index] = true;
        }
    }
    if (reliableOutbox_ != nullptr) reliableOutbox_->poll();
    pollTelemetry();
    pollTimeSync(); // Control/ACK/bootstrap and sampling keep their original priority.
}

Result DeviceRuntime::setTimeSync(ITimeSync* service) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    if (timeSync_ != nullptr && service != timeSync_)
        return Result::failure(ErrorCode::AlreadyExists);
    timeSync_ = service;
    return Result::success();
}

WallTimeSource DeviceRuntime::timeSource(uint8_t transportId) const {
    const IFrameTransport* transport = transports_.at(transportId);
    if (transport == nullptr) return WallTimeSource::None;
    const uint16_t flags = transport->capabilities().features;
    const bool local = (flags & TransportFeatureLocal) != 0U;
    const bool cloud = (flags & TransportFeatureCloud) != 0U;
    const bool authenticated = (flags & TransportFeatureAuthenticated) != 0U;
    if (cloud && !local && authenticated) return WallTimeSource::Server;
    if (local && !cloud && (authenticated || transports_.accessAt(transportId).authorization != nullptr))
        return WallTimeSource::Controller;
    return WallTimeSource::None;
}

TimeSyncPeer DeviceRuntime::timePeer(const PeerSession& session) const {
    TimeSyncPeer value;
    const IFrameTransport* transport = transports_.at(session.transportId);
    if (transport == nullptr || transport->state() != TransportState::Online) return value;
    value.transportId = session.transportId;
    value.sessionId = session.sessionId;
    value.source = timeSource(session.transportId); // Bound path, never Hello.role.
    value.features = session.helloComplete ? session.negotiatedFeatures : 0U;
    value.authenticated = authorizedFor(session, kAuthorizationPermissionControl);
    value.permissions = value.authenticated ? kAuthorizationPermissionControl : 0U;
    return value;
}

TimeSyncPeer DeviceRuntime::selectedTimePeer() const {
    TimeSyncPeer selected;
    for (size_t i = 0U; i < BLINKER_MAX_PEER_SESSIONS; ++i) {
        const PeerSession& session = peers_[i];
        if (!session.occupied || !session.helloComplete || !session.idReady) continue;
        const TimeSyncPeer candidate = timePeer(session);
        if (candidate.source == WallTimeSource::None || !candidate.authenticated ||
            (candidate.features & bbp2::FeatureTimeSync) == 0U) continue;
        if (candidate.source == WallTimeSource::Server) return candidate;
        if (selected.source == WallTimeSource::None) selected = candidate;
    }
    return selected;
}

void DeviceRuntime::pollTimeSync() {
    if (timeSync_ == nullptr) return;
    const TimeSyncPeer selected = selectedTimePeer();
    if (!timeSync_->poll(selected) || selected.source == WallTimeSource::None) return;
    const uint16_t sequence = nextSequence();
    size_t written = 0U;
    Result result = timeSync_->request(selected, sequence,
        MutableByteSpan(transmitBuffer_.data + bbp2::kBaseHeaderSize,
                        transmitBuffer_.size - bbp2::kBaseHeaderSize), written);
    if (result) {
        SendTarget target;
        target.kind = SendTargetKind::Session;
        target.transportId = selected.transportId;
        target.sessionId = selected.sessionId;
        result = sendEncodedBody(bbp2::MessageKind::TimeRequest, bbp2::FlagNone, sequence, written, target);
        if (!result) timeSync_->sendRejected(result.code());
    } else timeSync_->reset(false); // Encoding/RNG failure retains the long retry deadline.
}

void DeviceRuntime::setCommandHandler(
    CommandHandler handler,
    void* context) {
    commandHandler_ = handler;
    commandContext_ = context;
}

void DeviceRuntime::setStateApplyHandler(
    StateApplyHandler handler,
    void* context) {
    stateApplyHandler_ = handler;
    stateApplyContext_ = context;
}

void DeviceRuntime::setStateApplyTransactionHandler(
    StateApplyTransactionHandler handler,
    void* context) {
    stateApplyTransactionHandler_ = handler;
    stateApplyTransactionContext_ = context;
}

void DeviceRuntime::setStateSettlementHandler(
    StateSettlementHandler handler,
    void* context) {
    stateSettlementHandler_ = handler;
    stateSettlementContext_ = context;
}

void DeviceRuntime::setRemoteErrorHandler(
    RemoteErrorHandler handler,
    void* context) {
    remoteErrorHandler_ = handler;
    remoteErrorContext_ = context;
}

void DeviceRuntime::setEventHandler(
    EventHandler handler,
    void* context) {
    eventHandler_ = handler;
    eventContext_ = context;
}

void DeviceRuntime::setStateEncoder(
    StateEncoder encoder,
    void* context) {
    stateEncoder_ = encoder;
    stateContext_ = context;
}

void DeviceRuntime::setStatePageEncoder(
    StatePageEncoder encoder,
    void* context) {
    statePageEncoder_ = encoder;
    statePageContext_ = context;
}

uint32_t DeviceRuntime::transportFeatures(uint8_t transportId) const {
    const TransportAccess access = transports_.accessAt(transportId);
    uint32_t features = config_.protocolFeatures &
        ~static_cast<uint32_t>(bbp2::FeatureAuthentication |
            bbp2::FeatureControllerControl | bbp2::FeaturePresenceKeyControl | bbp2::FeatureTimeSync);
    if (timeSync_ != nullptr && config_.maxFrameSize >= bbp2::kBaseHeaderSize + bbp2::kTimeResponseMaxSize &&
        timeSource(transportId) != WallTimeSource::None)
        features |= bbp2::FeatureTimeSync;
    if (access.authorization != nullptr) features |= bbp2::FeatureAuthentication;
    if (access.control != nullptr) {
        features |= bbp2::FeatureControllerControl;
        if (access.control->supportsPresenceKeyControl()) {
            features |= bbp2::FeaturePresenceKeyControl;
        }
    }
    return features;
}

void DeviceRuntime::resetAuthorizationSessions() {
    for (size_t index = 0; index < transports_.size(); ++index) {
        IAuthorizationProvider* provider = transports_.accessAt(index).authorization;
        if (provider == nullptr) continue;
        bool alreadyReset = false;
        for (size_t prior = 0; prior < index; ++prior) {
            if (transports_.accessAt(prior).authorization == provider) {
                alreadyReset = true;
                break;
            }
        }
        if (!alreadyReset) provider->resetSessions();
    }
}

Result DeviceRuntime::setReliableOutbox(ReliableOutbox* outbox) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    reliableOutbox_ = outbox;
    return Result::success();
}

Result DeviceRuntime::setTelemetrySampler(
    IClock* clock,
    TelemetrySampleEncoder encoder,
    void* context) {
    if (started_) return Result::failure(ErrorCode::NotConfigured);
    if ((clock == nullptr) != (encoder == nullptr) ||
        (encoder != nullptr && context == nullptr)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    telemetryClock_ = clock;
    telemetrySampleEncoder_ = encoder;
    telemetrySampleContext_ = context;
    if (encoder != nullptr) {
        config_.protocolFeatures |= bbp2::FeatureTelemetry;
    } else {
        config_.protocolFeatures &=
            ~static_cast<uint32_t>(bbp2::FeatureTelemetry);
        telemetryLeases_.reset();
    }
    return Result::success();
}

void DeviceRuntime::deferTelemetrySamples(
    const RxContext& source,
    uint32_t nowMillis,
    uint32_t durationMillis) {
    telemetryLeases_.deferSource(source, nowMillis, durationMillis);
}

void DeviceRuntime::receiveThunk(
    void* context,
    ByteView frame,
    const RxContext& rx) {
    DeviceRuntime* runtime = static_cast<DeviceRuntime*>(context);
    if (runtime != nullptr) runtime->receive(frame, rx);
}

void DeviceRuntime::connectedThunk(
    void* context,
    const RxContext& rx) {
    DeviceRuntime* runtime = static_cast<DeviceRuntime*>(context);
    if (runtime != nullptr) runtime->sessionConnected(rx);
}

void DeviceRuntime::disconnectedThunk(
    void* context,
    const RxContext& rx) {
    DeviceRuntime* runtime = static_cast<DeviceRuntime*>(context);
    if (runtime != nullptr) runtime->sessionDisconnected(rx);
}

void DeviceRuntime::sessionConnected(const RxContext& rx) {
    if (!started_ || rx.sessionId == 0U) return;
    PeerSession* session = peer(rx, true);
    if (session != nullptr) {
        session->encrypted = rx.encrypted;
        session->bonded = rx.bonded;
        session->transportAuthenticated = rx.authenticated;
    }
}

void DeviceRuntime::sessionDisconnected(const RxContext& rx) {
    if (rx.sessionId == 0U) return;
    if (reliableOutbox_ != nullptr) {
        reliableOutbox_->sessionClosed(rx);
    }
    telemetryLeases_.sessionClosed(rx);
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId) {
            retirePeer(session);
            return;
        }
    }
    IControllerControlEndpoint* control = transports_.accessAt(rx.transportId).control;
    if (control != nullptr) control->controllerSessionClosed(rx);
}

void DeviceRuntime::retirePeer(PeerSession& session) {
    if (timeSync_ != nullptr) timeSync_->sessionClosed(session.transportId, session.sessionId);
    const TransportAccess access = transports_.accessAt(session.transportId);
    IAuthorizationProvider* authorizationProvider = access.authorization;
    IControllerControlEndpoint* controllerControlEndpoint = access.control;
    // A transport can become unavailable without a native disconnect callback
    // (e.g. MQTT credential failure). Retire its control window as well as its
    // authorization, without resetting another transport's sessions.
    if (controllerControlEndpoint != nullptr) {
        RxContext closed;
        closed.transportId = session.transportId;
        closed.sessionId = session.sessionId;
        closed.encrypted = session.encrypted;
        closed.bonded = session.bonded;
        closed.authenticated = session.transportAuthenticated;
        controllerControlEndpoint->controllerSessionClosed(closed);
    }
    if (authorizationProvider != nullptr) {
        AuthorizationSessionContext security;
        security.transportId = session.transportId;
        security.sessionId = session.sessionId;
        security.encrypted = session.encrypted;
        security.bonded = session.bonded;
        security.transportAuthenticated = session.transportAuthenticated;
        authorizationProvider->sessionClosed(security);
    }
    session = PeerSession();
}

void DeviceRuntime::receive(ByteView encoded, const RxContext& rx) {
    incrementDiagnostic(receivedFrameCount_);
    PeerSession* existingPeer = peer(rx, false);
    if (existingPeer != nullptr) {
        existingPeer->encrypted = rx.encrypted;
        existingPeer->bonded = rx.bonded;
        existingPeer->transportAuthenticated = rx.authenticated;
    }
    bbp2::FrameView frame;
    Result result = bbp2::parseFrame(encoded, frame);
    if (!result || encoded.size > config_.maxFrameSize ||
        frame.header.bodyLength > config_.maxReassemblySize) {
        incrementDiagnostic(rejectedFrameCount_);
        sendError(WireError::MalformedMessage, 0, rx);
        return;
    }

    const bbp2::MessageKind kind =
        static_cast<bbp2::MessageKind>(frame.header.kind);
    if (kind == bbp2::MessageKind::Hello) {
        result = handleHello(frame, rx);
    } else if (kind == bbp2::MessageKind::Authenticate) {
        result = handleAuthenticate(frame, rx);
    } else if (kind == bbp2::MessageKind::Command) {
        result = handleCommand(frame, rx);
    } else if (kind == bbp2::MessageKind::Patch) {
        result = handlePatch(frame, rx);
    } else if (kind == bbp2::MessageKind::Event) {
        result = handleEvent(frame, rx);
    } else if (kind == bbp2::MessageKind::Ack) {
        result = handleAck(frame, rx);
    } else if (kind == bbp2::MessageKind::Error) {
        result = handleError(frame, rx);
    } else if (kind == bbp2::MessageKind::TimeResponse) {
        if (timeSync_ != nullptr) (void)timeSync_->poll(selectedTimePeer());
        result = timeSync_ != nullptr && existingPeer != nullptr
            ? timeSync_->receive(timePeer(*existingPeer), frame)
            : Result::failure(ErrorCode::UnsupportedFeature);
    } else if (kind == bbp2::MessageKind::ManifestRequest) {
        result = handleManifestRequest(frame, rx);
    } else if (kind == bbp2::MessageKind::ManifestAccept) {
        result = handleManifestAccept(frame, rx);
    } else if (kind == bbp2::MessageKind::StateRequest) {
        result = handleStateRequest(frame, rx);
    } else if (kind == bbp2::MessageKind::TelemetryControl) {
        result = handleTelemetryControl(frame, rx);
    } else if (kind == bbp2::MessageKind::ControllerControlOpen) {
        result = handleControllerControlOpen(frame, rx);
    } else if (kind == bbp2::MessageKind::ControllerMutation) {
        result = handleControllerMutation(frame, rx);
    } else if (kind == bbp2::MessageKind::PresenceKeyMutation) {
        result = handlePresenceKeyMutation(frame, rx);
    } else if (kind == bbp2::MessageKind::Manifest ||
               kind == bbp2::MessageKind::AuthResult ||
               kind == bbp2::MessageKind::StatePage ||
               kind == bbp2::MessageKind::TelemetryStatus ||
               kind == bbp2::MessageKind::TelemetryData ||
               kind == bbp2::MessageKind::ControllerControlChallenge ||
               kind == bbp2::MessageKind::ControllerMutationReceipt ||
               kind == bbp2::MessageKind::PresenceKeyReceipt) {
        // Device-originated response kinds are ignored unless the peer
        // explicitly asks for an acknowledgement.
        if ((frame.header.flags & bbp2::FlagAckRequired) != 0U) {
            result = Result::failure(ErrorCode::UnsupportedFeature);
        } else {
            return;
        }
    } else {
        result = Result::failure(ErrorCode::UnsupportedFeature);
    }

    if (!result) {
        incrementDiagnostic(rejectedFrameCount_);
        // Never answer a malformed response with another ERROR: two peers
        // doing so would create an error loop. Requests still receive a
        // correlated wire error.
        if ((frame.header.flags & bbp2::FlagIsResponse) == 0U &&
            kind != bbp2::MessageKind::Error) {
            const bool stateConflict =
                result.code() == ErrorCode::StateConflict;
            sendError(
                mapError(result.code()),
                frame.header.sequence,
                rx,
                stateConflict,
                stateRevision_);
        }
    }
}

Result DeviceRuntime::handleControllerControlOpen(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    IControllerControlEndpoint* controllerControlEndpoint =
        transports_.accessAt(rx.transportId).control;
    if (frame.header.flags != bbp2::FlagNone) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (controllerControlEndpoint == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) &
         bbp2::FeatureControllerControl) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!localTransport(rx.transportId) || !rx.encrypted ||
        !explicitlyAuthorizedLocal(
            rx,
            kAuthorizationPermissionControl)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    Result result = bbp2::decodeControllerControlOpenBody(frame.body);
    ByteView nonce;
    if (result) {
        result = controllerControlEndpoint->beginControlWindow(
            rx,
            nonce);
    }
    bbp2::ControllerControlChallengeBody challenge;
    challenge.controlNonce = nonce;
    ByteView encoded;
    if (result) {
        result = bbp2::encodeControllerControlChallengeBody(
            challenge,
            MutableByteSpan(
                transmitBuffer_.data + bbp2::kBaseHeaderSize,
                transmitBuffer_.size - bbp2::kBaseHeaderSize),
            encoded);
    }
    return result
               ? sendEncodedBody(
                     bbp2::MessageKind::ControllerControlChallenge,
                     bbp2::FlagIsResponse,
                     frame.header.sequence,
                     encoded.size,
                     responseTarget(rx))
               : result;
}

Result DeviceRuntime::handleControllerMutation(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    IControllerControlEndpoint* controllerControlEndpoint =
        transports_.accessAt(rx.transportId).control;
    if (frame.header.flags != bbp2::FlagNone) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (controllerControlEndpoint == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) &
         bbp2::FeatureControllerControl) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!localTransport(rx.transportId) || !rx.encrypted ||
        !explicitlyAuthorizedLocal(
            rx,
            kAuthorizationPermissionControl)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    bbp2::ControllerMutationBody mutation;
    Result result = bbp2::decodeControllerMutationBody(
        frame.body,
        mutation);
    const SendTarget target = responseTarget(rx);
    const size_t maximumFrame = maximumOutboundFrameSize(target);
    if (result && maximumFrame <
                      bbp2::kControllerMutationReceiptMaximumFrameSize) {
        result = Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView receipt;
    if (result) {
        result = controllerControlEndpoint->applyControllerMutation(
            rx,
            mutation.grant,
            mutation.controllerSecret,
            MutableByteSpan(
                transmitBuffer_.data + bbp2::kBaseHeaderSize,
                maximumFrame - bbp2::kBaseHeaderSize),
            receipt);
    }
    return result
               ? sendEncodedBody(
                     bbp2::MessageKind::ControllerMutationReceipt,
                     bbp2::FlagIsResponse,
                     frame.header.sequence,
                     receipt.size,
                     target)
               : result;
}

Result DeviceRuntime::handlePresenceKeyMutation(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    IControllerControlEndpoint* controllerControlEndpoint =
        transports_.accessAt(rx.transportId).control;
    if (frame.header.flags != bbp2::FlagNone) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (controllerControlEndpoint == nullptr ||
        !controllerControlEndpoint->supportsPresenceKeyControl()) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) &
         bbp2::FeaturePresenceKeyControl) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!localTransport(rx.transportId) || !rx.encrypted ||
        !explicitlyAuthorizedLocal(
            rx,
            kAuthorizationPermissionManageControllers)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    const SendTarget target = responseTarget(rx);
    const size_t maximumFrame = maximumOutboundFrameSize(target);
    if (maximumFrame <
        bbp2::kBaseHeaderSize + kPresenceKeyReceiptMaxEncodedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView receipt;
    Result result = controllerControlEndpoint->applyPresenceKeyMutation(
        rx,
        frame.body,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            maximumFrame - bbp2::kBaseHeaderSize),
        receipt);
    return result
               ? sendEncodedBody(
                     bbp2::MessageKind::PresenceKeyReceipt,
                     bbp2::FlagIsResponse,
                     frame.header.sequence,
                     receipt.size,
                     target)
               : result;
}

Result DeviceRuntime::handleHello(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    IAuthorizationProvider* authorizationProvider =
        transports_.accessAt(rx.transportId).authorization;
    bbp2::HelloBody hello;
    Result result = bbp2::decodeHelloBody(frame.body, hello);
    if (!result) return result;
    bool supportsVersion = false;
    for (uint8_t index = 0; index < hello.versionCount; ++index) {
        if (hello.versions[index] == bbp2::kVersion) {
            supportsVersion = true;
            break;
        }
    }
    if (!supportsVersion) {
        return Result::failure(ErrorCode::UnsupportedVersion);
    }
    PeerSession* session = peer(rx, true);
    if (session == nullptr) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    session->helloComplete = true;
    session->negotiatedFeatures =
        hello.features & transportFeatures(rx.transportId);
    session->remoteMaxFrameSize = hello.maxFrameSize;
    if (hello.maxFrameSize < bbp2::kBaseHeaderSize + bbp2::kTimeResponseMaxSize)
        session->negotiatedFeatures &= ~static_cast<uint32_t>(bbp2::FeatureTimeSync);
    if (hello.maxFrameSize <
        bbp2::kControllerMutationReceiptMaximumFrameSize) {
        session->negotiatedFeatures &=
            ~static_cast<uint32_t>(bbp2::FeatureControllerControl);
    }
    if (hello.maxFrameSize <
        bbp2::kPresenceKeyReceiptMaximumFrameSize) {
        session->negotiatedFeatures &=
            ~static_cast<uint32_t>(bbp2::FeaturePresenceKeyControl);
    }
    session->stateObserved = false;
    session->snapshotCursor = 0U;
    session->snapshotNextCursor = 0U;
    session->snapshotIdMode = false;
    session->idReady =
        (session->negotiatedFeatures & bbp2::FeatureEndpointIds) != 0U &&
        hello.hasManifestRevision && hello.hasManifestFingerprint &&
        hello.manifestRevision == config_.manifestRevision &&
        hasManifestFingerprint_ &&
        memcmp(
            hello.manifestFingerprint.data,
            manifestFingerprint_,
            sizeof(manifestFingerprint_)) == 0;
    session->remoteReliableReceiveWindow =
        (session->negotiatedFeatures &
         bbp2::FeatureReliableDelivery) != 0U
            ? hello.reliableReceiveWindow
            : 0U;
    if (authorizationProvider != nullptr &&
        (session->negotiatedFeatures &
         bbp2::FeatureAuthentication) != 0U) {
        bool commonMethod = false;
        for (uint8_t peerMethod = 0;
             peerMethod < hello.authorizationMethodCount && !commonMethod;
             ++peerMethod) {
            for (size_t localMethod = 0;
                 localMethod < authorizationProvider->methodCount();
                 ++localMethod) {
                if (hello.authorizationMethods[peerMethod] ==
                    authorizationProvider->methodAt(localMethod)) {
                    commonMethod = true;
                    break;
                }
            }
        }
        if (!commonMethod) {
            session->negotiatedFeatures &=
                ~static_cast<uint32_t>(bbp2::FeatureAuthentication);
        }
    }
    session->encrypted = rx.encrypted;
    session->bonded = rx.bonded;
    session->transportAuthenticated = rx.authenticated;

    if ((frame.header.flags & bbp2::FlagIsResponse) == 0U) {
        result = sendHello(
            responseTarget(rx),
            bbp2::FlagIsResponse,
            frame.header.sequence);
        if (!result) return result;
    }
    session->helloSent = true;
    if ((frame.header.flags & bbp2::FlagAckRequired) != 0U) {
        return sendAck(frame.header.sequence, rx);
    }
    return Result::success();
}

Result DeviceRuntime::handleCommand(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if ((frame.header.flags & bbp2::FlagIsResponse) != 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!authorizedFor(rx, kAuthorizationPermissionControl)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!negotiated(rx)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (commandHandler_ == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    PeerSession* session = peer(rx, false);
    const bool reliable =
        session != nullptr &&
        (session->negotiatedFeatures &
         bbp2::FeatureReliableDelivery) != 0U &&
        (frame.header.flags & bbp2::FlagAckRequired) != 0U;
    uint8_t fingerprint[kRequestFingerprintSize] = {};
    if (reliable) {
        bool replayed = false;
        Result replayResult = replayReliableRequest(
            frame, rx, *session, fingerprint, replayed);
        if (!replayResult || replayed) return replayResult;
    }

    EndpointVisitContext context;
    context.runtime = this;
    context.rx = &rx;
    context.handler = commandHandler_;
    context.handlerContext = commandContext_;
    context.requiredAccess = static_cast<uint8_t>(
        AccessCommand | AccessWrite);
    const bool idMode =
        (frame.header.flags & bbp2::FlagIdMode) != 0U;
    Result result = idMode && !idModeReady(rx)
                        ? Result::failure(ErrorCode::NotConfigured)
                        : (idMode
                               ? bbp2::visitIdBody(
                                     frame.body,
                                     &validateIdCommandValue,
                                     &endpoints_)
                               : bbp2::visitKeyedBody(
                                     frame.body,
                                     &validateCommandValue,
                                     &endpoints_));
    if (result) {
        result = idMode
                     ? bbp2::visitIdBody(
                           frame.body,
                           &DeviceRuntime::endpointIdVisitThunk,
                           &context)
                     : bbp2::visitKeyedBody(
                           frame.body,
                           &DeviceRuntime::endpointVisitThunk,
                           &context);
    }
    if (result && stateSettlementHandler_ != nullptr) {
        result = stateSettlementHandler_(stateSettlementContext_, nullptr);
    }
    if (reliable) {
        RequestReplayRecord& record = rememberRequest(
            *session,
            frame.header.sequence,
            fingerprint,
            result ? RequestReplayResponse::Ack
                   : RequestReplayResponse::Error,
            result ? WireError::MalformedMessage
                   : mapError(result.code()));
        secureZero(MutableByteSpan(fingerprint, sizeof(fingerprint)));
        if (result) {
            sendRequestAck(record, rx);
            return Result::success();
        }
    }
    if (!result) return result;
    if ((frame.header.flags & bbp2::FlagAckRequired) != 0U) {
        return sendAck(frame.header.sequence, rx);
    }
    return Result::success();
}

Result DeviceRuntime::handlePatch(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if ((frame.header.flags & bbp2::FlagIsResponse) != 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!authorizedFor(rx, kAuthorizationPermissionControl)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) & bbp2::FeatureStateRevision) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (stateApplyHandler_ == nullptr &&
        stateApplyTransactionHandler_ == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    PeerSession* session = peer(rx, false);
    const bool reliable =
        session != nullptr &&
        (session->negotiatedFeatures &
         bbp2::FeatureReliableDelivery) != 0U &&
        (frame.header.flags & bbp2::FlagAckRequired) != 0U;
    // Apply is a state mutation. It is only safe after the peer observed a
    // snapshot in this session and when retries can be deduplicated.
    if (!reliable || !session->stateObserved) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    uint8_t fingerprint[kRequestFingerprintSize] = {};
    bool replayed = false;
    Result result = replayReliableRequest(
        frame, rx, *session, fingerprint, replayed);
    if (!result || replayed) return result;

    bbp2::StatePatchBody patch;
    result = bbp2::decodeStatePatchBody(frame.body, patch);
    const bool idMode =
        (frame.header.flags & bbp2::FlagIdMode) != 0U;
    if (result && patch.mode != bbp2::StatePatchMode::Apply) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result && idMode && !session->idReady) {
        result = Result::failure(ErrorCode::NotConfigured);
    }
    if (result) {
        result = idMode
                     ? bbp2::visitIdBody(
                           patch.values,
                           &validateIdApplyValue,
                           &endpoints_)
                     : bbp2::visitKeyedBody(
                           patch.values,
                           &validateApplyValue,
                           &endpoints_);
    }
    if (result && stateSettlementHandler_ != nullptr) {
        result = stateSettlementHandler_(stateSettlementContext_, nullptr);
    }
    if (result && patch.revision != stateRevision_) {
        result = Result::failure(ErrorCode::StateConflict);
    }
    if (result && stateRevision_ == UINT32_MAX) {
        result = Result::failure(ErrorCode::CapacityExceeded);
    }
    bool changed = false;
    if (result && stateApplyTransactionHandler_ != nullptr) {
        result = stateApplyTransactionHandler_(
            stateApplyTransactionContext_,
            patch.values,
            idMode,
            rx,
            changed);
    } else if (result) {
        EndpointVisitContext context;
        context.runtime = this;
        context.rx = &rx;
        context.handler = stateApplyHandler_;
        context.handlerContext = stateApplyContext_;
        context.requiredAccess = AccessWrite;
        result = idMode
                     ? bbp2::visitIdBody(
                           patch.values,
                           &DeviceRuntime::endpointIdVisitThunk,
                           &context)
                     : bbp2::visitKeyedBody(
                           patch.values,
                           &DeviceRuntime::endpointVisitThunk,
                           &context);
        changed = result.ok();
    }
    if (result && changed) {
        ++stateRevision_;
        // The applying peer learns the accepted new revision from ACK and may
        // continue optimistic writes. Every other peer must complete a fresh
        // snapshot before it can write against the new revision.
        invalidateStateObservations(session);
    }

    RequestReplayRecord& record = rememberRequest(
        *session,
        frame.header.sequence,
        fingerprint,
        result ? RequestReplayResponse::Ack
               : RequestReplayResponse::Error,
        result ? WireError::MalformedMessage : mapError(result.code()),
        result || result.code() == ErrorCode::StateConflict,
        stateRevision_);
    secureZero(MutableByteSpan(fingerprint, sizeof(fingerprint)));
    if (!result) return result;
    sendRequestAck(record, rx);
    // ACK is the source peer's confirmation. Other authorized peers receive
    // a best-effort full keyed snapshot at the same revision. Full keyed
    // state avoids forwarding source-specific endpoint IDs and also leaves
    // room for the application/hardware layer to normalize accepted values.
    if (changed) fanOutCurrentState(&rx);
    return Result::success();
}

Result DeviceRuntime::handleEvent(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if ((frame.header.flags & bbp2::FlagIsResponse) != 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    if (!authorizedFor(rx, kAuthorizationPermissionControl)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!negotiated(rx)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (eventHandler_ == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    PeerSession* session = peer(rx, false);
    const bool reliable =
        session != nullptr &&
        (session->negotiatedFeatures &
         bbp2::FeatureReliableDelivery) != 0U &&
        (frame.header.flags & bbp2::FlagAckRequired) != 0U;
    uint8_t fingerprint[kRequestFingerprintSize] = {};
    if (reliable) {
        bool replayed = false;
        Result replayResult = replayReliableRequest(
            frame, rx, *session, fingerprint, replayed);
        if (!replayResult || replayed) return replayResult;
    }

    const bool idMode =
        (frame.header.flags & bbp2::FlagIdMode) != 0U;
    Result result = idMode && !idModeReady(rx)
                        ? Result::failure(ErrorCode::NotConfigured)
                        : (idMode
                               ? bbp2::visitIdBody(
                                     frame.body,
                                     &validateIdEventValue,
                                     &endpoints_)
                               : bbp2::visitKeyedBody(
                                     frame.body,
                                     &validateEventValue,
                                     &endpoints_));
    if (result) {
        EndpointVisitContext context;
        context.runtime = this;
        context.rx = &rx;
        context.handler = eventHandler_;
        context.handlerContext = eventContext_;
        context.requiredAccess = AccessEvent;
        result = idMode
                     ? bbp2::visitIdBody(
                           frame.body,
                           &DeviceRuntime::endpointIdVisitThunk,
                           &context)
                     : bbp2::visitKeyedBody(
                           frame.body,
                           &DeviceRuntime::endpointVisitThunk,
                           &context);
    }
    if (reliable) {
        RequestReplayRecord& record = rememberRequest(
            *session,
            frame.header.sequence,
            fingerprint,
            result ? RequestReplayResponse::Ack
                   : RequestReplayResponse::Error,
            result ? WireError::MalformedMessage
                   : mapError(result.code()));
        secureZero(MutableByteSpan(fingerprint, sizeof(fingerprint)));
        if (result) {
            sendRequestAck(record, rx);
            return Result::success();
        }
    }
    if (!result) return result;
    if ((frame.header.flags & bbp2::FlagAckRequired) != 0U) {
        return sendAck(frame.header.sequence, rx);
    }
    return Result::success();
}

Result DeviceRuntime::handleAck(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (frame.header.flags != bbp2::FlagIsResponse) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    bbp2::AckBody body;
    Result result = bbp2::decodeAckBody(frame.body, body);
    if (!result) return result;
    if (body.acknowledgedSequence == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    // Pre-auth HELLO acknowledgements remain legal, but only an authorized
    // peer may complete an application reliable-delivery record.
    if (reliableOutbox_ != nullptr &&
        authorizedFor(rx, kAuthorizationPermissionObserve)) {
        reliableOutbox_->acknowledge(body.acknowledgedSequence, rx);
    }
    return Result::success();
}

Result DeviceRuntime::handleError(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (frame.header.flags != bbp2::FlagIsResponse) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    bbp2::ErrorBody body;
    Result result = bbp2::decodeErrorBody(frame.body, body);
    if (!result) return result;
    PeerSession* timeSession = peer(rx, false);
    if (timeSync_ != nullptr && timeSession != nullptr && body.hasRelatedSequence)
        (void)timeSync_->receive(timePeer(*timeSession), frame);
    if (reliableOutbox_ != nullptr &&
        authorizedFor(rx, kAuthorizationPermissionObserve) &&
        body.hasRelatedSequence &&
        body.relatedSequence != 0U) {
        reliableOutbox_->reject(
            body.relatedSequence,
            body.errorCode,
            rx);
    }
    if (remoteErrorHandler_ != nullptr &&
        authorizedFor(rx, kAuthorizationPermissionObserve)) {
        remoteErrorHandler_(
            remoteErrorContext_,
            static_cast<WireError>(body.errorCode),
            body.hasRelatedSequence ? body.relatedSequence : 0U,
            rx);
    }
    return Result::success();
}

Result DeviceRuntime::handleAuthenticate(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    IAuthorizationProvider* authorizationProvider =
        transports_.accessAt(rx.transportId).authorization;
    if (!negotiated(rx)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!localTransport(rx.transportId) ||
        (!rx.encrypted &&
         (authorizationProvider == nullptr ||
          !authorizationProvider->establishesSecureTransport()))) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (authorizationProvider != nullptr &&
        (negotiatedFeatures(rx) &
         bbp2::FeatureAuthentication) == 0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (authorizationProvider == nullptr ||
        (frame.header.flags &
         static_cast<uint8_t>(bbp2::FlagIsResponse | bbp2::FlagIdMode)) !=
            0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    bbp2::AuthRequestBody request;
    Result result = bbp2::decodeAuthRequestBody(frame.body, request);
    if (!result) return result;
    bool supportedMethod = false;
    for (size_t index = 0; index < authorizationProvider->methodCount(); ++index) {
        if (authorizationProvider->methodAt(index) == request.method) {
            supportedMethod = true;
            break;
        }
    }
    if (!supportedMethod) return Result::failure(ErrorCode::UnsupportedFeature);
    PeerSession* session = peer(rx, false);
    if (session == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (session->authorizationMethod != 0U &&
        session->authorizationMethod != request.method) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    bbp2::AuthResultBody response;
    response.method = request.method;
    if (session->authorizationLocked) {
        response.status = bbp2::AuthStatus::Locked;
        const Result sendResult =
            sendAuthResult(frame.header.sequence, response, rx);
        if (sendResult) {
            (void)transports_.disconnectSession(
                rx.transportId,
                rx.sessionId);
        }
        return sendResult;
    }
    if (session->authorizationPermissions != 0U) {
        response.status = bbp2::AuthStatus::Authorized;
        return sendAuthResult(frame.header.sequence, response, rx);
    }
    session->authorizationMethod = request.method;
    AuthorizationSessionContext security;
    security.transportId = rx.transportId;
    security.sessionId = rx.sessionId;
    security.encrypted = rx.encrypted;
    security.bonded = rx.bonded;
    security.transportAuthenticated = rx.authenticated;
    AuthorizationDecision decision;
    result = authorizationProvider->authorize(
        request.method,
        request.payload,
        security,
        decision);
    if (!result) return result;
    if (static_cast<uint8_t>(decision.status) >
            static_cast<uint8_t>(bbp2::AuthStatus::Rejected) ||
        (decision.responsePayload.size != 0U &&
         decision.responsePayload.data == nullptr) ||
        (decision.status != bbp2::AuthStatus::Continue &&
         !decision.responsePayload.empty()) ||
        (decision.status == bbp2::AuthStatus::Authorized
             ? !validAuthorizationPermissions(decision.permissions)
             : decision.permissions != 0U)) {
        authorizationProvider->sessionClosed(security);
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (decision.status == bbp2::AuthStatus::Rejected) {
        ++session->authorizationAttempts;
        if (session->authorizationAttempts >=
            config_.maxAuthorizationAttempts) {
            session->authorizationLocked = true;
            decision.status = bbp2::AuthStatus::Locked;
            authorizationProvider->sessionClosed(security);
        }
    }
    response.status = decision.status;
    response.payload = decision.responsePayload;
    response.hasPayload = !decision.responsePayload.empty();
    result = sendAuthResult(frame.header.sequence, response, rx);
    if (decision.status == bbp2::AuthStatus::Authorized) {
        if (!result) {
            authorizationProvider->sessionClosed(security);
            return result;
        }
        authorizationProvider->authorizationResultQueued(security);
        session->authorizationPermissions = decision.permissions;
    } else if (decision.status == bbp2::AuthStatus::Locked && result) {
        (void)transports_.disconnectSession(
            rx.transportId,
            rx.sessionId);
    }
    return result;
}

Result DeviceRuntime::handleManifestRequest(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (!authorizedFor(rx, kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) & bbp2::FeatureManifest) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if ((config_.protocolFeatures & bbp2::FeatureManifest) == 0U) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (frame.header.flags != bbp2::FlagNone ||
        frame.header.sequence == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    bbp2::ManifestRequestBody request;
    Result result = bbp2::decodeManifestRequestBody(frame.body, request);
    if (!result) return result;
    const SendTarget target = responseTarget(rx);
    const size_t maximumFrame = maximumOutboundFrameSize(target);
    if (maximumFrame <= bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView manifest;
    uint16_t nextCursor = request.cursor;
    result = encodeManifestPage(
        config_.manifestRevision,
        endpoints_,
        ByteView(manifestFingerprint_, sizeof(manifestFingerprint_)),
        request.cursor,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            maximumFrame - bbp2::kBaseHeaderSize),
        manifest,
        nextCursor);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::Manifest,
        bbp2::FlagIsResponse,
        frame.header.sequence,
        manifest.size,
        target);
}

Result DeviceRuntime::handleManifestAccept(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (!authorizedFor(rx, kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    PeerSession* session = peer(rx, false);
    const uint32_t required =
        bbp2::FeatureManifest | bbp2::FeatureEndpointIds;
    if (session == nullptr || !session->helloComplete ||
        (session->negotiatedFeatures & required) != required) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (frame.header.flags != bbp2::FlagAckRequired ||
        frame.header.sequence == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    bbp2::ManifestAcceptBody accept;
    Result result = bbp2::decodeManifestAcceptBody(frame.body, accept);
    if (!result) return result;
    if (!hasManifestFingerprint_ ||
        accept.revision != config_.manifestRevision ||
        memcmp(
            accept.fingerprint.data,
            manifestFingerprint_,
            sizeof(manifestFingerprint_)) != 0) {
        session->idReady = false;
        return Result::failure(ErrorCode::ManifestConflict);
    }
    session->idReady = true;
    return sendAck(frame.header.sequence, rx);
}

Result DeviceRuntime::handleStateRequest(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (!authorizedFor(rx, kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) & bbp2::FeatureStateRevision) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (statePageEncoder_ == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    const uint8_t allowedFlags = bbp2::FlagIdMode;
    if ((frame.header.flags & ~allowedFlags) != 0U ||
        frame.header.sequence == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    const bool idMode =
        (frame.header.flags & bbp2::FlagIdMode) != 0U;
    if (idMode && !idModeReady(rx)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    bbp2::StateRequestBody request;
    Result result = bbp2::decodeStateRequestBody(frame.body, request);
    if (!result) return result;
    PeerSession* session = peer(rx, false);
    if (session == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (stateSettlementHandler_ != nullptr) {
        result = stateSettlementHandler_(stateSettlementContext_, &rx);
        if (!result) return result;
    }
    if (request.cursor != 0U) {
        const bool sameRevision =
            request.hasObservedRevision &&
            request.observedRevision == stateRevision_;
        const bool expectedOrDuplicate =
            request.cursor == session->snapshotNextCursor ||
            request.cursor == session->snapshotCursor;
        if (session->snapshotNextCursor == 0U || !sameRevision ||
            !expectedOrDuplicate || session->snapshotIdMode != idMode) {
            return Result::failure(ErrorCode::StateConflict);
        }
    }

    const SendTarget target = responseTarget(rx);
    const size_t maximumFrame = maximumOutboundFrameSize(target);
    if (maximumFrame <= bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    const uint32_t snapshotRevision = stateRevision_;
    ByteView responseBody;
    uint16_t nextCursor = request.cursor;
    uint16_t totalFields = 0U;
    result = statePageEncoder_(
        statePageContext_,
        snapshotRevision,
        request.cursor,
        idMode,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            maximumFrame - bbp2::kBaseHeaderSize),
        responseBody,
        nextCursor,
        totalFields);
    if (!result) return result;
    if (stateRevision_ != snapshotRevision) {
        return Result::failure(ErrorCode::StateConflict);
    }
    bbp2::StatePageBody page;
    result = bbp2::decodeStatePageBody(responseBody, page);
    if (!result || page.revision != snapshotRevision ||
        page.cursor != request.cursor ||
        page.nextCursor != nextCursor ||
        page.totalFields != totalFields ||
        totalFields != endpoints_.size()) {
        return result
                   ? Result::failure(ErrorCode::InternalError)
                   : result;
    }
    StatePageValidationContext validation;
    validation.endpoints = &endpoints_;
    validation.cursor = page.cursor;
    validation.nextCursor = page.nextCursor;
    result = idMode
                 ? bbp2::visitIdBody(
                       page.values,
                       &validateStatePageIdValue,
                       &validation)
                 : bbp2::visitKeyedBody(
                       page.values,
                       &validateStatePageKeyedValue,
                       &validation);
    if (!result) return result;
    result = sendEncodedBody(
        bbp2::MessageKind::StatePage,
        static_cast<uint8_t>(
            bbp2::FlagIsResponse |
            (idMode ? bbp2::FlagIdMode : bbp2::FlagNone)),
        frame.header.sequence,
        responseBody.size,
        target);
    if (result) {
        session->snapshotCursor = request.cursor;
        session->snapshotNextCursor = nextCursor;
        session->snapshotIdMode = idMode;
        session->stateObserved = nextCursor == totalFields;
    }
    return result;
}

Result DeviceRuntime::handleTelemetryControl(
    const bbp2::FrameView& frame,
    const RxContext& rx) {
    if (telemetryClock_ == nullptr || telemetrySampleEncoder_ == nullptr) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    if (!authorizedFor(rx, kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    const uint32_t required =
        bbp2::FeatureManifest | bbp2::FeatureEndpointIds |
        bbp2::FeatureTelemetry;
    if (!negotiated(rx) ||
        (negotiatedFeatures(rx) & required) != required ||
        !idModeReady(rx)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (frame.header.flags != bbp2::FlagIdMode ||
        frame.header.sequence == 0U) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    bbp2::TelemetryControlBody control;
    Result result = bbp2::decodeTelemetryControlBody(frame.body, control);
    if (!result) return result;
    bbp2::TelemetryStatusBody status;
    result = telemetryLeases_.apply(
        control,
        rx,
        endpoints_,
        telemetryClock_->monotonicMillis(),
        status);
    if (!result) return result;
    return sendTelemetryStatus(
        frame.header.sequence,
        status,
        responseTarget(rx),
        true);
}

Result DeviceRuntime::endpointVisitThunk(
    void* context,
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType) {
    EndpointVisitContext* visit = static_cast<EndpointVisitContext*>(context);
    if (visit == nullptr || visit->runtime == nullptr || visit->rx == nullptr ||
        visit->handler == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    return visit->runtime->visitEndpointValue(
        endpointKey,
        encodedValue,
        valueType,
        *visit);
}

Result DeviceRuntime::endpointIdVisitThunk(
    void* context,
    uint16_t endpointId,
    ByteView encodedValue,
    cbor::Type valueType) {
    EndpointVisitContext* visit = static_cast<EndpointVisitContext*>(context);
    if (visit == nullptr || visit->runtime == nullptr || visit->rx == nullptr ||
        visit->handler == nullptr) {
        return Result::failure(ErrorCode::InternalError);
    }
    const EndpointDescriptor* endpoint =
        visit->runtime->endpoints_.findById(endpointId);
    if (endpoint == nullptr) return Result::failure(ErrorCode::NotFound);
    return visit->runtime->visitEndpointValue(
        *endpoint,
        encodedValue,
        valueType,
        *visit);
}

Result DeviceRuntime::visitEndpointValue(
    StringView endpointKey,
    ByteView encodedValue,
    cbor::Type valueType,
    const EndpointVisitContext& visit) {
    const EndpointDescriptor* endpoint = endpoints_.find(endpointKey);
    if (endpoint == nullptr) {
        return Result::failure(ErrorCode::NotFound);
    }
    return visitEndpointValue(*endpoint, encodedValue, valueType, visit);
}

Result DeviceRuntime::visitEndpointValue(
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    const EndpointVisitContext& visit) {
    Result result = validateEndpointValue(
        endpoint,
        encodedValue,
        valueType,
        visit.requiredAccess);
    if (!result) return result;
    return visit.handler(
        visit.handlerContext,
        endpoint,
        encodedValue,
        valueType,
        *visit.rx);
}

Result DeviceRuntime::sendHello(
    const SendTarget& target,
    uint8_t flags,
    uint16_t sequence) {
    bbp2::HelloBody hello;
    hello.role = bbp2::PeerRole::Device;
    hello.versions[0] = bbp2::kVersion;
    hello.versionCount = 1;
    hello.features = transportFeatures(target.transportId);
    IAuthorizationProvider* authorizationProvider =
        transports_.accessAt(target.transportId).authorization;
    hello.maxFrameSize = config_.maxFrameSize;
    hello.maxReassemblySize = config_.maxReassemblySize;
    if ((config_.protocolFeatures & bbp2::FeatureManifest) != 0U) {
        hello.manifestRevision = config_.manifestRevision;
        hello.hasManifestRevision = true;
    }
    if (hasManifestFingerprint_) {
        hello.manifestFingerprint = ByteView(
            manifestFingerprint_,
            sizeof(manifestFingerprint_));
        hello.hasManifestFingerprint = true;
    }
    if (authorizationProvider != nullptr) {
        const size_t count = authorizationProvider->methodCount();
        if (count == 0U || count > bbp2::kMaxAuthorizationMethods) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        hello.authorizationMethodCount = static_cast<uint8_t>(count);
        for (size_t index = 0; index < count; ++index) {
            hello.authorizationMethods[index] =
                authorizationProvider->methodAt(index);
        }
    }
    if ((config_.protocolFeatures &
         bbp2::FeatureReliableDelivery) != 0U) {
        hello.reliableReceiveWindow = config_.reliableReceiveWindow;
    }

    ByteView body;
    Result result = bbp2::encodeHelloBody(
        hello,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::Hello,
        flags,
        sequence,
        body.size,
        target);
}

Result DeviceRuntime::sendAck(
    uint16_t acknowledgedSequence,
    const RxContext& rx,
    bool hasStateRevision,
    uint32_t stateRevision) {
    bbp2::AckBody ack;
    ack.acknowledgedSequence = acknowledgedSequence;
    ack.hasStateRevision = hasStateRevision;
    ack.stateRevision = stateRevision;
    ByteView body;
    Result result = bbp2::encodeAckBody(
        ack,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::Ack,
        bbp2::FlagIsResponse,
        nextSequence(),
        body.size,
        responseTarget(rx));
}

Result DeviceRuntime::sendTelemetryStatus(
    uint16_t sequence,
    const bbp2::TelemetryStatusBody& status,
    const SendTarget& target,
    bool response) {
    ByteView body;
    Result result = bbp2::encodeTelemetryStatusBody(
        status,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::TelemetryStatus,
        static_cast<uint8_t>(
            bbp2::FlagIdMode |
            (response ? bbp2::FlagIsResponse : bbp2::FlagNone)),
        sequence,
        body.size,
        target);
}

Result DeviceRuntime::sendTelemetrySample(
    TelemetryLeaseSlot& lease,
    uint32_t nowMillis) {
    SendTarget target;
    target.kind = lease.sessionId == 0U
                      ? SendTargetKind::Transport
                      : SendTargetKind::Session;
    target.transportId = lease.transportId;
    target.sessionId = lease.sessionId;
    PeerSession* session = peer(target);
    const uint32_t required =
        bbp2::FeatureManifest | bbp2::FeatureEndpointIds |
        bbp2::FeatureTelemetry;
    if (session == nullptr || !session->helloComplete || !session->idReady ||
        (session->negotiatedFeatures & required) != required ||
        !authorizedFor(*session, kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    const size_t maximumFrame = maximumOutboundFrameSize(target);
    const size_t reserve = bbp2::kBaseHeaderSize +
                           bbp2::kTelemetryDataEnvelopeReserve;
    if (maximumFrame <= reserve || transmitBuffer_.size <= reserve) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    const size_t valueCapacity = maximumFrame < transmitBuffer_.size
                                     ? maximumFrame - reserve
                                     : transmitBuffer_.size - reserve;
    bbp2::IdBodyWriter writer(MutableByteSpan(
        transmitBuffer_.data + reserve,
        valueCapacity));
    Result result = writer.begin(lease.fieldCount);
    if (result) {
        result = telemetrySampleEncoder_(
            telemetrySampleContext_,
            ByteView(lease.fields, sizeof(lease.fields)),
            lease.fieldCount,
            writer);
    }
    ByteView values;
    if (result) result = writer.finish(values);
    if (!result) return result;

    bbp2::TelemetryDataBody data;
    data.streamId = lease.streamId;
    data.streamEpoch = lease.streamEpoch;
    data.sampleSequence = lease.sampleSequence;
    data.monotonicMillis = nowMillis;
    data.values = values;
    ByteView body;
    result = bbp2::encodeTelemetryDataBody(
        data,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            maximumFrame - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::TelemetryData,
        bbp2::FlagIdMode,
        nextSequence(),
        body.size,
        target);
}

void DeviceRuntime::pollTelemetry() {
    if (telemetryClock_ == nullptr || telemetrySampleEncoder_ == nullptr) {
        return;
    }
    const uint32_t now = telemetryClock_->monotonicMillis();
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        TelemetryLeaseSlot* expired = telemetryLeases_.firstExpired(now);
        if (expired == nullptr) break;
        bbp2::TelemetryStatusBody status;
        status.streamId = expired->streamId;
        status.streamEpoch = expired->streamEpoch;
        status.status = bbp2::TelemetryStatusCode::Expired;
        status.effectiveIntervalMillis = expired->intervalMillis;
        status.leaseMillis = 0U;
        SendTarget target;
        target.kind = expired->sessionId == 0U
                          ? SendTargetKind::Transport
                          : SendTargetKind::Session;
        target.transportId = expired->transportId;
        target.sessionId = expired->sessionId;
        (void)sendTelemetryStatus(
            nextSequence(), status, target, false);
        incrementDiagnostic(telemetryCounters_.leasesExpired);
        telemetryLeases_.close(*expired);
    }
    for (size_t index = 0U; index < BLINKER_TELEMETRY_LEASE_SLOTS; ++index) {
        TelemetryLeaseSlot* due = telemetryLeases_.firstDue(now);
        if (due == nullptr) break;
        // Telemetry is best-effort and never enters ReliableOutbox. Consume
        // this sampling opportunity before sending so congestion drops the
        // old sample, advances sequence and cannot create a hot retry loop.
        telemetryLeases_.sampled(*due, now);
        const Result result = sendTelemetrySample(*due, now);
        if (result) {
            incrementDiagnostic(telemetryCounters_.samplesSent);
        } else {
            incrementDiagnostic(telemetryCounters_.samplesDropped);
        }
    }
}

Result DeviceRuntime::sendError(
    WireError error,
    uint16_t relatedSequence,
    const RxContext& rx,
    bool hasStateRevision,
    uint32_t stateRevision) {
    if (transmitBuffer_.data == nullptr ||
        transmitBuffer_.size < bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    bbp2::ErrorBody bodyValue;
    bodyValue.errorCode = static_cast<uint16_t>(error);
    if (relatedSequence != 0U) {
        bodyValue.relatedSequence = relatedSequence;
        bodyValue.hasRelatedSequence = true;
    }
    bodyValue.hasStateRevision = hasStateRevision;
    bodyValue.stateRevision = stateRevision;
    ByteView body;
    Result result = bbp2::encodeErrorBody(
        bodyValue,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::Error,
        bbp2::FlagIsResponse,
        nextSequence(),
        body.size,
        responseTarget(rx));
}

Result DeviceRuntime::sendAuthResult(
    uint16_t sequence,
    const bbp2::AuthResultBody& bodyValue,
    const RxContext& rx) {
    ByteView body;
    Result result = bbp2::encodeAuthResultBody(
        bodyValue,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    return sendEncodedBody(
        bbp2::MessageKind::AuthResult,
        bbp2::FlagIsResponse,
        sequence,
        body.size,
        responseTarget(rx));
}

Result DeviceRuntime::sendPatch(
    ByteView keyedBody,
    const SendTarget& target) {
    if (target.kind == SendTargetKind::Broadcast) {
        if (!started_) return Result::failure(ErrorCode::NotConfigured);
        Result result = bbp2::visitKeyedBody(
            keyedBody,
            &validatePatchValue,
            &endpoints_);
        if (!result) return result;
        uint32_t revision = 0U;
        result = advanceStateRevision(revision);
        return result ? fanOutCurrentPatch(keyedBody) : result;
    }
    return sendPatchInternal(
        keyedBody, false, false, true, target, nullptr);
}

Result DeviceRuntime::sendPatchById(
    ByteView idBody,
    const SendTarget& target) {
    return sendPatchInternal(
        idBody, true, false, true, target, nullptr);
}

Result DeviceRuntime::advanceStateRevision(uint32_t& revision) {
    revision = stateRevision_;
    if (stateRevision_ == UINT32_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    ++stateRevision_;
    invalidateStateObservations();
    revision = stateRevision_;
    return Result::success();
}

void DeviceRuntime::invalidateStateObservations(
    PeerSession* observingPeer) {
    for (size_t index = 0;
         index < BLINKER_MAX_PEER_SESSIONS;
         ++index) {
        PeerSession& session = peers_[index];
        if (!session.occupied) continue;
        session.stateObserved = &session == observingPeer;
        session.snapshotCursor = 0U;
        session.snapshotNextCursor = 0U;
        session.snapshotIdMode = false;
    }
}

Result DeviceRuntime::sendCurrentPatch(
    ByteView keyedBody,
    const SendTarget& target) {
    if (target.kind == SendTargetKind::Broadcast) {
        return fanOutCurrentPatch(keyedBody);
    }
    return sendPatchInternal(
        keyedBody, false, false, false, target, nullptr);
}

Result DeviceRuntime::sendCurrentPatchById(
    ByteView idBody,
    const SendTarget& target) {
    return sendPatchInternal(
        idBody, true, false, false, target, nullptr);
}

Result DeviceRuntime::fanOutCurrentPatch(
    ByteView keyedBody,
    const RxContext* exclude) {
    if (!started_ ||
        (config_.protocolFeatures & bbp2::FeatureStateRevision) == 0U) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result result = bbp2::visitKeyedBody(
        keyedBody,
        &validatePatchValue,
        &endpoints_);
    if (!result) return result;

    bbp2::StatePatchBody patch;
    patch.mode = bbp2::StatePatchMode::Report;
    patch.revision = stateRevision_;
    patch.values = keyedBody;
    ByteView body;
    result = bbp2::encodeStatePatchBody(
        patch,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        body);
    if (!result) return result;
    ByteView frame;
    result = prepareEncodedBody(
        bbp2::MessageKind::Patch,
        bbp2::FlagNone,
        nextSequence(),
        body.size,
        frame);
    return result
               ? fanOutPreparedFrame(
                     frame,
                     bbp2::FeatureStateRevision,
                     exclude)
               : result;
}

Result DeviceRuntime::fanOutCurrentState(const RxContext* exclude) {
    if (!started_ || stateEncoder_ == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    const size_t reserve = bbp2::kStatePatchEnvelopeReserve;
    if (transmitBuffer_.size < bbp2::kBaseHeaderSize + reserve) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    bbp2::KeyedBodyWriter writer(
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize + reserve,
            transmitBuffer_.size - bbp2::kBaseHeaderSize - reserve));
    Result result = stateEncoder_(stateContext_, writer);
    if (!result) return result;
    ByteView state;
    result = writer.finish(state);
    return result ? fanOutCurrentPatch(state, exclude) : result;
}

Result DeviceRuntime::broadcastPatch(ByteView keyedBody) {
    SendTarget target;
    target.kind = SendTargetKind::Broadcast;
    return sendPatch(keyedBody, target);
}

Result DeviceRuntime::sendReliablePatch(
    ByteView keyedBody,
    const SendTarget& target,
    uint16_t& sequence) {
    return sendPatchInternal(
        keyedBody, false, true, true, target, &sequence);
}

Result DeviceRuntime::sendReliablePatchById(
    ByteView idBody,
    const SendTarget& target,
    uint16_t& sequence) {
    return sendPatchInternal(
        idBody, true, true, true, target, &sequence);
}

Result DeviceRuntime::sendCurrentReliablePatch(
    ByteView keyedBody,
    const SendTarget& target,
    uint16_t& sequence) {
    return sendPatchInternal(
        keyedBody, false, true, false, target, &sequence);
}

Result DeviceRuntime::sendCurrentReliablePatchById(
    ByteView idBody,
    const SendTarget& target,
    uint16_t& sequence) {
    return sendPatchInternal(
        idBody, true, true, false, target, &sequence);
}

Result DeviceRuntime::sendEvent(
    ByteView keyedBody,
    const SendTarget& target) {
    Result result = bbp2::visitKeyedBody(
        keyedBody,
        &validateEventValue,
        &endpoints_);
    if (!result) return result;
    if (target.kind != SendTargetKind::Broadcast) {
        PeerSession* targetPeer = peer(target);
        const bool requireManifest = targetPeer != nullptr &&
                                     cloudTransport(targetPeer->transportId);
        if (targetPeer == nullptr || !targetPeer->helloComplete ||
            (requireManifest &&
             (((targetPeer->negotiatedFeatures &
                bbp2::FeatureEndpointIds) == 0U) ||
              !targetPeer->idReady))) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (!authorizedFor(
                *targetPeer,
                kAuthorizationPermissionObserve)) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        return sendBody(
            bbp2::MessageKind::Event,
            bbp2::FlagNone,
            nextSequence(),
            keyedBody,
            target);
    }
    if (!started_) return Result::failure(ErrorCode::NotConfigured);
    if (keyedBody.size > transmitBuffer_.size - bbp2::kBaseHeaderSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memmove(
        transmitBuffer_.data + bbp2::kBaseHeaderSize,
        keyedBody.data,
        keyedBody.size);
    ByteView frame;
    result = prepareEncodedBody(
        bbp2::MessageKind::Event,
        bbp2::FlagNone,
        nextSequence(),
        keyedBody.size,
        frame);
    return result
               ? fanOutPreparedFrame(
                     frame,
                     bbp2::FeatureNone,
                     nullptr)
               : result;
}

Result DeviceRuntime::sendEventById(
    ByteView idBody,
    const SendTarget& target) {
    if (target.kind == SendTargetKind::Broadcast) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }
    PeerSession* targetPeer = peer(target);
    if (targetPeer == nullptr || !targetPeer->helloComplete ||
        (targetPeer->negotiatedFeatures &
         bbp2::FeatureEndpointIds) == 0U || !targetPeer->idReady) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (!authorizedFor(
            *targetPeer,
            kAuthorizationPermissionObserve)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    Result result = bbp2::visitIdBody(
        idBody,
        &validateIdEventValue,
        &endpoints_);
    if (!result) return result;
    return sendBody(
        bbp2::MessageKind::Event,
        bbp2::FlagIdMode,
        nextSequence(),
        idBody,
        target);
}

Result DeviceRuntime::sendPatchInternal(
    ByteView values,
    bool idMode,
    bool reliable,
    bool advanceRevision,
    const SendTarget& target,
    uint16_t* sequence) {
    if (sequence != nullptr) *sequence = 0;
    if (reliable &&
        (!started_ || reliableOutbox_ == nullptr || sequence == nullptr)) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    if (target.kind == SendTargetKind::Broadcast) {
        return Result::failure(ErrorCode::UnsupportedFeature);
    }

    PeerSession* targetPeer = nullptr;
    if (target.kind != SendTargetKind::Broadcast) {
        targetPeer = peer(target);
        const bool requireManifest = targetPeer != nullptr &&
                                     cloudTransport(targetPeer->transportId);
        uint32_t requiredFeatures = bbp2::FeatureStateRevision;
        if (idMode || requireManifest) {
            requiredFeatures |= bbp2::FeatureEndpointIds;
        }
        if (reliable) requiredFeatures |= bbp2::FeatureReliableDelivery;
        if (targetPeer == nullptr || !targetPeer->helloComplete ||
            (targetPeer->negotiatedFeatures & requiredFeatures) !=
                requiredFeatures ||
            ((idMode || requireManifest) && !targetPeer->idReady) ||
            (reliable && targetPeer->remoteReliableReceiveWindow == 0U)) {
            return Result::failure(ErrorCode::NotConfigured);
        }
        if (!authorizedFor(
                *targetPeer,
                kAuthorizationPermissionObserve)) {
            return Result::failure(ErrorCode::AuthenticationRequired);
        }
        if (reliable && reliableOutbox_->pendingCount(target) >=
                            targetPeer->remoteReliableReceiveWindow) {
            return Result::failure(ErrorCode::WouldBlock);
        }
    }

    Result result = idMode
                        ? bbp2::visitIdBody(
                              values,
                              &validateIdPatchValue,
                              &endpoints_)
                        : bbp2::visitKeyedBody(
                              values,
                              &validatePatchValue,
                              &endpoints_);
    if (!result) return result;
    if (advanceRevision && stateRevision_ == UINT32_MAX) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }

    const uint32_t revision =
        advanceRevision ? stateRevision_ + 1U : stateRevision_;
    bbp2::StatePatchBody patch;
    patch.mode = bbp2::StatePatchMode::Report;
    patch.revision = revision;
    patch.values = values;
    ByteView encoded;
    result = bbp2::encodeStatePatchBody(
        patch,
        MutableByteSpan(
            transmitBuffer_.data + bbp2::kBaseHeaderSize,
            transmitBuffer_.size - bbp2::kBaseHeaderSize),
        encoded);
    if (!result) return result;
    // Revision describes the application state, not transport success. Once
    // a valid report is formed, a send failure must not make a later
    // StateRequest expose changed values under the old revision.
    if (advanceRevision) {
        stateRevision_ = revision;
        invalidateStateObservations();
    }

    const uint8_t flags = static_cast<uint8_t>(
        (idMode ? static_cast<uint8_t>(bbp2::FlagIdMode) : 0U) |
        (reliable ? static_cast<uint8_t>(bbp2::FlagAckRequired) : 0U));
    const uint16_t candidate = nextSequence();
    if (!reliable) {
        return sendEncodedBody(
            bbp2::MessageKind::Patch,
            flags,
            candidate,
            encoded.size,
            target);
    }
    if (encoded.size + bbp2::kBaseHeaderSize >
        maximumOutboundFrameSize(target)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView encodedFrame;
    result = prepareEncodedBody(
        bbp2::MessageKind::Patch,
        flags,
        candidate,
        encoded.size,
        encodedFrame);
    if (result) result = reliableOutbox_->enqueue(encodedFrame, target);
    if (result) *sequence = candidate;
    return result;
}

Result DeviceRuntime::sendBody(
    bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    ByteView body,
    const SendTarget& target) {
    if (body.size > transmitBuffer_.size - bbp2::kBaseHeaderSize ||
        body.size > UINT16_MAX) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (body.size != 0U && body.data == nullptr) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    memmove(
        transmitBuffer_.data + bbp2::kBaseHeaderSize,
        body.data,
        body.size);
    return sendEncodedBody(kind, flags, sequence, body.size, target);
}

Result DeviceRuntime::sendEncodedBody(
    bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    size_t bodySize,
    const SendTarget& target) {
    if (bodySize + bbp2::kBaseHeaderSize >
        maximumOutboundFrameSize(target)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    ByteView encodedFrame;
    Result result = prepareEncodedBody(
        kind,
        flags,
        sequence,
        bodySize,
        encodedFrame);
    if (!result) return result;
    return transports_.send(encodedFrame, target);
}

Result DeviceRuntime::prepareEncodedBody(
    bbp2::MessageKind kind,
    uint8_t flags,
    uint16_t sequence,
    size_t bodySize,
    ByteView& encodedFrame) {
    if (bodySize > UINT16_MAX ||
        bodySize + bbp2::kBaseHeaderSize > config_.maxFrameSize ||
        bodySize + bbp2::kBaseHeaderSize > transmitBuffer_.size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    bbp2::FrameHeader header;
    header.kind = static_cast<uint8_t>(kind);
    header.flags = flags;
    header.sequence = sequence;
    header.bodyLength = static_cast<uint16_t>(bodySize);
    Result result = bbp2::encodeHeader(header, transmitBuffer_);
    if (!result) return result;
    encodedFrame = ByteView(
        transmitBuffer_.data,
        bbp2::kBaseHeaderSize + bodySize);
    return Result::success();
}

Result DeviceRuntime::fanOutPreparedFrame(
    ByteView encodedFrame,
    uint32_t requiredFeatures,
    const RxContext* exclude) {
    if (!started_ || encodedFrame.empty() || encodedFrame.data == nullptr) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    Result firstError = Result::success();
    const bool carriesStateRevision =
        (requiredFeatures & bbp2::FeatureStateRevision) != 0U;
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        PeerSession& session = peers_[index];
        const bool manifestPending = cloudTransport(session.transportId) &&
                                     (((session.negotiatedFeatures &
                                        bbp2::FeatureEndpointIds) == 0U) ||
                                      !session.idReady);
        if (!session.occupied || !session.helloComplete ||
            !authorizedFor(session, kAuthorizationPermissionObserve) ||
            (session.negotiatedFeatures & requiredFeatures) !=
                requiredFeatures ||
            ((requiredFeatures & bbp2::FeatureEndpointIds) != 0U &&
             !session.idReady) ||
            manifestPending) {
            continue;
        }
        if (exclude != nullptr &&
            exclude->transportId == session.transportId &&
            exclude->sessionId == session.sessionId) {
            continue;
        }
        SendTarget target;
        target.kind = session.sessionId == 0U
                          ? SendTargetKind::Transport
                          : SendTargetKind::Session;
        target.transportId = session.transportId;
        target.sessionId = session.sessionId;
        if (encodedFrame.size > maximumOutboundFrameSize(target)) {
            if (carriesStateRevision) {
                session.stateObserved = false;
                session.snapshotCursor = 0U;
                session.snapshotNextCursor = 0U;
            }
            if (firstError) {
                firstError = Result::failure(ErrorCode::BufferTooSmall);
            }
            continue;
        }
        const Result result = transports_.send(encodedFrame, target);
        if (!result) {
            if (carriesStateRevision) {
                session.stateObserved = false;
                session.snapshotCursor = 0U;
                session.snapshotNextCursor = 0U;
            }
            if (firstError) firstError = result;
        }
    }
    return firstError;
}

Result DeviceRuntime::requestFingerprint(
    const bbp2::FrameView& frame,
    uint8_t output[kRequestFingerprintSize]) const {
    static const uint8_t domain[] = {
        'B', 'L', 'I', 'N', 'K', 'E', 'R', '-', 'R', 'E', 'Q', 'U', 'E',
        'S', 'T', '-', 'V', '1'
    };
    const uint8_t metadata[] = {
        frame.header.kind,
        static_cast<uint8_t>(
            frame.header.flags &
            static_cast<uint8_t>(
                bbp2::FlagAckRequired | bbp2::FlagIdMode)),
        static_cast<uint8_t>(frame.header.bodyLength >> 8U),
        static_cast<uint8_t>(frame.header.bodyLength & 0xFFU)
    };
    uint8_t digest[kSha256Size] = {};
    Sha256 hash;
    Result result = hash.update(ByteView(domain, sizeof(domain)));
    if (result) result = hash.update(ByteView(metadata, sizeof(metadata)));
    if (result) result = hash.update(frame.body);
    if (result) {
        result = hash.finish(MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) memcpy(output, digest, kRequestFingerprintSize);
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    return result;
}

Result DeviceRuntime::replayReliableRequest(
    const bbp2::FrameView& frame,
    const RxContext& rx,
    PeerSession& session,
    uint8_t fingerprint[kRequestFingerprintSize],
    bool& replayed) {
    replayed = false;
    if (frame.header.sequence == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    Result result = requestFingerprint(frame, fingerprint);
    if (!result) return result;
    for (uint8_t index = 0;
         index < config_.reliableReceiveWindow;
         ++index) {
        RequestReplayRecord& record = session.requestReplay[index];
        if (record.response == RequestReplayResponse::Empty ||
            record.sequence != frame.header.sequence) {
            continue;
        }
        if (memcmp(
                record.fingerprint,
                fingerprint,
                kRequestFingerprintSize) != 0) {
            incrementDiagnostic(sequenceConflictCount_);
            secureZero(MutableByteSpan(
                fingerprint, kRequestFingerprintSize));
            return Result::failure(ErrorCode::SequenceConflict);
        }
        incrementDiagnostic(duplicateRequestCount_);
        if (frame.header.kind ==
            static_cast<uint8_t>(bbp2::MessageKind::Command)) {
            incrementDiagnostic(duplicateCommandCount_);
        }
        secureZero(MutableByteSpan(
            fingerprint, kRequestFingerprintSize));
        if (record.response == RequestReplayResponse::Ack ||
            record.response == RequestReplayResponse::AckPending) {
            sendRequestAck(record, rx);
        } else {
            sendError(
                record.error,
                frame.header.sequence,
                rx,
                record.hasStateRevision,
                record.stateRevision);
        }
        replayed = true;
        return Result::success();
    }
    return Result::success();
}

DeviceRuntime::RequestReplayRecord& DeviceRuntime::rememberRequest(
    PeerSession& session,
    uint16_t sequence,
    const uint8_t fingerprint[kRequestFingerprintSize],
    RequestReplayResponse response,
    WireError error,
    bool hasStateRevision,
    uint32_t stateRevision) {
    const uint8_t index = static_cast<uint8_t>(
        session.nextRequestReplay % config_.reliableReceiveWindow);
    RequestReplayRecord& record = session.requestReplay[index];
    secureZero(MutableByteSpan(record.fingerprint, sizeof(record.fingerprint)));
    memcpy(record.fingerprint, fingerprint, sizeof(record.fingerprint));
    record.sequence = sequence;
    record.response = response;
    record.error = error;
    record.hasStateRevision = hasStateRevision;
    record.stateRevision = stateRevision;
    session.nextRequestReplay = static_cast<uint8_t>(
        (index + 1U) % config_.reliableReceiveWindow);
    return record;
}

void DeviceRuntime::sendRequestAck(
    RequestReplayRecord& record,
    const RxContext& rx) {
    const Result result = sendAck(
        record.sequence, rx, record.hasStateRevision, record.stateRevision);
    // Reuse the receive window, not another frame queue. Only backpressure is
    // locally retryable; permanent failures keep exact peer replay as fallback.
    record.response = result.code() == ErrorCode::WouldBlock
                          ? RequestReplayResponse::AckPending
                          : RequestReplayResponse::Ack;
}

void DeviceRuntime::pollRequestAcks(uint8_t transportId) {
    bool attempted = false;
    for (size_t peerIndex = 0U; peerIndex < BLINKER_MAX_PEER_SESSIONS; ++peerIndex) {
        PeerSession& session = peers_[peerIndex];
        if (!session.occupied || session.transportId != transportId) continue;
        const bool allowed = session.helloComplete &&
            authorizedFor(session, kAuthorizationPermissionControl);
        // Oldest retained result first; a window replacement also discards any
        // pending send, preserving the negotiated bounded replay contract.
        for (uint8_t offset = 0U; offset < config_.reliableReceiveWindow; ++offset) {
            const uint8_t index = static_cast<uint8_t>(
                (session.nextRequestReplay + offset) % config_.reliableReceiveWindow);
            RequestReplayRecord& record = session.requestReplay[index];
            if (record.response != RequestReplayResponse::AckPending) continue;
            if (!allowed) {
                record.response = RequestReplayResponse::Ack;
                continue;
            }
            if (attempted) continue;
            RxContext rx;
            rx.transportId = session.transportId;
            rx.sessionId = session.sessionId;
            rx.encrypted = session.encrypted;
            rx.bonded = session.bonded;
            rx.authenticated = session.transportAuthenticated;
            sendRequestAck(record, rx);
            attempted = true; // At most one send attempt per online transport/poll.
        }
    }
}

SendTarget DeviceRuntime::responseTarget(const RxContext& rx) const {
    SendTarget target;
    target.kind = rx.sessionId == 0U
                      ? SendTargetKind::Transport
                      : SendTargetKind::Session;
    target.transportId = rx.transportId;
    target.sessionId = rx.sessionId;
    return target;
}

DeviceRuntime::PeerSession* DeviceRuntime::peer(
    const RxContext& rx,
    bool create) {
    PeerSession* empty = nullptr;
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId) {
            return &session;
        }
        if (!session.occupied && empty == nullptr) {
            empty = &session;
        }
    }
    if (!create || empty == nullptr) return nullptr;
    empty->occupied = true;
    empty->transportId = rx.transportId;
    empty->sessionId = rx.sessionId;
    return empty;
}

DeviceRuntime::PeerSession* DeviceRuntime::peer(
    const SendTarget& target) {
    if (target.kind == SendTargetKind::Broadcast) return nullptr;
    const uint32_t sessionId =
        target.kind == SendTargetKind::Session ? target.sessionId : 0U;
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        PeerSession& session = peers_[index];
        if (session.occupied &&
            session.transportId == target.transportId &&
            session.sessionId == sessionId) {
            return &session;
        }
    }
    return nullptr;
}

bool DeviceRuntime::negotiated(const RxContext& rx) const {
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId) {
            return session.helloComplete;
        }
    }
    return false;
}

bool DeviceRuntime::authorizedFor(
    const RxContext& rx,
    uint32_t requiredPermissions) const {
    if (!localTransport(rx.transportId)) return rx.authenticated;
    if (rx.authenticated) {
        // Transport-owned session authorization is independent of payload
        // encryption (LAN Plain). It must still supply live, exact permissions.
        const IFrameTransport* transport = transports_.at(rx.transportId);
        return transport != nullptr && (rx.encrypted ||
            (transport->capabilities().features & TransportFeatureAuthenticated) != 0U) &&
            (transport->sessionPermissions(rx.sessionId) & requiredPermissions) == requiredPermissions;
    }
    if (!rx.encrypted) return false; // Existing in-band BLE authorization remains secure-only.
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId) {
            return (session.authorizationPermissions & requiredPermissions) ==
                   requiredPermissions;
        }
    }
    return false;
}

bool DeviceRuntime::authorizedFor(
    const PeerSession& session,
    uint32_t requiredPermissions) const {
    if (!localTransport(session.transportId)) {
        return session.transportAuthenticated;
    }
    const IFrameTransport* transport = transports_.at(session.transportId);
    if (session.transportAuthenticated) {
        return transport != nullptr && (session.encrypted ||
            (transport->capabilities().features & TransportFeatureAuthenticated) != 0U) &&
            (transport->sessionPermissions(session.sessionId) & requiredPermissions) == requiredPermissions;
    }
    if (!session.encrypted) return false;
    return (session.authorizationPermissions & requiredPermissions) == requiredPermissions;
}

bool DeviceRuntime::explicitlyAuthorizedLocal(
    const RxContext& rx,
    uint32_t requiredPermissions) const {
    if (!localTransport(rx.transportId) || !rx.encrypted) return false;
    for (size_t index = 0U; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId) {
            return (session.authorizationPermissions & requiredPermissions) ==
                   requiredPermissions;
        }
    }
    return false;
}

bool DeviceRuntime::idModeReady(const RxContext& rx) const {
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId && session.helloComplete) {
            return session.idReady;
        }
    }
    return false;
}

bool DeviceRuntime::localTransport(uint8_t transportId) const {
    const IFrameTransport* transport = transports_.at(transportId);
    return transport != nullptr &&
           (transport->capabilities().features & TransportFeatureLocal) != 0U;
}

bool DeviceRuntime::cloudTransport(uint8_t transportId) const {
    const IFrameTransport* transport = transports_.at(transportId);
    return transport != nullptr &&
           (transport->capabilities().features & TransportFeatureCloud) != 0U;
}

uint32_t DeviceRuntime::negotiatedFeatures(const RxContext& rx) const {
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (session.occupied && session.transportId == rx.transportId &&
            session.sessionId == rx.sessionId && session.helloComplete) {
            return session.negotiatedFeatures;
        }
    }
    return bbp2::FeatureNone;
}

size_t DeviceRuntime::maximumOutboundFrameSize(
    const SendTarget& target) const {
    size_t maximum = config_.maxFrameSize;
    if (maximum > transmitBuffer_.size) maximum = transmitBuffer_.size;
    if (target.kind == SendTargetKind::Broadcast) return maximum;
    const uint32_t sessionId =
        target.kind == SendTargetKind::Session ? target.sessionId : 0U;
    for (size_t index = 0; index < BLINKER_MAX_PEER_SESSIONS; ++index) {
        const PeerSession& session = peers_[index];
        if (!session.occupied || session.transportId != target.transportId ||
            session.sessionId != sessionId) {
            continue;
        }
        if (session.remoteMaxFrameSize != 0U &&
            maximum > session.remoteMaxFrameSize) {
            maximum = session.remoteMaxFrameSize;
        }
        break;
    }
    return maximum;
}

uint16_t DeviceRuntime::nextSequence() {
    uint16_t result = 0;
    do {
        result = nextSequence_;
        ++nextSequence_;
        if (nextSequence_ == 0U) ++nextSequence_;
    } while (reliableOutbox_ != nullptr &&
             reliableOutbox_->sequencePending(result));
    return result;
}

WireError DeviceRuntime::mapError(ErrorCode error) {
    switch (error) {
        case ErrorCode::AuthenticationRequired:
            return WireError::AuthenticationRequired;
        case ErrorCode::NotConfigured:
            return WireError::NegotiationRequired;
        case ErrorCode::UnsupportedFeature:
        case ErrorCode::UnsupportedVersion:
            return WireError::UnsupportedMessage;
        case ErrorCode::NotFound:
            return WireError::UnknownEndpoint;
        case ErrorCode::CapacityExceeded:
        case ErrorCode::BufferTooSmall:
            return WireError::ResourceExhausted;
        case ErrorCode::InternalError:
            return WireError::InternalError;
        case ErrorCode::SequenceConflict:
            return WireError::SequenceConflict;
        case ErrorCode::StateConflict:
            return WireError::StateConflict;
        case ErrorCode::ManifestConflict:
            return WireError::ManifestConflict;
        case ErrorCode::InvalidArgument:
        case ErrorCode::ValueOutOfRange:
            return WireError::CommandRejected;
        default:
            return WireError::MalformedMessage;
    }
}

} // namespace blinker
