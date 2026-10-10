#include <BlinkerV2/control/DeviceKeySessionContract.h>
#include <BlinkerV2/identity/ConfiguredDeviceKeySource.h>
#include <BlinkerV2/runtime/DeviceKeySessionArena.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

blinker::ByteView bytes(const uint8_t* value, size_t size) {
    return blinker::ByteView(value, size);
}

void fillSequence(uint8_t* output, size_t size, uint8_t first) {
    for (size_t index = 0U; index < size; ++index) {
        output[index] = static_cast<uint8_t>(first + index);
    }
}

void testEncodingAndDerivation() {
    static const char expected[] =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
    blinker::DeviceKey key;
    fillSequence(key.bytes, sizeof(key.bytes), 0U);

    char text[blinker::kDeviceKeyTextSize];
    blinker::StringView encoded;
    blinker::Result result = blinker::encodeDeviceKey(
        key, blinker::MutableCharSpan(text, sizeof(text)), encoded);
    check(result.ok(), "DeviceKey encode failed");
    check(encoded.size == sizeof(expected) - 1U, "DeviceKey text size");
    check(std::memcmp(encoded.data, expected, encoded.size) == 0,
          "DeviceKey text mismatch");

    blinker::DeviceKey decoded;
    result = blinker::decodeDeviceKey(encoded, decoded);
    check(result.ok() && blinker::sameDeviceKey(key, decoded),
          "DeviceKey roundtrip failed");

    char nonCanonical[sizeof(expected) - 1U];
    std::memcpy(nonCanonical, expected, sizeof(nonCanonical));
    nonCanonical[sizeof(nonCanonical) - 1U] = '9';
    result = blinker::decodeDeviceKey(
        blinker::StringView(nonCanonical, sizeof(nonCanonical)), decoded);
    check(!result.ok(), "non-canonical Base64URL accepted");
    result = blinker::decodeDeviceKey(
        blinker::StringView("bek_AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8"),
        decoded);
    check(!result.ok(), "prefixed DeviceKey accepted");
    result = blinker::decodeDeviceKey(
        blinker::StringView(
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"),
        decoded);
    check(!result.ok(), "all-zero DeviceKey accepted");

    blinker::DeviceKeyLocator locator;
    result = blinker::deriveDeviceKeyLocator(key, locator);
    check(result.ok(), "locator derivation failed");
    uint8_t authKey[blinker::kDeviceAuthKeySize];
    result = blinker::deriveDeviceAuthKey(
        key, blinker::MutableByteSpan(authKey, sizeof(authKey)));
    check(result.ok(), "auth-key derivation failed");
    check(std::memcmp(locator.bytes, authKey, sizeof(locator.bytes)) != 0,
          "domain-separated derivations collided");
    std::memset(authKey, 0, sizeof(authKey));
    blinker::clearDeviceKey(decoded);
    blinker::clearDeviceKey(key);
}

void testConfiguredSourceAndArena() {
    static const char encoded[] =
        "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8";
    blinker::ConfiguredDeviceKeySource source;
    blinker::DeviceKey loaded;
    check(!source.configured(), "configured source starts populated");
    check(source.load(loaded).code() == blinker::ErrorCode::NotFound,
          "empty configured source did not report NotFound");
    check(source.configure(
              blinker::StringView(encoded, sizeof(encoded) - 1U)).ok(),
          "configured source rejected valid DeviceKey");
    check(source.configured() && source.load(loaded).ok(),
          "configured source did not load DeviceKey");
    check(source.protection() == blinker::StorageProtection::PlainFlash,
          "configured source overstated storage protection");

    blinker::DeviceKey expected;
    fillSequence(expected.bytes, sizeof(expected.bytes), 0U);
    check(blinker::sameDeviceKey(expected, loaded),
          "configured source changed DeviceKey bytes");
    check(!source.configure(blinker::StringView("invalid", 7U)).ok(),
          "configured source accepted invalid DeviceKey");
    check(source.load(loaded).ok() &&
              blinker::sameDeviceKey(expected, loaded),
          "failed atomic reconfiguration replaced valid DeviceKey");
    source.clear();
    check(!source.configured() &&
              source.load(loaded).code() == blinker::ErrorCode::NotFound,
          "configured source clear failed");

    blinker::DeviceKeySessionArena arena;
    blinker::MutableByteSpan request = arena.requestBuffer();
    blinker::MutableByteSpan response = arena.responseBuffer();
    blinker::MutableCharSpan credentials = arena.credentialArena();
    check(request.size ==
              blinker::kDeviceKeySessionRequestMaxEncodedSize &&
              response.size ==
                  blinker::kDeviceKeySessionResponseMaxEncodedSize &&
              credentials.size ==
                  blinker::kDeviceKeySessionCredentialArenaSize,
          "DeviceKey arena span sizes drifted");
    std::memset(request.data, 0x11, request.size);
    std::memset(response.data, 0x22, response.size);
    std::memset(credentials.data, 0x33, credentials.size);
    arena.clear();
    for (size_t index = 0U; index < request.size; ++index) {
        check(request.data[index] == 0U, "request arena was not wiped");
    }
    for (size_t index = 0U; index < response.size; ++index) {
        check(response.data[index] == 0U, "response arena was not wiped");
    }
    for (size_t index = 0U; index < credentials.size; ++index) {
        check(credentials.data[index] == 0, "credential arena was not wiped");
    }
    blinker::clearDeviceKey(expected);
    blinker::clearDeviceKey(loaded);
}

void testContract() {
    uint8_t requestId[blinker::kDeviceKeyRequestIdSize];
    uint8_t locator[blinker::kDeviceKeyLocatorSize];
    uint8_t instance[16U];
    uint8_t clientNonce[blinker::kDeviceKeyNonceSize];
    uint8_t challengeId[blinker::kDeviceKeyChallengeIdSize];
    uint8_t serverNonce[blinker::kDeviceKeyNonceSize];
    fillSequence(requestId, sizeof(requestId), 0x10U);
    fillSequence(locator, sizeof(locator), 0x20U);
    fillSequence(instance, sizeof(instance), 0x30U);
    fillSequence(clientNonce, sizeof(clientNonce), 0x40U);
    fillSequence(challengeId, sizeof(challengeId), 0x50U);
    fillSequence(serverNonce, sizeof(serverNonce), 0x60U);

    blinker::DeviceKeyChallengeRequest challengeRequest;
    challengeRequest.requestId = bytes(requestId, sizeof(requestId));
    challengeRequest.locator = bytes(locator, sizeof(locator));
    challengeRequest.deviceInstanceId = bytes(instance, sizeof(instance));
    challengeRequest.clientNonce = bytes(clientNonce, sizeof(clientNonce));
    uint8_t encodedBuffer[512U];
    blinker::ByteView encoded;
    blinker::Result result = blinker::encodeDeviceKeyChallengeRequest(
        challengeRequest,
        blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
        encoded);
    check(result.ok() &&
              encoded.size <=
                  blinker::kDeviceKeyChallengeRequestMaxEncodedSize,
          "challenge request encode failed");
    blinker::DeviceKeyChallengeRequest decodedChallengeRequest;
    result = blinker::decodeDeviceKeyChallengeRequest(
        encoded, decodedChallengeRequest);
    check(result.ok() && decodedChallengeRequest.locator.size == 16U,
          "challenge request decode failed");

    blinker::DeviceKeyChallengeResponse challengeResponse;
    challengeResponse.requestId = bytes(requestId, sizeof(requestId));
    challengeResponse.challengeId = bytes(challengeId, sizeof(challengeId));
    challengeResponse.serverNonce = bytes(serverNonce, sizeof(serverNonce));
    challengeResponse.credentialVersion = 7U;
    challengeResponse.expiresInSeconds = 120U;
    result = blinker::encodeDeviceKeyChallengeResponse(
        challengeResponse,
        blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
        encoded);
    check(result.ok() &&
              encoded.size <=
                  blinker::kDeviceKeyChallengeResponseMaxEncodedSize,
          "challenge response encode failed");
    blinker::DeviceKeyChallengeResponse decodedChallengeResponse;
    check(blinker::decodeDeviceKeyChallengeResponse(
              encoded, decodedChallengeResponse).ok(),
          "challenge response decode failed");

    blinker::DeviceKeySessionProofContext context;
    context.requestId = bytes(requestId, sizeof(requestId));
    context.locator = bytes(locator, sizeof(locator));
    context.deviceInstanceId = bytes(instance, sizeof(instance));
    context.clientNonce = bytes(clientNonce, sizeof(clientNonce));
    context.challengeId = bytes(challengeId, sizeof(challengeId));
    context.serverNonce = bytes(serverNonce, sizeof(serverNonce));
    context.credentialVersion = 7U;
    context.protocolVersion = 2U;
    context.firmwareVersion = blinker::StringView("2.0.0-test");
    context.requestedSecurity = blinker::MqttSecurity::PlainTcp;
    context.proofAlgorithm = blinker::DeviceKeyProofAlgorithm::HmacSha256;

    blinker::DeviceKey key;
    fillSequence(key.bytes, sizeof(key.bytes), 0U);
    uint8_t proof[blinker::kDeviceKeyProofSize];
    result = blinker::computeDeviceKeySessionProof(
        key, context, blinker::MutableByteSpan(proof, sizeof(proof)));
    check(result.ok(), "session proof failed");
    check(blinker::verifyDeviceKeySessionProof(
              key, context, bytes(proof, sizeof(proof))).ok(),
          "session proof verify failed");
    proof[0] ^= 0x01U;
    check(blinker::verifyDeviceKeySessionProof(
              key, context, bytes(proof, sizeof(proof))).code() ==
              blinker::ErrorCode::AuthenticationRequired,
          "mutated session proof accepted");
    proof[0] ^= 0x01U;

    blinker::DeviceKeySessionRequest sessionRequest;
    sessionRequest.requestId = bytes(requestId, sizeof(requestId));
    sessionRequest.challengeId = bytes(challengeId, sizeof(challengeId));
    sessionRequest.protocolVersion = context.protocolVersion;
    sessionRequest.firmwareVersion = context.firmwareVersion;
    sessionRequest.requestedSecurity = context.requestedSecurity;
    sessionRequest.proofAlgorithm = context.proofAlgorithm;
    sessionRequest.proof = bytes(proof, sizeof(proof));
    result = blinker::encodeDeviceKeySessionRequest(
        sessionRequest,
        blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
        encoded);
    check(result.ok() &&
              encoded.size <= blinker::kDeviceKeySessionRequestMaxEncodedSize,
          "session request encode failed");
    blinker::DeviceKeySessionRequest decodedSessionRequest;
    check(blinker::decodeDeviceKeySessionRequest(
              encoded, decodedSessionRequest).ok(),
          "session request decode failed");

    uint8_t transcript[blinker::kDeviceKeyProofTranscriptMaxEncodedSize];
    result = blinker::encodeDeviceKeySessionProofTranscript(
        context,
        blinker::MutableByteSpan(transcript, sizeof(transcript)),
        encoded);
    check(result.ok() && encoded.size <= sizeof(transcript),
          "proof transcript encode failed");
    blinker::clearDeviceKey(key);
}

void testErrors() {
    blinker::DeviceKeySessionErrorBody body;
    body.error = blinker::DeviceKeySessionWireError::RateLimited;
    body.retryClass = blinker::DeviceKeySessionRetryClass::Backoff;
    body.retryAfterSeconds = 30U;
    body.hasRetryAfter = true;
    uint8_t buffer[64U];
    blinker::ByteView encoded;
    check(blinker::encodeDeviceKeySessionErrorBody(
              body,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "DeviceKey error encode failed");
    blinker::DeviceKeySessionErrorBody decoded;
    check(blinker::decodeDeviceKeySessionErrorBody(encoded, decoded).ok(),
          "DeviceKey error decode failed");
    check(blinker::deviceKeySessionHttpStatus(decoded.error) == 429U &&
              blinker::deviceKeySessionHttpStatusMatches(429U, decoded.error),
          "DeviceKey error HTTP mapping failed");
}

void testMaximumSessionResponseSize() {
    uint8_t requestId[blinker::kDeviceKeyRequestIdSize];
    uint8_t sessionId[blinker::kDeviceKeySessionIdSize];
    char host[blinker::kDeviceKeyMaximumHostSize];
    char clientId[blinker::kDeviceKeyMaximumClientIdSize];
    char username[blinker::kDeviceKeyMaximumUsernameSize];
    char password[blinker::kDeviceKeyMaximumPasswordSize];
    char publishTopic[blinker::kDeviceKeyMaximumTopicSize];
    char subscribeTopic[blinker::kDeviceKeyMaximumTopicSize];
    fillSequence(requestId, sizeof(requestId), 1U);
    fillSequence(sessionId, sizeof(sessionId), 2U);
    std::memset(host, 'h', sizeof(host));
    std::memset(clientId, 'c', sizeof(clientId));
    std::memset(username, 'u', sizeof(username));
    std::memset(password, 'p', sizeof(password));
    std::memset(publishTopic, 'a', sizeof(publishTopic));
    std::memset(subscribeTopic, 'b', sizeof(subscribeTopic));

    blinker::DeviceKeySessionResponse response;
    response.requestId = bytes(requestId, sizeof(requestId));
    response.credentialVersion = UINT32_MAX;
    response.broker = blinker::BrokerKind::Blinker;
    response.host = blinker::StringView(host, sizeof(host));
    response.port = UINT16_MAX;
    response.security = blinker::MqttSecurity::Tls;
    response.clientId = blinker::StringView(clientId, sizeof(clientId));
    response.username = blinker::StringView(username, sizeof(username));
    response.password = blinker::StringView(password, sizeof(password));
    response.publishTopic =
        blinker::StringView(publishTopic, sizeof(publishTopic));
    response.subscribeTopic =
        blinker::StringView(subscribeTopic, sizeof(subscribeTopic));
    response.sessionId = bytes(sessionId, sizeof(sessionId));
    response.expiresInSeconds =
        blinker::kDeviceKeyMaximumSessionLifetimeSeconds;
    response.refreshAfterSeconds = response.expiresInSeconds - 1U;

    uint8_t encodedBuffer[
        blinker::kDeviceKeySessionResponseMaxEncodedSize];
    blinker::ByteView encoded;
    check(blinker::encodeDeviceKeySessionResponse(
              response,
              blinker::MutableByteSpan(encodedBuffer, sizeof(encodedBuffer)),
              encoded).ok(),
          "maximum session response encode failed");
    check(encoded.size == sizeof(encodedBuffer),
          "session response maximum size drift");
    blinker::DeviceKeySessionResponse decoded;
    check(blinker::decodeDeviceKeySessionResponse(encoded, decoded).ok(),
          "maximum session response decode failed");
}

} // namespace

int main() {
    testEncodingAndDerivation();
    testConfiguredSourceAndArena();
    testContract();
    testErrors();
    testMaximumSessionResponseSize();
    std::cout << "DeviceKey core/session tests passed" << std::endl;
    return 0;
}
