#include <BlinkerV2Advanced.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "CHECK failed at line " << __LINE__ << ": "          \
                      << #condition << std::endl;                               \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

std::vector<uint8_t> fromHex(const char* text) {
    const size_t length = std::strlen(text);
    CHECK_TRUE((length % 2U) == 0U);
    std::vector<uint8_t> output(length / 2U);
    for (size_t index = 0U; index < output.size(); ++index) {
        const char pair[3] = {
            text[index * 2U],
            text[index * 2U + 1U],
            '\0'};
        output[index] = static_cast<uint8_t>(std::strtoul(pair, nullptr, 16));
    }
    return output;
}

class AtomicMemoryStore : public blinker::IAtomicBlobStore {
public:
    explicit AtomicMemoryStore(
        blinker::StorageProtection value =
            blinker::StorageProtection::EncryptedFlash)
        : length(0U),
          present(false),
          replaceResult(blinker::Result::success()),
          protectionValue(value),
          replaceCalls(0U) {
        std::memset(bytes, 0, sizeof(bytes));
    }

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < length) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, bytes, length);
        written = length;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        ++replaceCalls;
        if (!replaceResult) return replaceResult;
        if (value.data == nullptr || value.size > sizeof(bytes)) {
            return blinker::Result::failure(
                blinker::ErrorCode::CapacityExceeded);
        }
        std::memcpy(bytes, value.data, value.size);
        length = value.size;
        present = true;
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        present = false;
        length = 0U;
        std::memset(bytes, 0, sizeof(bytes));
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return protectionValue;
    }

    uint8_t bytes[128];
    size_t length;
    bool present;
    blinker::Result replaceResult;
    blinker::StorageProtection protectionValue;
    uint32_t replaceCalls;
};

class FixedRandom : public blinker::IRandom {
public:
    FixedRandom()
        : calls(0U), allZero(false), result(blinker::Result::success()) {}

    blinker::Result fill(blinker::MutableByteSpan output) override {
        ++calls;
        if (!result) return result;
        if (output.data == nullptr ||
            output.size != blinker::kDeviceInstanceIdSize) {
            return blinker::Result::failure(
                blinker::ErrorCode::InvalidArgument);
        }
        for (size_t index = 0U; index < output.size; ++index) {
            output.data[index] = allZero
                                     ? 0U
                                     : static_cast<uint8_t>(0xA0U + index);
        }
        return blinker::Result::success();
    }

    uint32_t calls;
    bool allZero;
    blinker::Result result;
};

std::vector<uint8_t> p256PublicKey() {
    return fromHex(
        "046b17d1f2e12c4247f8bce6e563a440"
        "f277037d812deb33a0f4a13945d898c296"
        "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162"
        "bce33576b315ececbb6406837bf51f5");
}

class FakeP256Verifier : public blinker::IP256DigestVerifier {
public:
    FakeP256Verifier()
        : verifyCalls(0U), verifyResult(blinker::Result::success()) {}

    blinker::Result verifyDigest(
        blinker::ByteView publicKey,
        blinker::ByteView digest,
        blinker::ByteView signature) override {
        ++verifyCalls;
        CHECK_TRUE(blinker::isCanonicalP256PublicKey(publicKey));
        CHECK_EQ(static_cast<size_t>(blinker::kP256DigestSize), digest.size);
        CHECK_TRUE(blinker::isCanonicalP256Signature(signature));
        return verifyResult;
    }

    uint32_t verifyCalls;
    blinker::Result verifyResult;
};

class FakeEd25519Verifier : public blinker::IEd25519DigestVerifier {
public:
    FakeEd25519Verifier() : verifyCalls(0U) {}

    blinker::Result verifyDigest(
        blinker::ByteView publicKey,
        blinker::ByteView digest,
        blinker::ByteView signature) override {
        ++verifyCalls;
        CHECK_TRUE(blinker::isCanonicalEd25519PublicKey(publicKey));
        CHECK_EQ(static_cast<size_t>(blinker::kEd25519DigestSize), digest.size);
        CHECK_TRUE(blinker::isCanonicalEd25519Signature(signature));
        return blinker::Result::success();
    }

    uint32_t verifyCalls;
};

void testP256SignatureNormalization() {
    uint8_t signature[blinker::kP256SignatureSize] = {};
    signature[blinker::kP256PrivateKeySize - 1U] = 1U;
    const std::vector<uint8_t> high = fromHex(
        "ffffffff00000000ffffffffffffffff"
        "bce6faada7179e84f3b9cac2fc632550");
    std::memcpy(
        signature + blinker::kP256PrivateKeySize,
        high.data(),
        high.size());
    CHECK_TRUE(blinker::normalizeP256Signature(
        blinker::MutableByteSpan(signature, sizeof(signature))).ok());
    CHECK_TRUE(blinker::isCanonicalP256Signature(
        blinker::ByteView(signature, sizeof(signature))));
    CHECK_EQ(static_cast<uint8_t>(1U), signature[sizeof(signature) - 1U]);
}

