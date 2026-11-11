#include <BlinkerV2/security/ControllerHmacSha256Authorizer.h>

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

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

uint32_t number(const Properties& properties, const char* key) {
    const std::string& value = required(properties, key);
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    check(end != value.c_str() && *end == '\0' && parsed <= UINT32_MAX,
          std::string("invalid number: ") + key);
    return static_cast<uint32_t>(parsed);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: controller_auth_golden <fixture>");
    const Properties properties = loadProperties(argv[1]);
    const std::vector<uint8_t> secret = decodeHex(required(properties, "secret"));
    const std::vector<uint8_t> controllerId =
        decodeHex(required(properties, "controller_id"));
    const std::vector<uint8_t> clientNonce =
        decodeHex(required(properties, "client_nonce"));
    const std::vector<uint8_t> deviceNonce =
        decodeHex(required(properties, "device_nonce"));
    const blinker::ControllerCredentialDomain domain =
        static_cast<blinker::ControllerCredentialDomain>(
            number(properties, "domain"));
    const uint32_t generation = number(properties, "ownership_generation");
    const uint32_t version = number(properties, "credential_version");
    const uint32_t permissions = number(properties, "permissions");
    check(number(properties, "method") ==
              blinker::security::kControllerHmacSha256Method,
          "method mismatch");
    check(number(properties, "suite") ==
              static_cast<uint8_t>(
                  blinker::ControllerCredentialSuite::HmacSha256_32),
          "suite mismatch");

    uint8_t proof[blinker::security::kControllerAuthProofSize];
    blinker::Result result = blinker::security::computeControllerAuthProof(
        view(secret),
        view(controllerId),
        domain,
        generation,
        version,
        permissions,
        view(clientNonce),
        view(deviceNonce),
        blinker::security::ControllerAuthProofRole::Device,
        blinker::MutableByteSpan(proof, sizeof(proof)));
    check(result.ok(), "device proof failed");
    check(equal(
              blinker::ByteView(proof, sizeof(proof)),
              decodeHex(required(properties, "expected.device_proof"))),
          "device proof mismatch");

    uint8_t init[blinker::security::kControllerAuthInitPayloadSize];
    size_t offset = 0U;
    init[offset++] = blinker::security::ControllerAuthPayloadInit;
    std::memcpy(init + offset, &controllerId[0], controllerId.size());
    offset += controllerId.size();
    init[offset++] = static_cast<uint8_t>(domain);
    writeU32(init + offset, generation);
    offset += 4U;
    writeU32(init + offset, version);
    offset += 4U;
    std::memcpy(init + offset, &clientNonce[0], clientNonce.size());
    check(equal(
              blinker::ByteView(init, sizeof(init)),
              decodeHex(required(properties, "expected.init"))),
          "init mismatch");

    uint8_t challenge[blinker::security::kControllerAuthChallengePayloadSize];
    challenge[0] = blinker::security::ControllerAuthPayloadChallenge;
    std::memcpy(challenge + 1U, &deviceNonce[0], deviceNonce.size());
    writeU32(challenge + 1U + deviceNonce.size(), permissions);
    std::memcpy(
        challenge + 1U + deviceNonce.size() + 4U,
        proof,
        sizeof(proof));
    check(equal(
              blinker::ByteView(challenge, sizeof(challenge)),
              decodeHex(required(properties, "expected.challenge"))),
          "challenge mismatch");

    result = blinker::security::computeControllerAuthProof(
        view(secret),
        view(controllerId),
        domain,
        generation,
        version,
        permissions,
        view(clientNonce),
        view(deviceNonce),
        blinker::security::ControllerAuthProofRole::App,
        blinker::MutableByteSpan(proof, sizeof(proof)));
    check(result.ok(), "App proof failed");
    check(equal(
              blinker::ByteView(proof, sizeof(proof)),
              decodeHex(required(properties, "expected.app_proof"))),
          "App proof mismatch");
    uint8_t payload[blinker::security::kControllerAuthProofPayloadSize];
    payload[0] = blinker::security::ControllerAuthPayloadProof;
    std::memcpy(payload + 1U, proof, sizeof(proof));
    check(equal(
              blinker::ByteView(payload, sizeof(payload)),
              decodeHex(required(properties, "expected.proof_payload"))),
          "proof payload mismatch");
    std::cout << "C++ controller authorization golden passed: 5 exact values"
              << std::endl;
    return 0;
}
