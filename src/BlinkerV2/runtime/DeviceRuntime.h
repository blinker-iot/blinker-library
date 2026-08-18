#ifndef BLINKER_RUNTIME_DEVICERUNTIME_H
#define BLINKER_RUNTIME_DEVICERUNTIME_H

#include "../core/Sha256.h"
#include "../core/ResourceProfile.h"
#include "../interface/IAuthorizationProvider.h"
#include "../interface/IControllerControlEndpoint.h"
#include "../model/EndpointRegistry.h"
#include "../protocol/bbp2/Frame.h"
#include "../protocol/bbp2/KeyedBody.h"
#include "../protocol/bbp2/Messages.h"
#include "ReliableOutbox.h"
#include "../transport/TransportHub.h"

#if BLINKER_RELIABLE_RECEIVE_WINDOW < 1 || \
    BLINKER_RELIABLE_RECEIVE_WINDOW > 16
#error "BLINKER_RELIABLE_RECEIVE_WINDOW must be between 1 and 16"
#endif

namespace blinker {

enum class WireError : uint16_t {
    MalformedMessage = 1,
    AuthenticationRequired = 2,
    NegotiationRequired = 3,
    UnsupportedMessage = 4,
    UnknownEndpoint = 5,
    CommandRejected = 6,
    ResourceExhausted = 7,
    InternalError = 8,
    SequenceConflict = 9,
    StateConflict = 10,
    ManifestConflict = 11
};

typedef Result (*EndpointValueHandler)(
    void* context,
    const EndpointDescriptor& endpoint,
    ByteView encodedValue,
    cbor::Type valueType,
    const RxContext& rx);

typedef EndpointValueHandler CommandHandler;
typedef EndpointValueHandler StateApplyHandler;
typedef EndpointValueHandler EventHandler;

// Receives a fully decoded and prevalidated state transaction. Implementations
// must either commit every value or none. changed controls whether Runtime
// advances the authoritative state revision.
typedef Result (*StateApplyTransactionHandler)(
    void* context,
    ByteView values,
    bool idMode,
    const RxContext& rx,
    bool& changed);

// The callback begins the map and writes the current endpoint values. The
// runtime finishes and validates the keyed body before sending it.
typedef Result (*StateEncoder)(
    void* context,
    bbp2::KeyedBodyWriter& writer);

// Encodes one complete StatePage body directly into the bounded output. The
// encoder must cover [cursor,nextCursor) in Field Manifest order, may omit
// uninitialized fields from values, and must not mutate authoritative state.
typedef Result (*StatePageEncoder)(
    void* context,
    uint32_t revision,
    uint16_t cursor,
    bool idMode,
    MutableByteSpan output,
    ByteView& encoded,
    uint16_t& nextCursor,
    uint16_t& totalFields);

struct DeviceRuntimeConfig {
    uint32_t protocolFeatures;
    uint16_t maxFrameSize;
    uint32_t maxReassemblySize;
    uint32_t manifestRevision;
    uint32_t initialStateRevision;
    uint8_t maxAuthorizationAttempts;
    uint8_t reliableReceiveWindow;

    DeviceRuntimeConfig()
        : protocolFeatures(
              bbp2::FeatureManifest | bbp2::FeatureEndpointIds |
              bbp2::FeatureReliableDelivery |
              bbp2::FeatureStateRevision),
          maxFrameSize(512),
          maxReassemblySize(512),
          manifestRevision(0),
          initialStateRevision(0),
          maxAuthorizationAttempts(3),
          reliableReceiveWindow(BLINKER_RELIABLE_RECEIVE_WINDOW) {}
};

class DeviceRuntime {
public:
    DeviceRuntime(
        TransportHub& transports,
        EndpointRegistry& endpoints,
        MutableByteSpan transmitBuffer,
        const DeviceRuntimeConfig& config = DeviceRuntimeConfig());

