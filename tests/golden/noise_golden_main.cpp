#include <BlinkerV2Advanced.h>

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
    Properties result;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line[line.size() - 1U] == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t split = line.find('=');
        if (split == std::string::npos || split == 0U) {
            fail(std::string("invalid property: ") + line);
        }
        result[line.substr(0U, split)] = line.substr(split + 1U);
    }
    return result;
}

const std::string& requireProperty(
    const Properties& properties,
    const std::string& key) {
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
    if ((value.size() & 1U) != 0U) fail("odd hex length");
    std::vector<uint8_t> result(value.size() / 2U);
    for (size_t index = 0U; index < result.size(); ++index) {
        result[index] = static_cast<uint8_t>(
            (nibble(value[index * 2U]) << 4U) |
            nibble(value[index * 2U + 1U]));
    }
    return result;
}

blinker::ByteView view(const std::vector<uint8_t>& value) {
    return value.empty()
               ? blinker::ByteView()
               : blinker::ByteView(&value[0], value.size());
}

bool equal(blinker::ByteView first, blinker::ByteView second) {
    return first.size == second.size &&
           (first.empty() ||
            std::memcmp(first.data, second.data, first.size) == 0);
}

bool equal(blinker::ByteView first, const std::vector<uint8_t>& second) {
    return equal(first, view(second));
}

class VectorRandom : public blinker::IRandom {
public:
    explicit VectorRandom(const std::vector<uint8_t>& bytes)
        : bytes_(bytes), used_(false) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        if (used_ || bytes_.size() != 32U || output.data == nullptr ||
            output.size != bytes_.size()) {
            return blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
        std::memcpy(output.data, &bytes_[0], bytes_.size());
        used_ = true;
        return blinker::Result::success();
    }

private:
    std::vector<uint8_t> bytes_;
    bool used_;
};

struct AesOperation {
    std::vector<uint8_t> key;
    std::vector<uint8_t> nonce;
    std::vector<uint8_t> aad;
    std::vector<uint8_t> plaintext;
    std::vector<uint8_t> output;
};

class ReplayCryptoProvider : public blinker::INoiseCryptoProvider {
public:
    explicit ReplayCryptoProvider(const Properties& properties)
        : publicPrivate_(), publicOutput_(), dhPrivate_(), dhRemote_(),
          dhOutput_(), aes_() {
        const size_t publicCount = static_cast<size_t>(std::strtoul(
            requireProperty(properties, "primitive.public.count").c_str(),
            nullptr,
            10));
        check(publicCount == 2U, "unexpected public vector count");
        for (size_t index = 1U; index <= publicCount; ++index) {
            const std::string prefix =
                std::string("primitive.public.") + std::to_string(index);
            publicPrivate_.push_back(decodeHex(requireProperty(
                properties,
                prefix + ".private")));
            publicOutput_.push_back(decodeHex(requireProperty(
                properties,
                prefix + ".output")));
        }
        check(
            requireProperty(properties, "primitive.dh.count") == "1",
            "unexpected DH vector count");
        dhPrivate_ = decodeHex(requireProperty(
            properties,
            "primitive.dh.1.private"));
        dhRemote_ = decodeHex(requireProperty(
            properties,
            "primitive.dh.1.remote"));
        dhOutput_ = decodeHex(requireProperty(
            properties,
            "primitive.dh.1.output"));

        const size_t aesCount = static_cast<size_t>(std::strtoul(
            requireProperty(properties, "primitive.aes.count").c_str(),
            nullptr,
            10));
        check(aesCount == 3U, "unexpected AES vector count");
        for (size_t index = 1U; index <= aesCount; ++index) {
            const std::string prefix =
                std::string("primitive.aes.") + std::to_string(index);
            AesOperation operation;
            operation.key = decodeHex(requireProperty(
                properties,
                prefix + ".key"));
            operation.nonce = decodeHex(requireProperty(
                properties,
                prefix + ".nonce"));
            operation.aad = decodeHex(requireProperty(
                properties,
                prefix + ".aad"));
            operation.plaintext = decodeHex(requireProperty(
                properties,
                prefix + ".plaintext"));
            operation.output = decodeHex(requireProperty(
                properties,
                prefix + ".output"));
            aes_.push_back(operation);
        }
    }

