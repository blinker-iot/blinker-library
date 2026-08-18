#ifndef BLINKER_CONTROL_DEVICEKEYSESSIONCONTRACT_H
#define BLINKER_CONTROL_DEVICEKEYSESSIONCONTRACT_H

#include "../identity/DeviceKey.h"
#include "../identity/DeviceInstanceId.h"
#include "../protocol/cbor/Cbor.h"
#include "Session.h"

namespace blinker {

enum : size_t {
    kDeviceKeyRequestIdSize = 16U,
    kDeviceKeyChallengeIdSize = 16U,
    kDeviceKeyNonceSize = 16U,
    kDeviceKeyProofSize = 32U,
    kDeviceKeySessionIdSize = 16U,
    kDeviceKeyMaximumFirmwareVersionSize = 64U,
    kDeviceKeyMaximumHostSize = 96U,
    kDeviceKeyMaximumClientIdSize = 64U,
    kDeviceKeyMaximumUsernameSize = 64U,
    kDeviceKeyMaximumPasswordSize = 96U,
    kDeviceKeyMaximumTopicSize = 96U,
    kDeviceKeyMaximumChallengeLifetimeSeconds = 300U,
    kDeviceKeyMaximumSessionLifetimeSeconds = 604800U,
    kDeviceKeySessionCredentialArenaSize = 518U,
    kDeviceKeyChallengeRequestMaxEncodedSize = 75U,
    kDeviceKeyChallengeResponseMaxEncodedSize = 69U,
    kDeviceKeySessionRequestMaxEncodedSize = 147U,
    kDeviceKeyProofTranscriptMaxEncodedSize = 215U,
    kDeviceKeySessionResponseMaxEncodedSize = 595U
};

static const uint8_t kDeviceKeySessionContractVersion = 1U;

enum class DeviceKeyProofAlgorithm : uint8_t {
    HmacSha256 = 1U
};

enum class DeviceKeySessionRetryClass : uint8_t {
    Never = 0U,
    SameRequest = 1U,
    Backoff = 2U,
    NewChallenge = 3U,
    AuthorizationRequired = 4U
};

enum class DeviceKeySessionWireError : uint16_t {
    Malformed = 3000U,
    UnsupportedVersion = 3001U,
    UnsupportedAlgorithm = 3002U,
    DeviceKeyInvalid = 3100U,
    DeviceKeyRotated = 3101U,
    CredentialRevoked = 3102U,
    ChallengeExpired = 3103U,
    ChallengeConflict = 3104U,
    SessionRevoked = 3105U,
    DeviceRevoked = 3106U,
    RateLimited = 3500U,
    TemporarilyUnavailable = 3501U
};

struct DeviceKeyChallengeRequest {
    ByteView requestId;
    ByteView locator;
    ByteView deviceInstanceId;
    ByteView clientNonce;
};

struct DeviceKeyChallengeResponse {
    ByteView requestId;
    ByteView challengeId;
    ByteView serverNonce;
    uint32_t credentialVersion;
    uint32_t expiresInSeconds;

    DeviceKeyChallengeResponse()
        : credentialVersion(0U), expiresInSeconds(0U) {}
};

struct DeviceKeySessionRequest {
    ByteView requestId;
    ByteView challengeId;
    uint8_t protocolVersion;
    StringView firmwareVersion;
    MqttSecurity requestedSecurity;
    DeviceKeyProofAlgorithm proofAlgorithm;
    ByteView proof;

    DeviceKeySessionRequest()
        : protocolVersion(2U),
          requestedSecurity(MqttSecurity::PlainTcp),
          proofAlgorithm(DeviceKeyProofAlgorithm::HmacSha256) {}
};

// Exact HMAC input. The server reconstructs locator, instance and both nonces
// from its single-use challenge record, then combines them with the session
// request fields. No raw DeviceKey is part of this structure.
struct DeviceKeySessionProofContext {
    ByteView requestId;
    ByteView locator;
    ByteView deviceInstanceId;
    ByteView clientNonce;
    ByteView challengeId;
    ByteView serverNonce;
    uint32_t credentialVersion;
    uint8_t protocolVersion;
    StringView firmwareVersion;
    MqttSecurity requestedSecurity;
    DeviceKeyProofAlgorithm proofAlgorithm;

    DeviceKeySessionProofContext()
        : credentialVersion(0U),
          protocolVersion(2U),
          requestedSecurity(MqttSecurity::PlainTcp),
          proofAlgorithm(DeviceKeyProofAlgorithm::HmacSha256) {}
};

struct DeviceKeySessionResponse {
    ByteView requestId;
    uint32_t credentialVersion;
    BrokerKind broker;
    StringView host;
    uint16_t port;
    MqttSecurity security;
    StringView clientId;
    StringView username;
    StringView password;
    StringView publishTopic;
    StringView subscribeTopic;
    ByteView sessionId;
    uint32_t expiresInSeconds;
    uint32_t refreshAfterSeconds;

    DeviceKeySessionResponse()
        : credentialVersion(0U),
          broker(BrokerKind::Unknown),
          port(0U),
          security(MqttSecurity::PlainTcp),
          expiresInSeconds(0U),
          refreshAfterSeconds(0U) {}
};

struct DeviceKeySessionErrorBody {
    DeviceKeySessionWireError error;
    DeviceKeySessionRetryClass retryClass;
    uint32_t retryAfterSeconds;
    bool hasRetryAfter;

    DeviceKeySessionErrorBody()
        : error(DeviceKeySessionWireError::Malformed),
          retryClass(DeviceKeySessionRetryClass::Never),
          retryAfterSeconds(0U),
          hasRetryAfter(false) {}
};

StringView deviceKeyChallengeTarget();
StringView deviceKeySessionTarget();

Result encodeDeviceKeyChallengeRequest(
    const DeviceKeyChallengeRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeDeviceKeyChallengeRequest(
    ByteView encoded,
    DeviceKeyChallengeRequest& body);
Result encodeDeviceKeyChallengeResponse(
    const DeviceKeyChallengeResponse& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeDeviceKeyChallengeResponse(
    ByteView encoded,
    DeviceKeyChallengeResponse& body);

Result encodeDeviceKeySessionRequest(
    const DeviceKeySessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeDeviceKeySessionRequest(
    ByteView encoded,
    DeviceKeySessionRequest& body);
Result encodeDeviceKeySessionProofTranscript(
    const DeviceKeySessionProofContext& context,
    MutableByteSpan output,
    ByteView& encoded);
Result computeDeviceKeySessionProof(
    const DeviceKey& key,
    const DeviceKeySessionProofContext& context,
    MutableByteSpan proofOutput);
Result verifyDeviceKeySessionProof(
    const DeviceKey& key,
    const DeviceKeySessionProofContext& context,
    ByteView proof);

Result encodeDeviceKeySessionResponse(
    const DeviceKeySessionResponse& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeDeviceKeySessionResponse(
    ByteView encoded,
    DeviceKeySessionResponse& body);
Result copyDeviceKeySessionCredentials(
    const DeviceKeySessionResponse& response,
    MutableCharSpan credentialArena,
    SessionCredentials& credentials);

Result encodeDeviceKeySessionErrorBody(
    const DeviceKeySessionErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeDeviceKeySessionErrorBody(
    ByteView encoded,
    DeviceKeySessionErrorBody& body);
uint16_t deviceKeySessionHttpStatus(DeviceKeySessionWireError error);
bool deviceKeySessionHttpStatusMatches(
    uint16_t statusCode,
    DeviceKeySessionWireError error);

} // namespace blinker

#endif
