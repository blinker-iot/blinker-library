#include <BlinkerV2/control/DeviceKeySessionContract.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

typedef std::map<std::string, std::string> Properties;

void fail(const std::string& message) {
    std::cerr << message << std::endl;
    std::exit(1);
}

void check(bool condition, const std::string& message) {
    if (!condition) fail(message);
}

Properties loadProperties(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) fail(std::string("cannot open fixture: ") + path);
    Properties output;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line[line.size() - 1U] == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t split = line.find('=');
        if (split == std::string::npos || split == 0U ||
            output.find(line.substr(0U, split)) != output.end()) {
            fail(std::string("invalid property: ") + line);
        }
        output[line.substr(0U, split)] = line.substr(split + 1U);
    }
    return output;
}

const std::string& required(
    const Properties& properties,
    const char* key) {
    Properties::const_iterator found = properties.find(key);
    if (found == properties.end()) fail(std::string("missing property: ") + key);
    return found->second;
}

uint8_t nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<uint8_t>(value - 'a' + 10);
    }
    fail("invalid lowercase hex");
    return 0U;
}

std::vector<uint8_t> decodeHex(const std::string& value) {
    check((value.size() & 1U) == 0U, "odd hex length");
    std::vector<uint8_t> output(value.size() / 2U);
    for (size_t index = 0U; index < output.size(); ++index) {
        output[index] = static_cast<uint8_t>(
            (nibble(value[index * 2U]) << 4U) |
            nibble(value[index * 2U + 1U]));
    }
    return output;
}

blinker::ByteView view(const std::vector<uint8_t>& value) {
    return value.empty()
               ? blinker::ByteView()
               : blinker::ByteView(&value[0], value.size());
}

bool equal(blinker::ByteView value, const std::vector<uint8_t>& expected) {
    return value.size == expected.size() &&
           (value.empty() ||
            std::memcmp(value.data, &expected[0], value.size) == 0);
}

uint32_t number(const Properties& properties, const char* key) {
    const std::string& value = required(properties, key);
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    check(end != value.c_str() && *end == '\0' && parsed <= UINT32_MAX,
          std::string("invalid number: ") + key);
    return static_cast<uint32_t>(parsed);
}

blinker::StringView text(
    const Properties& properties,
    const char* key) {
    const std::string& value = required(properties, key);
    return blinker::StringView(value.data(), value.size());
}

