#ifndef BLINKER_PROTOCOL_BBP2_MESSAGES_H
#define BLINKER_PROTOCOL_BBP2_MESSAGES_H

#include "../../core/Result.h"
#include "../../core/Span.h"
#include "Frame.h"
#include "../cbor/Cbor.h"

namespace blinker {
namespace bbp2 {

static const uint8_t kMaxHelloVersions = 4;
static const uint8_t kMaxAuthorizationMethods = 4;
static const uint8_t kMaxReliableReceiveWindow = 16;
static const uint8_t kManifestFingerprintSize = 32;
static const uint8_t kRouteIdentitySize = 16;
// Maximum bytes before the nested values map in a StatePatchBody:
// map(3), keys 0..2, mode and a canonical uint32 revision.
static const uint8_t kStatePatchEnvelopeReserve = 10;
static const uint8_t kTelemetryDataEnvelopeReserve = 26;
static const uint8_t kMaxTelemetryFields = 32;

enum class PeerRole : uint8_t {
    Device = 0,
    App = 1,
    Server = 2
};

enum ProtocolFeature : uint32_t {
    FeatureNone = 0,
    FeatureManifest = 1UL << 0,
    FeatureEndpointIds = 1UL << 1,
    FeatureFragmentation = 1UL << 2,
    FeatureCrc32c = 1UL << 3,
    FeatureHistory = 1UL << 4,
    FeatureAuthentication = 1UL << 5,
    FeatureReliableDelivery = 1UL << 6,
    FeatureStateRevision = 1UL << 7,
    // bit 8 is reserved for the removed pre-freeze WidgetCatalog experiment.
    FeatureControllerControl = 1UL << 9,
    FeatureRouting = 1UL << 10,
    FeatureTelemetry = 1UL << 11,
    // Account App/Broker capability; never enabled by the device runtime.
    FeaturePresence = 1UL << 12
};

enum class RoutePeerKind : uint8_t {
    LogicalDevice = 0,
    CloudService = 1,
    DeviceGroup = 2,
    Account = 3,
    Platform = 4
};

enum : size_t {
    kControllerControlNonceSize = 16U,
    kControllerCredentialSecretSize = 32U,
    kControllerGrantMaximumSize = 193U,
    kControllerMutationReceiptMaximumSize = 145U,
    kControllerControlOpenMaxEncodedSize = 2U,
    kControllerControlChallengeMaxEncodedSize = 19U,
    kControllerMutationMaxEncodedSize = 231U,
    kControllerMutationMaximumFrameSize =
        kBaseHeaderSize + kControllerMutationMaxEncodedSize,
    kControllerMutationReceiptMaximumFrameSize =
        kBaseHeaderSize + kControllerMutationReceiptMaximumSize
};

static const uint8_t kControllerControlWireVersion = 1U;

struct HelloBody {
    PeerRole role;
    uint8_t versions[kMaxHelloVersions];
    uint8_t versionCount;
    uint32_t features;
    uint16_t maxFrameSize;
    uint32_t maxReassemblySize;
    uint32_t manifestRevision;
    ByteView manifestFingerprint;
    uint16_t authorizationMethods[kMaxAuthorizationMethods];
    uint8_t authorizationMethodCount;
    uint8_t reliableReceiveWindow;
    bool hasManifestRevision;
    bool hasManifestFingerprint;

    HelloBody()
        : role(PeerRole::Device),
          versions(),
          versionCount(0),
          features(FeatureNone),
          maxFrameSize(0),
          maxReassemblySize(0),
          manifestRevision(0),
          manifestFingerprint(),
          authorizationMethods(),
          authorizationMethodCount(0),
          reliableReceiveWindow(0),
          hasManifestRevision(false),
          hasManifestFingerprint(false) {}
};

struct AckBody {
    uint16_t acknowledgedSequence;
    uint32_t stateRevision;
    bool hasStateRevision;

    AckBody()
        : acknowledgedSequence(0),
          stateRevision(0),
          hasStateRevision(false) {}
};

struct ManifestRequestBody {
    uint16_t cursor;

    ManifestRequestBody() : cursor(0) {}
};

struct ManifestAcceptBody {
    uint32_t revision;
    ByteView fingerprint;