    blinker::Result x25519PublicKey(
        blinker::ByteView privateKey,
        blinker::MutableByteSpan publicKey) override {
        for (size_t index = 0U; index < publicPrivate_.size(); ++index) {
            if (equal(privateKey, publicPrivate_[index])) {
                return copy(publicOutput_[index], publicKey);
            }
        }
        return blinker::Result::failure(blinker::ErrorCode::InternalError);
    }

    blinker::Result x25519(
        blinker::ByteView privateKey,
        blinker::ByteView remotePublicKey,
        blinker::MutableByteSpan sharedSecret) override {
        const bool forward = equal(privateKey, dhPrivate_) &&
                             equal(remotePublicKey, dhRemote_);
        const bool reverse = equal(privateKey, publicPrivate_[1]) &&
                             equal(remotePublicKey, publicOutput_[0]);
        if (!forward && !reverse) {
            return blinker::Result::failure(blinker::ErrorCode::InternalError);
        }
        return copy(dhOutput_, sharedSecret);
    }

    blinker::Result aes256GcmEncrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView plaintext,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        for (size_t index = 0U; index < aes_.size(); ++index) {
            const AesOperation& operation = aes_[index];
            if (equal(key, operation.key) && equal(nonce, operation.nonce) &&
                equal(associatedData, operation.aad) &&
                equal(plaintext, operation.plaintext)) {
                blinker::Result result = copy(operation.output, output);
                if (result) written = operation.output.size();
                return result;
            }
        }
        return blinker::Result::failure(blinker::ErrorCode::InternalError);
    }

    blinker::Result aes256GcmDecrypt(
        blinker::ByteView key,
        blinker::ByteView nonce,
        blinker::ByteView associatedData,
        blinker::ByteView ciphertextAndTag,
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        for (size_t index = 0U; index < aes_.size(); ++index) {
            const AesOperation& operation = aes_[index];
            if (equal(key, operation.key) && equal(nonce, operation.nonce) &&
                equal(associatedData, operation.aad) &&
                equal(ciphertextAndTag, operation.output)) {
                blinker::Result result = copy(operation.plaintext, output);
                if (result) written = operation.plaintext.size();
                return result;
            }
        }
        return blinker::Result::failure(
            blinker::ErrorCode::AuthenticationRequired);
    }

private:
    static blinker::Result copy(
        const std::vector<uint8_t>& source,
        blinker::MutableByteSpan output) {
        if (output.size < source.size() ||
            (!source.empty() && output.data == nullptr)) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        if (!source.empty()) {
            std::memcpy(output.data, &source[0], source.size());
        }
        return blinker::Result::success();
    }

    std::vector<std::vector<uint8_t> > publicPrivate_;
    std::vector<std::vector<uint8_t> > publicOutput_;
    std::vector<uint8_t> dhPrivate_;
    std::vector<uint8_t> dhRemote_;
    std::vector<uint8_t> dhOutput_;
    std::vector<AesOperation> aes_;
};

