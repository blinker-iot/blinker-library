#include <BlinkerV2/protocol/ble/Fragment.h>
#include <BlinkerV2/security/DirectSecureSession.h>

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
    std::cerr << "DirectSecureRecord golden: " << message << std::endl;
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

const std::string& required(const Properties& properties, const std::string& key) {
    const Properties::const_iterator found = properties.find(key);
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

uint32_t number(const Properties& properties, const std::string& key) {
    const std::string& value = required(properties, key);
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    check(end != value.c_str() && *end == '\0' && parsed <= UINT32_MAX,
          std::string("invalid number: ") + key);
    return static_cast<uint32_t>(parsed);
}

blinker::ByteView view(const std::vector<uint8_t>& value) {
    return value.empty() ? blinker::ByteView()
                         : blinker::ByteView(&value[0], value.size());
}

bool equal(blinker::ByteView actual, const std::vector<uint8_t>& expected) {
    return actual.size == expected.size() &&
           (actual.empty() ||
            std::memcmp(actual.data, &expected[0], actual.size) == 0);
}

class ReplayAead final : public blinker::IX25519AesGcmCryptoProvider {
public:
    explicit ReplayAead(const Properties& properties) : properties_(properties) {}

    blinker::Result x25519PublicKey(
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return unsupported();
    }

    blinker::Result x25519(
        blinker::ByteView,
        blinker::ByteView,
        blinker::MutableByteSpan) override {
        return unsupported();
    }

    blinker::Result aes256GcmEncrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView aad,
        blinker::ByteView plaintext,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        for (uint32_t index = 1U; index <= primitiveCount(); ++index) {
            const std::string prefix = primitivePrefix(index);
            if (!matches(key, prefix + "key") ||
                !matches(nonce, prefix + "nonce") ||
                !matches(aad, prefix + "aad") ||
                !matches(plaintext, prefix + "plaintext")) {
                continue;
            }
            const std::vector<uint8_t> expected =
                decodeHex(required(properties_, prefix + "output"));
            if (output.data == nullptr || output.size < expected.size()) {
                return blinker::Result::failure(
                    blinker::ErrorCode::BufferTooSmall);
            }
            std::memcpy(output.data, &expected[0], expected.size());
            written = expected.size();
            return blinker::Result::success();
        }
        return blinker::Result::failure(blinker::ErrorCode::InvalidArgument);
    }

    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView aad,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        for (uint32_t index = 1U; index <= primitiveCount(); ++index) {
            const std::string prefix = primitivePrefix(index);
            if (!matches(key, prefix + "key") ||
                !matches(nonce, prefix + "nonce") ||
                !matches(aad, prefix + "aad") ||
                !matches(ciphertextAndTag, prefix + "output")) {
                continue;
            }
            const std::vector<uint8_t> expected =
                decodeHex(required(properties_, prefix + "plaintext"));
            if (output.data == nullptr || output.size < expected.size()) {
                return blinker::Result::failure(
                    blinker::ErrorCode::BufferTooSmall);
            }
            std::memcpy(output.data, &expected[0], expected.size());
            written = expected.size();
            return blinker::Result::success();
        }
        return blinker::Result::failure(
            blinker::ErrorCode::AuthenticationRequired);
    }

private:
    static blinker::Result unsupported() {
        return blinker::Result::failure(
            blinker::ErrorCode::UnsupportedFeature);
    }

    uint32_t primitiveCount() const {
        return number(properties_, "primitive.aes.count");
    }

    static std::string primitivePrefix(uint32_t index) {
        return std::string("primitive.aes.") + std::to_string(index) + ".";
    }

    bool matches(blinker::ByteView actual, const std::string& key) const {
        return equal(actual, decodeHex(required(properties_, key)));
    }

    const Properties& properties_;
};

void checkExpected(
    const Properties& properties,
    const std::string& key,
    blinker::ByteView actual) {
    check(equal(actual, decodeHex(required(properties, key))),
          std::string("mismatch: ") + key);
}

size_t fragmentCount(size_t size, size_t capacity) {
    return capacity == 0U ? 0U : size / capacity + (size % capacity != 0U ? 1U : 0U);
}

void checkAtt20(
    const Properties& properties,
    const char* name,
    blinker::ByteView plaintext,
    blinker::ByteView record) {
    const size_t att = number(properties, "att.payload");
    const size_t header = number(properties, "fragment.header");
    check(att == blinker::ble::kMinimumPacketSize,
          "fixture no longer represents ATT20");
    check(header == blinker::ble::kFragmentHeaderSize,
          "fragment header mismatch");
    const size_t capacity = att - header;
    check(fragmentCount(plaintext.size, capacity) == number(
              properties, std::string("expected.att20.raw.") + name),
          std::string("raw ATT20 count mismatch: ") + name);
    check(fragmentCount(record.size, capacity) == number(
              properties, std::string("expected.att20.secure.") + name),
          std::string("secure ATT20 count mismatch: ") + name);
}