    ManifestAcceptBody() : revision(0), fingerprint() {}
};

// State snapshots are explicitly paged and are not PATCH messages: consumers
// must stage every page and publish one authoritative snapshot only after the
// complete cursor range has been received at one revision.
struct StateRequestBody {
    uint16_t cursor;
    uint32_t observedRevision;
    bool hasObservedRevision;

    StateRequestBody()
        : cursor(0),
          observedRevision(0),
          hasObservedRevision(false) {}
};

struct StatePageBody {
    uint32_t revision;
    uint16_t cursor;
    uint16_t nextCursor;
    uint16_t totalFields;
    ByteView values;

    StatePageBody()
        : revision(0),
          cursor(0),
          nextCursor(0),
          totalFields(0),
          values() {}
};

struct ErrorBody {
    uint16_t errorCode;
    uint16_t relatedSequence;
    uint32_t stateRevision;
    StringView detail;
    bool hasRelatedSequence;
    bool hasDetail;
    bool hasStateRevision;

    ErrorBody()
        : errorCode(0),
          relatedSequence(0),
          stateRevision(0),
          detail(),
          hasRelatedSequence(false),
          hasDetail(false),
          hasStateRevision(false) {}
};

// PATCH has explicit direction semantics. Report carries authoritative state
// at revision; Apply asks the authoritative device to apply values only when
// its current revision equals revision.
enum class StatePatchMode : uint8_t {
    Report = 0,
    Apply = 1
};

struct StatePatchBody {
    StatePatchMode mode;
    uint32_t revision;
    ByteView values;

    StatePatchBody()
        : mode(StatePatchMode::Report), revision(0), values() {}
};

enum class TelemetryControlOperation : uint8_t {
    Open = 0,
    Renew = 1,
    Close = 2
};

enum class TelemetryStatusCode : uint8_t {
    Opened = 0,
    Renewed = 1,
    Closed = 2,
    Expired = 3
};

// Control maps are operation-specific:
// open  {0:op,1:streamId,3:leaseMs,4:intervalMs,5:[fieldId...]}
// renew {0:op,1:streamId,2:epoch,3:leaseMs}
// close {0:op,1:streamId,2:epoch}
// Field IDs are strictly increasing Manifest IDs. No key/string mode exists.
struct TelemetryControlBody {
    TelemetryControlOperation operation;
    uint32_t streamId;
    uint32_t streamEpoch;
    uint32_t leaseMillis;
    uint32_t intervalMillis;
    uint16_t fieldIds[kMaxTelemetryFields];
    uint8_t fieldCount;

    TelemetryControlBody()
        : operation(TelemetryControlOperation::Open),
          streamId(0U),
          streamEpoch(0U),
          leaseMillis(0U),
          intervalMillis(0U),
          fieldIds(),
          fieldCount(0U) {}
};

// Status always has five fields so consumers can replace local lease state
// without operation-specific branches. leaseMillis is zero after close/expiry.
struct TelemetryStatusBody {
    uint32_t streamId;
    uint32_t streamEpoch;
    TelemetryStatusCode status;
    uint32_t effectiveIntervalMillis;
    uint32_t leaseMillis;

    TelemetryStatusBody()
        : streamId(0U),
          streamEpoch(0U),
          status(TelemetryStatusCode::Opened),
          effectiveIntervalMillis(0U),
          leaseMillis(0U) {}
};

// One best-effort sample per frame in v1. values is one canonical ID-keyed
// map and the enclosing frame must carry FlagIdMode.
struct TelemetryDataBody {
    uint32_t streamId;
    uint32_t streamEpoch;
    uint32_t sampleSequence;
    uint32_t monotonicMillis;
    ByteView values;

    TelemetryDataBody()
        : streamId(0U),
          streamEpoch(0U),
          sampleSequence(0U),
          monotonicMillis(0U),
          values() {}
};

// Route uses peer as the target and requires requestId. Delivery uses peer as
// the Broker-authenticated source; unsolicited state/event delivery may omit
// requestId. messageBody is one complete canonical CBOR value and is never
// converted to JSON by the wire codec.
struct RoutedMessageBody {
    RoutePeerKind peerKind;
    ByteView peerId;
    ByteView requestId;
    uint8_t messageKind;
    uint8_t messageFlags;
    ByteView messageBody;
    bool hasRequestId;

