#include <BlinkerV2/core/Sha256.h>
#include <BlinkerV2/provisioning/ControllerControlContract.h>

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

const std::string& required(const Properties& properties, const char* key) {
    Properties::const_iterator found = properties.find(key);
    if (found == properties.end()) fail(std::string("missing property: ") + key);
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
    return value.empty() ? blinker::ByteView()
                         : blinker::ByteView(&value[0], value.size());
}

uint64_t number(const Properties& properties, const char* key) {
    const std::string& text = required(properties, key);
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    check(end != text.c_str() && *end == '\0',
          std::string("invalid number: ") + key);
    return static_cast<uint64_t>(parsed);
}

bool equalHex(blinker::ByteView value, const std::string& expected) {
    const std::vector<uint8_t> bytes = decodeHex(expected);
    return value.size == bytes.size() &&
           (value.empty() ||
            std::memcmp(value.data, &bytes[0], value.size) == 0);
}

void expect(
    const Properties& properties,
    const char* key,
    blinker::ByteView value) {
    check(equalHex(value, required(properties, key)),
          std::string("mismatch: ") + key);
}

blinker::ControllerGrant makeGrant(
    const Properties& properties,
    blinker::ControllerMutationOperation operation,
    blinker::ByteView grantId,
    blinker::ByteView deviceId,
    blinker::ByteView controllerId,
    blinker::ByteView secretDigest,
    blinker::ByteView nonce,
    blinker::ByteView signature) {
    blinker::ControllerGrant grant;
    grant.operation = operation;
    grant.grantId = grantId;
    grant.deviceInstanceId = deviceId;
    grant.ownershipGeneration = static_cast<uint32_t>(
        number(properties, "ownership_generation"));
    grant.controllerId = controllerId;
    grant.expectedCredentialVersion =
        operation == blinker::ControllerMutationOperation::Install ? 0U : 1U;
    grant.credentialVersion = 1U;
    grant.permissions =
        operation == blinker::ControllerMutationOperation::Revoke
            ? 0U
            : static_cast<uint32_t>(number(properties, "permissions"));
    grant.secretDigest = secretDigest;
    grant.controlNonce = nonce;
    grant.serverKeyId = static_cast<uint32_t>(
        number(properties, "server_key_id"));
    grant.signatureAlgorithm =
        blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
    grant.signature = signature;
    return grant;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: controller_control_golden <fixture>");
    const Properties properties = loadProperties(argv[1]);
    const std::vector<uint8_t> deviceId =
        decodeHex(required(properties, "device_instance_id"));
    const std::vector<uint8_t> controllerId =
        decodeHex(required(properties, "controller_id"));
    const std::vector<uint8_t> secret =
        decodeHex(required(properties, "controller_secret"));
    const std::vector<uint8_t> installGrantId =
        decodeHex(required(properties, "install_grant_id"));
    const std::vector<uint8_t> installNonce =
        decodeHex(required(properties, "install_control_nonce"));
    const std::vector<uint8_t> installSignature =
        decodeHex(required(properties, "install_signature"));
    const std::vector<uint8_t> revokeGrantId =
        decodeHex(required(properties, "revoke_grant_id"));
    const std::vector<uint8_t> revokeNonce =
        decodeHex(required(properties, "revoke_control_nonce"));
    const std::vector<uint8_t> revokeSignature =
        decodeHex(required(properties, "revoke_signature"));
    const std::vector<uint8_t> commitKey =
        decodeHex(required(properties, "commit_idempotency_key"));

    uint8_t secretDigest[blinker::kControllerSecretDigestSize] = {};
    check(blinker::sha256(
              view(secret),
              blinker::MutableByteSpan(secretDigest, sizeof(secretDigest)))
              .ok(),
          "secret digest failed");
    expect(
        properties,
        "expected.install_secret_digest",
        blinker::ByteView(secretDigest, sizeof(secretDigest)));

    uint8_t zeros[blinker::kControllerSecretDigestSize] = {};
    blinker::ControllerGrant install = makeGrant(
        properties,
        blinker::ControllerMutationOperation::Install,
        view(installGrantId),
        view(deviceId),
        view(controllerId),
        blinker::ByteView(secretDigest, sizeof(secretDigest)),
        view(installNonce),
        view(installSignature));
    blinker::ControllerGrant revoke = makeGrant(
        properties,
        blinker::ControllerMutationOperation::Revoke,
        view(revokeGrantId),
        view(deviceId),
        view(controllerId),
        blinker::ByteView(zeros, sizeof(zeros)),
        view(revokeNonce),
        view(revokeSignature));

    uint8_t workspace[blinker::kControllerControlWorkspaceSize] = {};
    uint8_t buffer[blinker::kControllerGrantMaxEncodedSize] = {};
    blinker::ByteView encoded;
    check(blinker::encodeControllerGrantTranscript(
              install,
              blinker::MutableByteSpan(workspace, sizeof(workspace)),
              encoded).ok(),
          "install transcript encode failed");
    expect(properties, "expected.install_grant_transcript", encoded);
    uint8_t digest[blinker::kSha256Size] = {};
    check(blinker::sha256(
              encoded,
              blinker::MutableByteSpan(digest, sizeof(digest))).ok(),
          "grant digest failed");
    expect(
        properties,
        "expected.install_grant_digest",
        blinker::ByteView(digest, sizeof(digest)));
    check(blinker::encodeControllerGrant(
              install,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "install grant encode failed");
    expect(properties, "expected.install_grant", encoded);
    blinker::ControllerGrant decodedGrant;
    check(blinker::decodeControllerGrant(encoded, decodedGrant).ok(),
          "install grant decode failed");

    blinker::ControllerMutationReceipt receipt;
    receipt.operation = install.operation;
    receipt.grantId = install.grantId;
    receipt.deviceInstanceId = install.deviceInstanceId;
    receipt.ownershipGeneration = install.ownershipGeneration;
    receipt.controllerId = install.controllerId;
    receipt.credentialVersion = install.credentialVersion;
    receipt.permissions = install.permissions;
    receipt.secretDigest = install.secretDigest;
    receipt.proofKind = blinker::ControllerReceiptProofKind::HmacSha256;
    check(blinker::encodeControllerMutationReceiptTranscript(
              receipt,
              blinker::MutableByteSpan(workspace, sizeof(workspace)),
              encoded).ok(),
          "receipt transcript encode failed");
    expect(properties, "expected.install_receipt_transcript", encoded);
    uint8_t proof[blinker::kControllerReceiptProofSize] = {};
    check(blinker::computeControllerMutationReceiptProof(
              view(secret),
              receipt,
              blinker::MutableByteSpan(workspace, sizeof(workspace)),
              blinker::MutableByteSpan(proof, sizeof(proof))).ok(),
          "receipt proof failed");
    expect(
        properties,
        "expected.install_receipt_proof",
        blinker::ByteView(proof, sizeof(proof)));
    receipt.proof = blinker::ByteView(proof, sizeof(proof));
    check(blinker::encodeControllerMutationReceipt(
              receipt,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "receipt encode failed");
    expect(properties, "expected.install_receipt", encoded);
    check(blinker::sha256(
              encoded,
              blinker::MutableByteSpan(digest, sizeof(digest))).ok(),
          "receipt digest failed");
    expect(
        properties,
        "expected.install_receipt_digest",
        blinker::ByteView(digest, sizeof(digest)));

    blinker::ControllerCommitRequest commit;
    commit.grantId = install.grantId;
    commit.operation = install.operation;
    commit.deviceInstanceId = install.deviceInstanceId;
    commit.ownershipGeneration = install.ownershipGeneration;
    commit.controllerId = install.controllerId;
    commit.credentialVersion = install.credentialVersion;
    commit.receiptDigest = blinker::ByteView(digest, sizeof(digest));
    commit.verification =
        blinker::ControllerCommitVerification::Method2Confirmed;
    commit.idempotencyKey = view(commitKey);
    check(blinker::encodeControllerCommitRequest(
              commit,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "commit encode failed");
    expect(properties, "expected.install_commit", encoded);
    blinker::ControllerCommitRequest decodedCommit;
    check(blinker::decodeControllerCommitRequest(encoded, decodedCommit).ok(),
          "commit decode failed");

    check(blinker::encodeControllerGrantTranscript(
              revoke,
              blinker::MutableByteSpan(workspace, sizeof(workspace)),
              encoded).ok(),
          "revoke transcript encode failed");
    expect(properties, "expected.revoke_grant_transcript", encoded);
    check(blinker::encodeControllerGrant(
              revoke,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "revoke grant encode failed");
    expect(properties, "expected.revoke_grant", encoded);

    receipt = blinker::ControllerMutationReceipt();
    receipt.operation = revoke.operation;
    receipt.grantId = revoke.grantId;
    receipt.deviceInstanceId = revoke.deviceInstanceId;
    receipt.ownershipGeneration = revoke.ownershipGeneration;
    receipt.controllerId = revoke.controllerId;
    receipt.credentialVersion = revoke.credentialVersion;
    receipt.permissions = 0U;
    receipt.secretDigest = revoke.secretDigest;
    receipt.proofKind = blinker::ControllerReceiptProofKind::None;
    check(blinker::encodeControllerMutationReceipt(
              receipt,
              blinker::MutableByteSpan(buffer, sizeof(buffer)),
              encoded).ok(),
          "revoke receipt encode failed");
    expect(properties, "expected.revoke_receipt", encoded);

    std::cout << "C++ controller control golden passed: 12 exact values"
              << std::endl;
    return 0;
}