    Result start();
    void stop();
    void poll(uint32_t totalBudgetMicros);
    void setCommandHandler(CommandHandler handler, void* context);
    void setStateApplyHandler(StateApplyHandler handler, void* context);
    void setStateApplyTransactionHandler(
        StateApplyTransactionHandler handler,
        void* context);
    void setEventHandler(EventHandler handler, void* context);
    void setStateEncoder(StateEncoder encoder, void* context);
    void setStatePageEncoder(StatePageEncoder encoder, void* context);
    Result setAuthorizationProvider(IAuthorizationProvider* provider);
    Result setControllerControlEndpoint(
        IControllerControlEndpoint* endpoint);
    // The outbox and every slot buffer must outlive Runtime and be configured
    // before start(). It is optional when the device only receives reliable
    // COMMANDs and never originates reliable PATCHes.
    Result setReliableOutbox(ReliableOutbox* outbox);

    Result sendPatch(ByteView keyedBody, const SendTarget& target);
    Result sendPatchById(ByteView idBody, const SendTarget& target);
    Result broadcastPatch(ByteView keyedBody);
    // For one logical state change sent to multiple peers: advance once,
    // then report the same current revision to each target.
    Result advanceStateRevision(uint32_t& revision);
    Result sendCurrentPatch(ByteView keyedBody, const SendTarget& target);
    Result sendCurrentPatchById(ByteView idBody, const SendTarget& target);
    // Sends one keyed report only to HELLO-complete, authorized peers that
    // negotiated state revisions. No eligible peers is a successful no-op.
    // An optional source is excluded when an accepted Apply is fanned out.
    Result fanOutCurrentPatch(
        ByteView keyedBody,
        const RxContext* exclude = nullptr);
    // Encodes the authoritative state through StateEncoder and fans out one
    // full keyed snapshot. This is used after ID-mode Apply so peer-specific
    // endpoint IDs never leak across transports/sessions.
    Result fanOutCurrentState(const RxContext* exclude = nullptr);
    // Success means accepted by the outbox, not acknowledged by the peer.
    // Observe the final outcome through ReliableDeliveryHandler.
    Result sendReliablePatch(
        ByteView keyedBody,
        const SendTarget& target,
        uint16_t& sequence);
    Result sendReliablePatchById(
        ByteView idBody,
        const SendTarget& target,
        uint16_t& sequence);
    Result sendCurrentReliablePatch(
        ByteView keyedBody,
        const SendTarget& target,
        uint16_t& sequence);
    Result sendCurrentReliablePatchById(
        ByteView idBody,
        const SendTarget& target,
        uint16_t& sequence);

    Result sendEvent(ByteView keyedBody, const SendTarget& target);
    Result sendEventById(ByteView idBody, const SendTarget& target);

    uint32_t stateRevision() const { return stateRevision_; }

    uint32_t receivedFrameCount() const { return receivedFrameCount_; }
    uint32_t rejectedFrameCount() const { return rejectedFrameCount_; }
    uint32_t duplicateCommandCount() const { return duplicateCommandCount_; }
    uint32_t duplicateRequestCount() const { return duplicateRequestCount_; }
    uint32_t sequenceConflictCount() const { return sequenceConflictCount_; }

private:
    static const uint8_t kRequestFingerprintSize = 16;

    enum class RequestReplayResponse : uint8_t {
        Empty = 0,
        Ack,
        Error
    };

    struct RequestReplayRecord {
        uint8_t fingerprint[kRequestFingerprintSize];
        uint16_t sequence;
        uint32_t stateRevision;
        WireError error;
        RequestReplayResponse response;
        bool hasStateRevision;

        RequestReplayRecord()
            : fingerprint(),
              sequence(0),
              stateRevision(0),
              error(WireError::MalformedMessage),
              response(RequestReplayResponse::Empty),
              hasStateRevision(false) {}
    };