void checkEncoded(
    blinker::Result result,
    blinker::ByteView encoded,
    const Properties& properties,
    const char* key) {
    check(result.ok(), std::string("encode failed: ") + key);
    check(equal(encoded, decodeHex(required(properties, key))),
          std::string("exact mismatch: ") + key);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: device_key_session_golden <fixture>");
    const Properties properties = loadProperties(argv[1]);
    const std::vector<uint8_t> raw = decodeHex(required(properties, "raw_key"));
    check(raw.size() == blinker::kDeviceKeySize, "raw DeviceKey size");

    blinker::DeviceKey key;
    std::memcpy(key.bytes, &raw[0], raw.size());
    char encodedKey[blinker::kDeviceKeyTextSize];
    blinker::StringView keyText;
    blinker::Result result = blinker::encodeDeviceKey(
        key,
        blinker::MutableCharSpan(encodedKey, sizeof(encodedKey)),
        keyText);
    check(result.ok() &&
              keyText.size == required(properties, "encoded_key").size() &&
              std::memcmp(
                  keyText.data,
                  required(properties, "encoded_key").data(),
                  keyText.size) == 0,
          "encoded DeviceKey mismatch");
    blinker::DeviceKey decodedKey;
    result = blinker::decodeDeviceKey(keyText, decodedKey);
    check(result.ok() && blinker::sameDeviceKey(key, decodedKey),
          "decoded DeviceKey mismatch");
    const char* invalidKeys[] = {
        "invalid.too_short",
        "invalid.prefixed",
        "invalid.padding",
        "invalid.standard_alphabet",
        "invalid.noncanonical_tail",
        "invalid.all_zero",
        "invalid.trailing_space"};
    for (size_t index = 0U;
         index < sizeof(invalidKeys) / sizeof(invalidKeys[0]);
         ++index) {
        const std::string& invalid = required(properties, invalidKeys[index]);
        result = blinker::decodeDeviceKey(
            blinker::StringView(invalid.data(), invalid.size()), decodedKey);
        check(!result.ok(),
              std::string("invalid DeviceKey accepted: ") + invalidKeys[index]);
    }

    blinker::DeviceKeyLocator locator;
    result = blinker::deriveDeviceKeyLocator(key, locator);
    check(result.ok() && equal(
              blinker::ByteView(locator.bytes, sizeof(locator.bytes)),
              decodeHex(required(properties, "expected.locator"))),
          "locator mismatch");
    uint8_t authKey[blinker::kDeviceAuthKeySize];
    result = blinker::deriveDeviceAuthKey(
        key, blinker::MutableByteSpan(authKey, sizeof(authKey)));
    check(result.ok() && equal(
              blinker::ByteView(authKey, sizeof(authKey)),
              decodeHex(required(properties, "expected.auth_key"))),
          "auth key mismatch");

    const std::vector<uint8_t> requestId =
        decodeHex(required(properties, "request_id"));
    const std::vector<uint8_t> instance =
        decodeHex(required(properties, "device_instance_id"));
    const std::vector<uint8_t> clientNonce =
        decodeHex(required(properties, "client_nonce"));
    const std::vector<uint8_t> challengeId =
        decodeHex(required(properties, "challenge_id"));
    const std::vector<uint8_t> serverNonce =
        decodeHex(required(properties, "server_nonce"));
    uint8_t buffer[1024U];
    blinker::ByteView encoded;

    blinker::DeviceKeyChallengeRequest challengeRequest;
    challengeRequest.requestId = view(requestId);
    challengeRequest.locator = blinker::ByteView(
        locator.bytes, sizeof(locator.bytes));
    challengeRequest.deviceInstanceId = view(instance);
    challengeRequest.clientNonce = view(clientNonce);
    result = blinker::encodeDeviceKeyChallengeRequest(
        challengeRequest,
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        encoded);
    checkEncoded(
        result, encoded, properties, "expected.challenge_request");
    blinker::DeviceKeyChallengeRequest decodedChallengeRequest;
    check(blinker::decodeDeviceKeyChallengeRequest(
              encoded, decodedChallengeRequest).ok(),
          "challenge request decode failed");

    blinker::DeviceKeyChallengeResponse challengeResponse;
    challengeResponse.requestId = view(requestId);
    challengeResponse.challengeId = view(challengeId);
    challengeResponse.serverNonce = view(serverNonce);
    challengeResponse.credentialVersion =
        number(properties, "credential_version");
    challengeResponse.expiresInSeconds =
        number(properties, "challenge_expires");
    result = blinker::encodeDeviceKeyChallengeResponse(
        challengeResponse,
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        encoded);
    checkEncoded(
        result, encoded, properties, "expected.challenge_response");
    blinker::DeviceKeyChallengeResponse decodedChallengeResponse;
    check(blinker::decodeDeviceKeyChallengeResponse(
              encoded, decodedChallengeResponse).ok(),
          "challenge response decode failed");

    blinker::DeviceKeySessionProofContext context;
    context.requestId = view(requestId);
    context.locator = blinker::ByteView(locator.bytes, sizeof(locator.bytes));
    context.deviceInstanceId = view(instance);
    context.clientNonce = view(clientNonce);
    context.challengeId = view(challengeId);
    context.serverNonce = view(serverNonce);
    context.credentialVersion = number(properties, "credential_version");
    context.protocolVersion = static_cast<uint8_t>(
        number(properties, "protocol_version"));
    context.firmwareVersion = text(properties, "firmware_version");
    context.requestedSecurity = static_cast<blinker::MqttSecurity>(
        number(properties, "requested_security"));
    context.proofAlgorithm = static_cast<blinker::DeviceKeyProofAlgorithm>(
        number(properties, "proof_algorithm"));
    result = blinker::encodeDeviceKeySessionProofTranscript(
        context,
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        encoded);
    checkEncoded(
        result, encoded, properties, "expected.proof_transcript");

    uint8_t proof[blinker::kDeviceKeyProofSize];
    result = blinker::computeDeviceKeySessionProof(
        key, context, blinker::MutableByteSpan(proof, sizeof(proof)));
    check(result.ok() && equal(
              blinker::ByteView(proof, sizeof(proof)),
              decodeHex(required(properties, "expected.proof"))),
          "proof mismatch");

    blinker::DeviceKeySessionRequest sessionRequest;
    sessionRequest.requestId = view(requestId);
    sessionRequest.challengeId = view(challengeId);
    sessionRequest.protocolVersion = context.protocolVersion;
    sessionRequest.firmwareVersion = context.firmwareVersion;
    sessionRequest.requestedSecurity = context.requestedSecurity;
    sessionRequest.proofAlgorithm = context.proofAlgorithm;
    sessionRequest.proof = blinker::ByteView(proof, sizeof(proof));
    result = blinker::encodeDeviceKeySessionRequest(
        sessionRequest,
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        encoded);
    checkEncoded(result, encoded, properties, "expected.session_request");
    blinker::DeviceKeySessionRequest decodedSessionRequest;
    check(blinker::decodeDeviceKeySessionRequest(
              encoded, decodedSessionRequest).ok(),
          "session request decode failed");

    const std::vector<uint8_t> sessionId =
        decodeHex(required(properties, "session_id"));
    blinker::DeviceKeySessionResponse sessionResponse;
    sessionResponse.requestId = view(requestId);
    sessionResponse.credentialVersion = context.credentialVersion;
    sessionResponse.broker = static_cast<blinker::BrokerKind>(
        number(properties, "broker"));
    sessionResponse.host = text(properties, "host");
    sessionResponse.port = static_cast<uint16_t>(number(properties, "port"));
    sessionResponse.security = context.requestedSecurity;
    sessionResponse.clientId = text(properties, "client_id");
    sessionResponse.username = text(properties, "username");
    sessionResponse.password = text(properties, "password");
    sessionResponse.publishTopic = text(properties, "publish_topic");
    sessionResponse.subscribeTopic = text(properties, "subscribe_topic");
    sessionResponse.sessionId = view(sessionId);
    sessionResponse.expiresInSeconds = number(properties, "session_expires");
    sessionResponse.refreshAfterSeconds =
        number(properties, "session_refresh_after");
    result = blinker::encodeDeviceKeySessionResponse(
        sessionResponse,
        blinker::MutableByteSpan(buffer, sizeof(buffer)),
        encoded);
    checkEncoded(result, encoded, properties, "expected.session_response");
    blinker::DeviceKeySessionResponse decodedSessionResponse;
    check(blinker::decodeDeviceKeySessionResponse(
              encoded, decodedSessionResponse).ok(),
          "session response decode failed");

    blinker::clearDeviceKey(decodedKey);
    blinker::clearDeviceKey(key);
    std::memset(authKey, 0, sizeof(authKey));
    std::memset(proof, 0, sizeof(proof));
    std::cout << "C++ DeviceKey session golden passed: 9 exact values"
              << std::endl;
    return 0;
}