void checkExpected(
    const Properties& properties,
    const std::string& key,
    blinker::ByteView actual) {
    const std::vector<uint8_t> expected = decodeHex(requireProperty(
        properties,
        key));
    check(equal(actual, expected), key + " mismatch");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: noise_golden_main <fixture>");
    const Properties properties = loadProperties(argv[1]);
    ReplayCryptoProvider crypto(properties);
    const std::vector<uint8_t> initiatorPrivate = decodeHex(requireProperty(
        properties,
        "primitive.public.1.private"));
    const std::vector<uint8_t> responderPrivate = decodeHex(requireProperty(
        properties,
        "primitive.public.2.private"));
    VectorRandom initiatorRandom(initiatorPrivate);
    VectorRandom responderRandom(responderPrivate);
    blinker::security::NoiseNnSession initiator(
        crypto,
        initiatorRandom);
    blinker::security::NoiseNnSession responder(
        crypto,
        responderRandom);

    checkExpected(
        properties,
        "expected.prologue",
        blinker::noise::localSecureNoisePrologue(
            blinker::noise::NoiseNnPattern::Nn));
    check(initiator.begin(
              blinker::security::NoiseRole::Initiator,
              blinker::noise::NoiseNnPattern::Nn,
              blinker::ByteView(),
              blinker::noise::localSecureNoisePrologue(
                  blinker::noise::NoiseNnPattern::Nn)).ok(),
          "initiator begin failed");
    check(responder.begin(
              blinker::security::NoiseRole::Responder,
              blinker::noise::NoiseNnPattern::Nn,
              blinker::ByteView(),
              blinker::noise::localSecureNoisePrologue(
                  blinker::noise::NoiseNnPattern::Nn)).ok(),
          "responder begin failed");

    const std::vector<uint8_t> firstPayload = decodeHex("a200010101");
    const std::vector<uint8_t> secondPayload = decodeHex("a10001");
    uint8_t firstBytes[128] = {};
    uint8_t secondBytes[128] = {};
    uint8_t payloadBytes[32] = {};
    blinker::ByteView first;
    blinker::ByteView second;
    blinker::ByteView payload;
    check(initiator.writeHandshake(
              view(firstPayload),
              blinker::MutableByteSpan(firstBytes, sizeof(firstBytes)),
              first).ok(),
          "message1 write failed");
    checkExpected(properties, "expected.message1", first);
    check(responder.readHandshake(
              first,
              blinker::MutableByteSpan(payloadBytes, sizeof(payloadBytes)),
              payload).ok() && equal(payload, firstPayload),
          "message1 read failed");
    check(responder.writeHandshake(
              view(secondPayload),
              blinker::MutableByteSpan(secondBytes, sizeof(secondBytes)),
              second).ok(),
          "message2 write failed");
    checkExpected(properties, "expected.message2", second);
    check(initiator.readHandshake(
              second,
              blinker::MutableByteSpan(payloadBytes, sizeof(payloadBytes)),
              payload).ok() && equal(payload, secondPayload),
          "message2 read failed");

    blinker::ByteView handshakeHash;
    check(initiator.handshakeHash(handshakeHash).ok(), "hash unavailable");
    checkExpected(properties, "expected.handshake_hash", handshakeHash);
    check(responder.handshakeHash(payload).ok() && equal(payload, handshakeHash),
          "responder handshake hash mismatch");

    const std::vector<uint8_t> request = decodeHex("01040007a10001");
    const std::vector<uint8_t> response = decodeHex("01050007");
    uint8_t transportBytes[128] = {};
    uint8_t plaintextBytes[128] = {};
    blinker::ByteView transport;
    blinker::ByteView plaintext;
    check(initiator.encryptTransport(
              view(request),
              blinker::MutableByteSpan(transportBytes, sizeof(transportBytes)),
              transport).ok(),
          "transport1 encryption failed");
    checkExpected(properties, "expected.transport1", transport);
    check(responder.decryptTransport(
              transport,
              blinker::MutableByteSpan(plaintextBytes, sizeof(plaintextBytes)),
              plaintext).ok() && equal(plaintext, request),
          "transport1 decryption failed");

    uint8_t recordBytes[160] = {};
    blinker::ByteView record;
    check(blinker::noise::encodeLocalSecureRecord(
              blinker::noise::LocalSecureRecordType::Transport,
              transport,
              blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
              record).ok(),
          "record3 encoding failed");
    checkExpected(properties, "expected.record3", record);

    check(responder.encryptTransport(
              view(response),
              blinker::MutableByteSpan(transportBytes, sizeof(transportBytes)),
              transport).ok(),
          "transport2 encryption failed");
    checkExpected(properties, "expected.transport2", transport);
    check(initiator.decryptTransport(
              transport,
              blinker::MutableByteSpan(plaintextBytes, sizeof(plaintextBytes)),
              plaintext).ok() && equal(plaintext, response),
          "transport2 decryption failed");

    check(blinker::noise::encodeLocalSecureRecord(
              blinker::noise::LocalSecureRecordType::InitiatorHandshake,
              first,
              blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
              record).ok(),
          "record1 encoding failed");
    checkExpected(properties, "expected.record1", record);
    check(blinker::noise::encodeLocalSecureRecord(
              blinker::noise::LocalSecureRecordType::ResponderHandshake,
              second,
              blinker::MutableByteSpan(recordBytes, sizeof(recordBytes)),
              record).ok(),
          "record2 encoding failed");
    checkExpected(properties, "expected.record2", record);

    std::cout << "C++ Noise golden passed: 9 exact values" << std::endl;
    return 0;
}
