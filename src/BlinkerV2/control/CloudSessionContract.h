#ifndef BLINKER_CONTROL_CLOUDSESSIONCONTRACT_H
#define BLINKER_CONTROL_CLOUDSESSIONCONTRACT_H

#include "../core/HmacSha256.h"
#include "../identity/CloudCredential.h"
#include "../protocol/cbor/Cbor.h"
#include "Session.h"

namespace blinker {

enum : size_t {
    kCloudSessionRequestIdSize = 16U,
    kCloudSessionIdSize = 16U,
    kCloudSessionProofSize = 32U,
    kCloudSessionMaxFirmwareVersionSize = 64U,
    kCloudSessionMaxHostSize = 96U,
    kCloudSessionMaxClientIdSize = 64U,
    kCloudSessionMaxUsernameSize = 64U,
    kCloudSessionMaxPasswordSize = 96U,
    kCloudSessionMaxTopicSize = 96U,
    kCloudSessionMaximumLifetimeSeconds = 604800U,
    kCloudSessionCredentialArenaSize = 518U
};

static const uint8_t kCloudSessionContractVersion = 2U;

enum class CloudSessionProofAlgorithm : uint8_t {
    HmacSha256 = 1
};

enum class CloudSessionRetryClass : uint8_t {
    Never = 0,
    SameRequest = 1,
    Backoff = 2,
    Reenroll = 3,
    NewRequest = 4
};

enum class CloudSessionWireError : uint16_t {
    Malformed = 2000,
    UnsupportedVersion = 2001,
    UnsupportedAlgorithm = 2002,
    AuthenticationRequired = 2003,
    CredentialNotFound = 2100,
    CredentialRevoked = 2101,
    CredentialVersionConflict = 2102,
    ProofInvalid = 2103,
    RequestConflict = 2104,
    DeviceRevoked = 2105,
    RequestExpired = 2106,
    RateLimited = 2500,
    TemporarilyUnavailable = 2501
};

struct CloudSessionRequest {
    ByteView requestId;
    StringView logicalDeviceId;
    uint32_t credentialVersion;
    uint32_t generation;
    uint8_t protocolVersion;
    StringView firmwareVersion;
    CloudSessionProofAlgorithm proofAlgorithm;
    ByteView proof;

    CloudSessionRequest()
        : credentialVersion(0U),
          generation(0U),
          protocolVersion(2U),
          proofAlgorithm(CloudSessionProofAlgorithm::HmacSha256) {}
};

struct CloudSessionResponse {
    ByteView requestId;
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

    CloudSessionResponse()
        : broker(BrokerKind::Unknown),
          port(0U),
          security(MqttSecurity::Tls),
          expiresInSeconds(0U),
          refreshAfterSeconds(0U) {}
};

struct CloudSessionErrorBody {
    CloudSessionWireError error;
    CloudSessionRetryClass retryClass;
    uint32_t retryAfterSeconds;
    bool hasRetryAfter;

    CloudSessionErrorBody()
        : error(CloudSessionWireError::Malformed),
          retryClass(CloudSessionRetryClass::Never),
          retryAfterSeconds(0U),
          hasRetryAfter(false) {}
};

Result encodeCloudSessionRequest(
    const CloudSessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudSessionRequest(
    ByteView encoded,
    CloudSessionRequest& body);
Result encodeCloudSessionRequestTranscript(
    const CloudSessionRequest& body,
    MutableByteSpan output,
    ByteView& encoded);
Result computeCloudSessionProof(
    ByteView cloudCredentialSecret,
    const CloudSessionRequest& body,
    MutableByteSpan transcriptBuffer,
    MutableByteSpan proofOutput);
Result verifyCloudSessionProof(
    ByteView cloudCredentialSecret,
    const CloudSessionRequest& body,
    MutableByteSpan transcriptBuffer);

Result encodeCloudSessionResponse(
    const CloudSessionResponse& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudSessionResponse(
    ByteView encoded,
    CloudSessionResponse& body);
Result copyCloudSessionCredentials(
    const CloudSessionResponse& response,
    MutableCharSpan credentialArena,
    SessionCredentials& credentials);

Result encodeCloudSessionErrorBody(
    const CloudSessionErrorBody& body,
    MutableByteSpan output,
    ByteView& encoded);
Result decodeCloudSessionErrorBody(
    ByteView encoded,
    CloudSessionErrorBody& body);
uint16_t cloudSessionHttpStatus(CloudSessionWireError error);
bool cloudSessionHttpStatusMatches(
    uint16_t statusCode,
    CloudSessionWireError error);

} // namespace blinker

#endif