void fillCanonicalSignature(uint8_t* signature) {
    std::memset(signature, 0, blinker::kP256SignatureSize);
    signature[blinker::kP256PrivateKeySize - 1U] = 1U;
    signature[blinker::kP256SignatureSize - 1U] = 1U;
}

void testServerKeyRing() {
    FakeP256Verifier crypto;
    const std::vector<uint8_t> publicKey = p256PublicKey();
    const blinker::P256ServerPublicKey keys[] = {
        blinker::P256ServerPublicKey(
            7U,
            blinker::ByteView(publicKey.data(), publicKey.size()))};
    blinker::P256ServerKeyRingVerifier keyRing(crypto, keys, 1U);

    uint8_t signature[blinker::kP256SignatureSize] = {};
    fillCanonicalSignature(signature);
    uint8_t digest[blinker::kP256DigestSize] = {};
    CHECK_TRUE(keyRing.verifyDigest(
        7U,
        blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
        blinker::ByteView(digest, sizeof(digest)),
        blinker::ByteView(signature, sizeof(signature))).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        keyRing.verifyDigest(
            8U,
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(signature, sizeof(signature))).code());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);
    const std::vector<uint8_t> highS = fromHex(
        "ffffffff00000000ffffffffffffffff"
        "bce6faada7179e84f3b9cac2fc632550");
    std::memcpy(
        signature + blinker::kP256PrivateKeySize,
        highS.data(),
        highS.size());
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        keyRing.verifyDigest(
            7U,
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(signature, sizeof(signature))).code());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);
    fillCanonicalSignature(signature);

    const blinker::P256ServerPublicKey duplicates[] = {
        keys[0], keys[0]};
    blinker::P256ServerKeyRingVerifier duplicateRing(
        crypto,
        duplicates,
        2U);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        duplicateRing.verifyDigest(
            7U,
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(signature, sizeof(signature))).code());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);


}

void testEd25519ServerKeyRing() {
    const std::vector<uint8_t> publicKey = fromHex(
        "03a107bff3ce10be1d70dd18e74bc099"
        "67e4d6309ba50d5f1ddc8664125531b8");
    std::vector<uint8_t> signature = fromHex(
        "b335dc5eae70b48764f11207bbaa6275"
        "7cbb94e820ceba376aeb62a5815dfabe"
        "784217758836ca28504a36a2160abbdc"
        "9e7874fe1d6524a58665ef2c426da30e");
    CHECK_TRUE(blinker::isCanonicalEd25519PublicKey(
        blinker::ByteView(publicKey.data(), publicKey.size())));
    CHECK_TRUE(blinker::isCanonicalEd25519Signature(
        blinker::ByteView(signature.data(), signature.size())));

    FakeEd25519Verifier crypto;
    const blinker::Ed25519ServerPublicKey keys[] = {
        blinker::Ed25519ServerPublicKey(
            9U,
            blinker::ByteView(publicKey.data(), publicKey.size()))};
    blinker::Ed25519ServerKeyRingVerifier keyRing(crypto, keys, 1U);
    uint8_t digest[blinker::kEd25519DigestSize] = {};
    CHECK_TRUE(keyRing.verifyDigest(
        9U,
        blinker::ServerSignatureAlgorithm::Ed25519,
        blinker::ByteView(digest, sizeof(digest)),
        blinker::ByteView(signature.data(), signature.size())).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);

    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        keyRing.verifyDigest(
            9U,
            blinker::ServerSignatureAlgorithm::EcdsaP256Sha256Raw,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(signature.data(), signature.size())).code());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);

    const std::vector<uint8_t> order = fromHex(
        "edd3f55c1a631258d69cf7a2def9de14"
        "00000000000000000000000000000010");
    std::memcpy(
        signature.data() + blinker::kEd25519PublicKeySize,
        order.data(),
        order.size());
    CHECK_TRUE(!blinker::isCanonicalEd25519Signature(
        blinker::ByteView(signature.data(), signature.size())));
    CHECK_EQ(
        blinker::ErrorCode::AuthenticationRequired,
        keyRing.verifyDigest(
            9U,
            blinker::ServerSignatureAlgorithm::Ed25519,
            blinker::ByteView(digest, sizeof(digest)),
            blinker::ByteView(signature.data(), signature.size())).code());
    CHECK_EQ(static_cast<uint32_t>(1U), crypto.verifyCalls);

    uint8_t identity[blinker::kEd25519PublicKeySize] = {1U};
    CHECK_TRUE(!blinker::isCanonicalEd25519PublicKey(
        blinker::ByteView(identity, sizeof(identity))));
}