    struct PeerSession {
        uint8_t transportId;
        uint32_t sessionId;
        uint32_t negotiatedFeatures;
        uint32_t authorizationPermissions;
        uint16_t remoteMaxFrameSize;
        uint16_t authorizationMethod;
        uint8_t authorizationAttempts;
        uint8_t remoteReliableReceiveWindow;
        uint8_t nextRequestReplay;
        bool helloComplete;
        bool helloSent;
        bool authorizationLocked;
        bool encrypted;
        bool bonded;
        bool transportAuthenticated;
        bool stateObserved;
        uint16_t snapshotCursor;
        uint16_t snapshotNextCursor;
        bool snapshotIdMode;
        // True means this session proved knowledge of the Runtime's current
        // revision+fingerprint. The 32-byte digest is not duplicated per
        // peer, keeping the session table compact.
        bool idReady;
        bool occupied;
        RequestReplayRecord requestReplay[BLINKER_RELIABLE_RECEIVE_WINDOW];

        PeerSession()
            : transportId(0),
              sessionId(0),
              negotiatedFeatures(bbp2::FeatureNone),
              authorizationPermissions(0U),
              remoteMaxFrameSize(0),
              authorizationMethod(0),
              authorizationAttempts(0),
              remoteReliableReceiveWindow(0),
              nextRequestReplay(0),
              helloComplete(false),
              helloSent(false),
              authorizationLocked(false),
              encrypted(false),
              bonded(false),
              transportAuthenticated(false),
              stateObserved(false),
              snapshotCursor(0),
              snapshotNextCursor(0),
              snapshotIdMode(false),
              idReady(false),
              occupied(false),
              requestReplay() {}
    };

    struct EndpointVisitContext {
        DeviceRuntime* runtime;
        const RxContext* rx;
        EndpointValueHandler handler;
        void* handlerContext;
        uint8_t requiredAccess;

        EndpointVisitContext()
            : runtime(nullptr),
              rx(nullptr),
              handler(nullptr),
              handlerContext(nullptr),
              requiredAccess(AccessNone) {}
    };

    static void receiveThunk(
        void* context,
        ByteView frame,
        const RxContext& rx);
    static void connectedThunk(void* context, const RxContext& rx);
    static void disconnectedThunk(void* context, const RxContext& rx);
    static Result endpointVisitThunk(
        void* context,
        StringView endpointKey,
        ByteView encodedValue,
        cbor::Type valueType);
    static Result endpointIdVisitThunk(
        void* context,
        uint16_t endpointId,
        ByteView encodedValue,
        cbor::Type valueType);