void checkCommandFragments(
    const Properties& properties,
    blinker::ByteView record) {
    const size_t packetSize = number(properties, "att.payload");
    const size_t capacity = packetSize - blinker::ble::kFragmentHeaderSize;
    const uint8_t frameId = static_cast<uint8_t>(
        number(properties, "fragment.frame_id"));
    const size_t expectedCount = number(
        properties, "expected.fragment.command.count");
    check(fragmentCount(record.size, capacity) == expectedCount,
          "command fragment count mismatch");
    size_t offset = 0U;
    for (size_t index = 0U; index < expectedCount; ++index) {
        const size_t remaining = record.size - offset;
        const size_t payloadSize = remaining < capacity ? remaining : capacity;
        uint8_t packet[blinker::ble::kMinimumPacketSize];
        blinker::ble::FragmentHeader header;
        header.flags = offset == 0U ? blinker::ble::FragmentFlagStart
                                    : blinker::ble::FragmentFlagNone;
        if (payloadSize == remaining) {
            header.flags = static_cast<uint8_t>(
                header.flags | blinker::ble::FragmentFlagEnd);
        }
        header.frameId = frameId;
        header.index = static_cast<uint8_t>(index);
        check(blinker::ble::encodeFragmentHeader(
                  header, blinker::MutableByteSpan(packet, sizeof(packet))).ok(),
              "fragment header encoding failed");
        std::memcpy(
            packet + blinker::ble::kFragmentHeaderSize,
            record.data + offset,
            payloadSize);
        checkExpected(
            properties,
            std::string("expected.fragment.command.") + std::to_string(index),
            blinker::ByteView(
                packet,
                blinker::ble::kFragmentHeaderSize + payloadSize));
        offset += payloadSize;
    }
    check(offset == record.size, "fragmentation did not consume record");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: direct_secure_golden <fixture>");
    const Properties properties = loadProperties(argv[1]);
    const std::vector<uint8_t> secret = decodeHex(required(properties, "secret"));
    const std::vector<uint8_t> controllerId =
        decodeHex(required(properties, "controller_id"));
    const std::vector<uint8_t> clientNonce =
        decodeHex(required(properties, "client_nonce"));
    const std::vector<uint8_t> deviceNonce =
        decodeHex(required(properties, "device_nonce"));
    const std::vector<uint8_t> command =
        decodeHex(required(properties, "plaintext.command"));
    const std::vector<uint8_t> ack =
        decodeHex(required(properties, "plaintext.ack"));
    const std::vector<uint8_t> patch =
        decodeHex(required(properties, "plaintext.patch"));

    blinker::security::DirectSecureContext context;
    context.controllerId = view(controllerId);
    context.domain = static_cast<blinker::ControllerCredentialDomain>(
        number(properties, "domain"));
    context.ownershipGeneration = number(properties, "ownership_generation");
    context.credentialVersion = number(properties, "credential_version");
    context.permissions = number(properties, "permissions");
    context.clientNonce = view(clientNonce);
    context.deviceNonce = view(deviceNonce);

    blinker::security::DirectSecureKeyMaterial material;
    check(blinker::security::deriveDirectSecureKeyMaterial(
              view(secret), context, material).ok(),
          "key derivation failed");
    checkExpected(properties, "expected.initiator_key", blinker::ByteView(
        material.initiatorKey, sizeof(material.initiatorKey)));
    checkExpected(properties, "expected.responder_key", blinker::ByteView(
        material.responderKey, sizeof(material.responderKey)));
    checkExpected(properties, "expected.initiator_nonce_prefix", blinker::ByteView(
        material.initiatorNoncePrefix, sizeof(material.initiatorNoncePrefix)));
    checkExpected(properties, "expected.responder_nonce_prefix", blinker::ByteView(
        material.responderNoncePrefix, sizeof(material.responderNoncePrefix)));
    checkExpected(properties, "expected.session_binding", blinker::ByteView(
        material.sessionBinding, sizeof(material.sessionBinding)));
    material.clear();

    ReplayAead crypto(properties);
    blinker::security::DirectSecureSession initiator;
    blinker::security::DirectSecureSession responder;
    check(initiator.begin(
              blinker::security::DirectSecureRole::Initiator,
              view(secret), context).ok(),
          "initiator begin failed");
    check(responder.begin(
              blinker::security::DirectSecureRole::Responder,
              view(secret), context).ok(),
          "responder begin failed");

    uint8_t recordStorage[96];
    uint8_t plaintextStorage[64];
    blinker::ByteView record;
    blinker::ByteView opened;

    check(initiator.seal(
              crypto, view(command),
              blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
              record).ok(),
          "command seal failed");
    checkExpected(properties, "expected.record.command", record);
    checkAtt20(properties, "command", view(command), record);
    checkCommandFragments(properties, record);
    check(responder.open(
              crypto, record,
              blinker::MutableByteSpan(plaintextStorage, sizeof(plaintextStorage)),
              opened).ok(),
          "command open failed");
    check(equal(opened, command), "command plaintext mismatch");

    check(responder.seal(
              crypto, view(ack),
              blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
              record).ok(),
          "ack seal failed");
    checkExpected(properties, "expected.record.ack", record);
    checkAtt20(properties, "ack", view(ack), record);
    check(initiator.open(
              crypto, record,
              blinker::MutableByteSpan(plaintextStorage, sizeof(plaintextStorage)),
              opened).ok(),
          "ack open failed");
    check(equal(opened, ack), "ack plaintext mismatch");

    check(responder.seal(
              crypto, view(patch),
              blinker::MutableByteSpan(recordStorage, sizeof(recordStorage)),
              record).ok(),
          "patch seal failed");
    checkExpected(properties, "expected.record.patch", record);
    checkAtt20(properties, "patch", view(patch), record);
    check(initiator.open(
              crypto, record,
              blinker::MutableByteSpan(plaintextStorage, sizeof(plaintextStorage)),
              opened).ok(),
          "patch open failed");
    check(equal(opened, patch), "patch plaintext mismatch");

    std::cout << "C++ DirectSecureRecord golden passed: KDF, 3 records, ATT20"
              << std::endl;
    return 0;
}