    RoutedMessageBody()
        : peerKind(RoutePeerKind::LogicalDevice),
          peerId(),
          requestId(),
          messageKind(static_cast<uint8_t>(MessageKind::Patch)),
          messageFlags(FlagNone),
          messageBody(),
          hasRequestId(false) {}
};

enum class AuthStatus : uint8_t {
    Continue = 0,
    Authorized = 1,
    Rejected = 2,
    Locked = 3
};

struct AuthRequestBody {
    uint16_t method;
    ByteView payload;

    AuthRequestBody() : method(0), payload() {}
};

struct AuthResultBody {
    uint16_t method;
    AuthStatus status;
    ByteView payload;
    bool hasPayload;

    AuthResultBody()
        : method(0),
          status(AuthStatus::Continue),
          payload(),
          hasPayload(false) {}
};

struct ControllerControlChallengeBody {
    ByteView controlNonce;
};

// The signed grant and raw secret share one bounded frame. Revoke carries an
// empty secret. The grant signs SHA-256(secret), so a modified raw secret is
// rejected by the controller coordinator.
struct ControllerMutationBody {
    ByteView grant;
    ByteView controllerSecret;
};

Result encodeHelloBody(
    const HelloBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeHelloBody(
    ByteView encoded,
    HelloBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeManifestRequestBody(
    const ManifestRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeManifestRequestBody(
    ByteView encoded,
    ManifestRequestBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeManifestAcceptBody(
    const ManifestAcceptBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeManifestAcceptBody(
    ByteView encoded,
    ManifestAcceptBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeStateRequestBody(
    const StateRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeStateRequestBody(
    ByteView encoded,
    StateRequestBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeStatePageBody(
    const StatePageBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeStatePageBody(
    ByteView encoded,
    StatePageBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeAckBody(
    const AckBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeAckBody(
    ByteView encoded,
    AckBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeErrorBody(
    const ErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeErrorBody(
    ByteView encoded,
    ErrorBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeStatePatchBody(
    const StatePatchBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeStatePatchBody(
    ByteView encoded,
    StatePatchBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeTelemetryControlBody(
    const TelemetryControlBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeTelemetryControlBody(
    ByteView encoded,
    TelemetryControlBody& body,
    const cbor::Limits& limits = cbor::Limits());
Result encodeTelemetryStatusBody(
    const TelemetryStatusBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeTelemetryStatusBody(
    ByteView encoded,
    TelemetryStatusBody& body,
    const cbor::Limits& limits = cbor::Limits());
Result encodeTelemetryDataBody(
    const TelemetryDataBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeTelemetryDataBody(
    ByteView encoded,
    TelemetryDataBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeRouteBody(
    const RoutedMessageBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeRouteBody(
    ByteView encoded,
    RoutedMessageBody& body,
    const cbor::Limits& limits = cbor::Limits());
Result encodeDeliveryBody(
    const RoutedMessageBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeDeliveryBody(
    ByteView encoded,
    RoutedMessageBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeAuthRequestBody(
    const AuthRequestBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeAuthRequestBody(
    ByteView encoded,
    AuthRequestBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeAuthResultBody(
    const AuthResultBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeAuthResultBody(
    ByteView encoded,
    AuthResultBody& body,
    const cbor::Limits& limits = cbor::Limits());

Result encodeControllerControlOpenBody(
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeControllerControlOpenBody(
    ByteView encoded,
    const cbor::Limits& limits = cbor::Limits());
Result encodeControllerControlChallengeBody(
    const ControllerControlChallengeBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeControllerControlChallengeBody(
    ByteView encoded,
    ControllerControlChallengeBody& body,
    const cbor::Limits& limits = cbor::Limits());
Result encodeControllerMutationBody(
    const ControllerMutationBody& body,
    MutableByteSpan output,
    ByteView& encoded,
    const cbor::Limits& limits = cbor::Limits());
Result decodeControllerMutationBody(
    ByteView encoded,
    ControllerMutationBody& body,
    const cbor::Limits& limits = cbor::Limits());

} // namespace bbp2
} // namespace blinker

#endif