void testDeviceInstanceIdLifecycle() {
    static_assert(
        sizeof(blinker::DeviceInstanceId) ==
            blinker::kDeviceInstanceIdSize,
        "DeviceInstanceId must not carry runtime metadata");
    static_assert(
        blinker::DeviceInstanceIdStore::serializedSize == 28U,
        "Device instance record size is part of the storage gate");

    AtomicMemoryStore storage(blinker::StorageProtection::PlainFlash);
    FixedRandom random;
    blinker::DeviceInstanceIdStore store(storage);
    blinker::DeviceInstanceId first;
    CHECK_TRUE(store.loadOrCreate(random, first).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), random.calls);
    CHECK_EQ(static_cast<uint32_t>(1U), storage.replaceCalls);
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceInstanceIdStore::serializedSize),
        storage.length);
    CHECK_TRUE(std::memcmp(storage.bytes, "BDN2", 4U) == 0);
    CHECK_EQ(static_cast<uint8_t>(1U), storage.bytes[4]);
    CHECK_TRUE(blinker::isValidDeviceInstanceId(first));

    char physicalId[blinker::kDeviceInstancePhysicalIdSize] = {};
    blinker::StringView physicalIdView;
    CHECK_TRUE(blinker::formatDeviceInstancePhysicalId(
        first,
        blinker::MutableCharSpan(physicalId, sizeof(physicalId)),
        physicalIdView).ok());
    const char expected[] = "dev_a0a1a2a3a4a5a6a7a8a9aaabacadaeaf";
    CHECK_EQ(sizeof(expected) - 1U, physicalIdView.size);
    CHECK_TRUE(std::memcmp(
        expected,
        physicalIdView.data,
        physicalIdView.size) == 0);

    char tooSmall[blinker::kDeviceInstancePhysicalIdSize - 1U] = {};
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        blinker::formatDeviceInstancePhysicalId(
            first,
            blinker::MutableCharSpan(tooSmall, sizeof(tooSmall)),
            physicalIdView).code());

    FixedRandom unavailableRandom;
    unavailableRandom.result = blinker::Result::failure(
        blinker::ErrorCode::NotConfigured);
    blinker::DeviceInstanceId second;
    CHECK_TRUE(store.loadOrCreate(unavailableRandom, second).ok());
    CHECK_EQ(static_cast<uint32_t>(0U), unavailableRandom.calls);
    CHECK_TRUE(blinker::sameDeviceInstanceId(first, second));
    CHECK_EQ(
        blinker::StorageProtection::PlainFlash,
        store.protection());

    storage.bytes[8] ^= 0x01U;
    FixedRandom replacementRandom;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        store.loadOrCreate(replacementRandom, second).code());
    CHECK_EQ(static_cast<uint32_t>(0U), replacementRandom.calls);
    CHECK_EQ(static_cast<uint32_t>(1U), storage.replaceCalls);

    AtomicMemoryStore emptyStorage;
    blinker::DeviceInstanceIdStore emptyStore(emptyStorage);
    FixedRandom zeroRandom;
    zeroRandom.allZero = true;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        emptyStore.loadOrCreate(zeroRandom, second).code());
    CHECK_EQ(static_cast<uint32_t>(1U), zeroRandom.calls);
    CHECK_EQ(static_cast<uint32_t>(0U), emptyStorage.replaceCalls);

    FixedRandom failedRandom;
    failedRandom.result = blinker::Result::failure(
        blinker::ErrorCode::NotConfigured);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        emptyStore.loadOrCreate(failedRandom, second).code());
    CHECK_EQ(static_cast<uint32_t>(1U), failedRandom.calls);
}

void testDeviceKeyStoreLifecycle() {
    static_assert(
        blinker::DeviceKeyStore::serializedSize == 44U,
        "DeviceKey record size is part of the storage gate");

    AtomicMemoryStore storage;
    blinker::DeviceKeyStore store(storage);
    blinker::DeviceKey key;
    for (size_t index = 0U; index < sizeof(key.bytes); ++index) {
        key.bytes[index] = static_cast<uint8_t>(index + 1U);
    }

    CHECK_TRUE(store.replace(key).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), storage.replaceCalls);
    CHECK_EQ(
        static_cast<size_t>(blinker::DeviceKeyStore::serializedSize),
        storage.length);
    CHECK_TRUE(std::memcmp(storage.bytes, "BDK2", 4U) == 0);
    CHECK_EQ(static_cast<uint8_t>(1U), storage.bytes[4]);
    CHECK_EQ(
        blinker::StorageProtection::EncryptedFlash,
        store.protection());

    blinker::DeviceKey loaded;
    CHECK_TRUE(store.load(loaded).ok());
    CHECK_TRUE(blinker::sameDeviceKey(key, loaded));
    blinker::clearDeviceKey(loaded);

    storage.bytes[8U] ^= 0x01U;
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        store.load(loaded).code());

    blinker::DeviceKey invalid;
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        store.replace(invalid).code());
    CHECK_EQ(static_cast<uint32_t>(1U), storage.replaceCalls);

    CHECK_TRUE(store.clear().ok());
    CHECK_TRUE(store.clear().ok());
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(loaded).code());
    blinker::clearDeviceKey(key);
    blinker::clearDeviceKey(invalid);
    blinker::clearDeviceKey(loaded);
}

} // namespace

int main() {
    testDeviceInstanceIdLifecycle();
    testDeviceKeyStoreLifecycle();
    testP256SignatureNormalization();
    testServerKeyRing();
    testEd25519ServerKeyRing();
    std::cout << "identity and ownership verification tests passed" << std::endl;
    return 0;
}
