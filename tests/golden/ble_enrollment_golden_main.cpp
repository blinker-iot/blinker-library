#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/arduino/config/PortableServerKeys.h>
#include <BlinkerV2/provisioning/BleEnrollmentGrant.h>

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

Properties load(const char* path) {
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

const std::string& required(const Properties& values, const std::string& key) {
    Properties::const_iterator found = values.find(key);
    if (found == values.end()) fail(std::string("missing property: ") + key);
    return found->second;
}

uint8_t nibble(char value) {
    if (value >= '0' && value <= '9') return static_cast<uint8_t>(value - '0');
    if (value >= 'a' && value <= 'f') {
        return static_cast<uint8_t>(value - 'a' + 10);
    }
    fail("invalid lowercase hex");
    return 0U;
}

std::vector<uint8_t> hex(const std::string& value) {
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
    return value.empty() ? blinker::ByteView()
                         : blinker::ByteView(&value[0], value.size());
}

uint64_t number(const Properties& values, const char* key) {
    const std::string& text = required(values, key);
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    check(end != text.c_str() && *end == '\0', std::string("invalid number: ") + key);
    return static_cast<uint64_t>(parsed);
}

void expect(const Properties& values,
            const std::string& key,
            blinker::ByteView actual) {
    const std::vector<uint8_t> expected = hex(required(values, key));
    check(actual.size == expected.size() &&
              (actual.empty() || std::memcmp(actual.data, &expected[0], actual.size) == 0),
          std::string("mismatch: ") + key);
}

void runAlgorithm(const Properties& values,
                  const char* prefix,
                  blinker::ServerSignatureAlgorithm algorithm,
                  const std::vector<uint8_t>& grantId,
                  const std::vector<uint8_t>& deviceId,
                  const std::vector<uint8_t>& setupId,
                  const std::vector<uint8_t>& setupHash,
                  const std::vector<uint8_t>& controllerId,
                  const std::vector<uint8_t>& secretDigest,
                  const std::vector<uint8_t>& presenceDigest,
                  const std::vector<uint8_t>& nonce) {
    const std::vector<uint8_t> signature =
        hex(required(values, std::string(prefix) + "_signature"));
    blinker::BleEnrollmentGrant grant;
    grant.grantId = view(grantId);
    grant.deviceInstanceId = view(deviceId);
    grant.setupSessionId = view(setupId);
    grant.setupTranscriptHash = view(setupHash);
    grant.accessEpoch = static_cast<uint32_t>(number(values, "access_epoch"));
    grant.controllerId = view(controllerId);
    grant.controllerSecretDigest = view(secretDigest);
    grant.controllerPermissions = static_cast<uint32_t>(number(values, "controller_permissions"));
    grant.presenceKeyVersion =
        static_cast<uint32_t>(number(values, "presence_key_version"));
    grant.presenceKeyDigest = view(presenceDigest);
    grant.nonce = view(nonce);
    grant.issuedAt = number(values, "issued_at");
    grant.expiresAt = number(values, "expires_at");
    grant.securityProfile = static_cast<blinker::BleEnrollmentSecurityProfile>(
        number(values, "security_profile"));
    grant.serverKeyId = static_cast<uint32_t>(number(values, "server_key_id"));
    grant.signatureAlgorithm = algorithm;
    grant.signature = view(signature);

    uint8_t workspace[blinker::kBleEnrollmentGrantWorkspaceSize] = {};
    blinker::ByteView encoded;
    check(blinker::encodeBleEnrollmentGrantTranscript(
              grant, blinker::MutableByteSpan(workspace, sizeof(workspace)), encoded).ok(),
          "transcript encode failed");
    expect(values, std::string(prefix) + "_transcript", encoded);
    uint8_t digest[blinker::kSha256Size] = {};
    check(blinker::sha256(encoded, blinker::MutableByteSpan(digest, sizeof(digest))).ok(),
          "transcript digest failed");
    expect(values, std::string(prefix) + "_digest",
           blinker::ByteView(digest, sizeof(digest)));

    uint8_t grantStorage[blinker::kBleEnrollmentGrantMaxEncodedSize] = {};
    check(blinker::encodeBleEnrollmentGrant(
              grant, blinker::MutableByteSpan(grantStorage, sizeof(grantStorage)), encoded).ok(),
          "grant encode failed");
    expect(values, std::string(prefix) + "_grant", encoded);
    blinker::BleEnrollmentGrant decoded;
    check(blinker::decodeBleEnrollmentGrant(encoded, decoded).ok(), "grant decode failed");
    check(decoded.signatureAlgorithm == algorithm &&
              decoded.accessEpoch == grant.accessEpoch &&
              decoded.securityProfile == grant.securityProfile &&
              decoded.serverKeyId == grant.serverKeyId,
          "decoded grant metadata mismatch");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: ble_enrollment_golden <fixture>");
    const Properties values = load(argv[1]);
    check(blinker::official::serverSigningKeysCount == 1U,
          "portable Ed25519 key ring size mismatch");
    check(blinker::official::serverSigningKeys[0].keyId ==
              number(values, "server_key_id"),
          "portable Ed25519 key id mismatch");
    expect(values, "ed25519_public_key", blinker::ByteView(
        blinker::official::serverSigningKeyBytes,
        sizeof(blinker::official::serverSigningKeyBytes)));
    const std::vector<uint8_t> grantId = hex(required(values, "grant_id"));
    const std::vector<uint8_t> deviceId = hex(required(values, "device_instance_id"));
    const std::vector<uint8_t> setupId = hex(required(values, "setup_session_id"));
    const std::vector<uint8_t> setupHash = hex(required(values, "setup_transcript_hash"));
    const std::vector<uint8_t> controllerId = hex(required(values, "controller_id"));
    const std::vector<uint8_t> secretDigest = hex(required(values, "controller_secret_digest"));
    const std::vector<uint8_t> presenceDigest = hex(required(values, "presence_key_digest"));
    const std::vector<uint8_t> nonce = hex(required(values, "nonce"));
    runAlgorithm(values, "ed25519", blinker::ServerSignatureAlgorithm::Ed25519,
                 grantId, deviceId, setupId, setupHash, controllerId, secretDigest,
                 presenceDigest, nonce);
    runAlgorithm(values, "p256", blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
                 grantId, deviceId, setupId, setupHash, controllerId, secretDigest,
                 presenceDigest, nonce);
    std::cout << "BLE enrollment golden vectors passed" << std::endl;
    return 0;
}