    void receive(ByteView frame, const RxContext& rx);
    void sessionConnected(const RxContext& rx);
    void sessionDisconnected(const RxContext& rx);
    Result handleHello(const bbp2::FrameView& frame, const RxContext& rx);
    Result handleCommand(const bbp2::FrameView& frame, const RxContext& rx);
    Result handlePatch(const bbp2::FrameView& frame, const RxContext& rx);
    Result handleEvent(const bbp2::FrameView& frame, const RxContext& rx);
    Result handleAck(const bbp2::FrameView& frame, const RxContext& rx);
    Result handleError(const bbp2::FrameView& frame, const RxContext& rx);
    Result handleAuthenticate(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result handleManifestRequest(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result handleManifestAccept(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result handleStateRequest(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result handleControllerControlOpen(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result handleControllerMutation(
        const bbp2::FrameView& frame,
        const RxContext& rx);
    Result visitEndpointValue(
        StringView endpointKey,
        ByteView encodedValue,
        cbor::Type valueType,
        const EndpointVisitContext& visit);
    Result visitEndpointValue(
        const EndpointDescriptor& endpoint,
        ByteView encodedValue,
        cbor::Type valueType,
        const EndpointVisitContext& visit);
    Result sendHello(const SendTarget& target, uint8_t flags, uint16_t sequence);
    Result sendAck(
        uint16_t acknowledgedSequence,
        const RxContext& rx,
        bool hasStateRevision = false,
        uint32_t stateRevision = 0);
    Result sendError(
        WireError error,
        uint16_t relatedSequence,
        const RxContext& rx,
        bool hasStateRevision = false,
        uint32_t stateRevision = 0);
    Result sendAuthResult(
        uint16_t sequence,
        const bbp2::AuthResultBody& body,
        const RxContext& rx);
    Result sendBody(
        bbp2::MessageKind kind,
        uint8_t flags,
        uint16_t sequence,
        ByteView body,
        const SendTarget& target);
    Result sendPatchInternal(
        ByteView values,
        bool idMode,
        bool reliable,
        bool advanceRevision,
        const SendTarget& target,
        uint16_t* sequence);
    Result sendEncodedBody(
        bbp2::MessageKind kind,
        uint8_t flags,
        uint16_t sequence,
        size_t bodySize,
        const SendTarget& target);
    Result prepareEncodedBody(
        bbp2::MessageKind kind,
        uint8_t flags,
        uint16_t sequence,
        size_t bodySize,
        ByteView& encodedFrame);
    Result fanOutPreparedFrame(
        ByteView encodedFrame,
        uint32_t requiredFeatures,
        const RxContext* exclude);
    Result requestFingerprint(
        const bbp2::FrameView& frame,
        uint8_t output[kRequestFingerprintSize]) const;
    Result replayReliableRequest(
        const bbp2::FrameView& frame,
        const RxContext& rx,
        PeerSession& session,
        uint8_t fingerprint[kRequestFingerprintSize],
        bool& replayed);
    void rememberRequest(
        PeerSession& session,
        uint16_t sequence,
        const uint8_t fingerprint[kRequestFingerprintSize],
        RequestReplayResponse response,
        WireError error,
        bool hasStateRevision = false,
        uint32_t stateRevision = 0);
    SendTarget responseTarget(const RxContext& rx) const;
    PeerSession* peer(const RxContext& rx, bool create);
    PeerSession* peer(const SendTarget& target);
    bool negotiated(const RxContext& rx) const;
    bool authorizedFor(
        const RxContext& rx,
        uint32_t requiredPermissions) const;
    bool authorizedFor(
        const PeerSession& session,
        uint32_t requiredPermissions) const;
    bool explicitlyAuthorizedLocal(
        const RxContext& rx,
        uint32_t requiredPermissions) const;
    bool idModeReady(const RxContext& rx) const;
    bool localTransport(uint8_t transportId) const;
    uint32_t negotiatedFeatures(const RxContext& rx) const;
    void invalidateStateObservations(PeerSession* observingPeer = nullptr);
    size_t maximumOutboundFrameSize(const SendTarget& target) const;
    uint16_t nextSequence();
    static WireError mapError(ErrorCode error);

    TransportHub& transports_;
    EndpointRegistry& endpoints_;
    MutableByteSpan transmitBuffer_;
    DeviceRuntimeConfig config_;
    CommandHandler commandHandler_;
    void* commandContext_;
    StateApplyHandler stateApplyHandler_;
    void* stateApplyContext_;
    StateApplyTransactionHandler stateApplyTransactionHandler_;
    void* stateApplyTransactionContext_;
    EventHandler eventHandler_;
    void* eventContext_;
    StateEncoder stateEncoder_;
    StatePageEncoder statePageEncoder_;
    void* stateContext_;
    void* statePageContext_;
    IAuthorizationProvider* authorizationProvider_;
    IControllerControlEndpoint* controllerControlEndpoint_;
    ReliableOutbox* reliableOutbox_;
    PeerSession peers_[BLINKER_MAX_PEER_SESSIONS];
    bool helloSent_[BLINKER_MAX_TRANSPORTS];
    uint16_t nextSequence_;
    uint32_t receivedFrameCount_;
    uint32_t rejectedFrameCount_;
    uint32_t duplicateCommandCount_;
    uint32_t duplicateRequestCount_;
    uint32_t sequenceConflictCount_;
    uint32_t stateRevision_;
    uint8_t manifestFingerprint_[kSha256Size];
    bool hasManifestFingerprint_;
    bool started_;
};

} // namespace blinker

#endif
