#include <BlinkerV2/core/Crc32.h>
#include <BlinkerV2/provisioning/WifiCredentialStore.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "WiFi store check failed at line " << __LINE__       \
                      << ": " << #condition << std::endl;                     \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual) CHECK_TRUE((expected) == (actual))

class FaultAtomicStore : public blinker::IAtomicBlobStore {
public:
    explicit FaultAtomicStore(
        blinker::StorageProtection protection =
            blinker::StorageProtection::EncryptedFlash)
        : data(),
          size(0U),
          present(false),
          protectionValue(protection),
          replaceError(blinker::ErrorCode::Ok),
          clearError(blinker::ErrorCode::Ok),
          applyReplaceOnFailure(false),
          applyClearOnFailure(false),
          replaceCalls(0U),
          clearCalls(0U),
          loadCalls(0U) {}

    blinker::Result load(
        blinker::MutableByteSpan output,
        size_t& written) override {
        ++loadCalls;
        written = 0U;
        if (!present) {
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (output.data == nullptr || output.size < size) {
            return blinker::Result::failure(
                blinker::ErrorCode::BufferTooSmall);
        }
        std::memcpy(output.data, data, size);
        written = size;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView value) override {
        ++replaceCalls;
        const bool fail = replaceError != blinker::ErrorCode::Ok;
        if (!fail || applyReplaceOnFailure) {
            CHECK_TRUE(value.data != nullptr);
            CHECK_TRUE(value.size <= sizeof(data));
            std::memset(data, 0, sizeof(data));
            std::memcpy(data, value.data, value.size);
            size = value.size;
            present = true;
        }
        if (fail) {
            const blinker::ErrorCode error = replaceError;
            replaceError = blinker::ErrorCode::Ok;
            applyReplaceOnFailure = false;
            return blinker::Result::failure(error);
        }
        return blinker::Result::success();
    }

    blinker::Result clear() override {
        ++clearCalls;
        const bool fail = clearError != blinker::ErrorCode::Ok;
        if (!fail || applyClearOnFailure) {
            std::memset(data, 0, sizeof(data));
            size = 0U;
            present = false;
        }
        if (fail) {
            const blinker::ErrorCode error = clearError;
            clearError = blinker::ErrorCode::Ok;
            applyClearOnFailure = false;
            return blinker::Result::failure(error);
        }
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return protectionValue;
    }

    uint8_t data[128];
    size_t size;
    bool present;
    blinker::StorageProtection protectionValue;
    blinker::ErrorCode replaceError;
    blinker::ErrorCode clearError;
    bool applyReplaceOnFailure;
    bool applyClearOnFailure;
    uint32_t replaceCalls;
    uint32_t clearCalls;
    uint32_t loadCalls;
};

blinker::WifiNetworkConfig network(
    const char* ssid,
    const char* credential,
    blinker::WifiAuthentication authentication,
    bool hidden = false) {
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(ssid),
        std::strlen(ssid));
    config.authentication = authentication;
    config.credential = blinker::ByteView(
        reinterpret_cast<const uint8_t*>(credential),
        std::strlen(credential));
    config.hidden = hidden;
    return config;
}

void checkProfile(
    const blinker::WifiCredentialProfile& profile,
    uint32_t generation,
    const char* ssid,
    const char* credential,
    blinker::WifiAuthentication authentication,
    bool hidden = false) {
    CHECK_EQ(generation, profile.generation);
    CHECK_EQ(authentication, profile.authentication);
    CHECK_EQ(hidden, profile.hidden());
    CHECK_EQ(std::strlen(ssid), static_cast<size_t>(profile.ssidLength));
    CHECK_EQ(
        std::strlen(credential),
        static_cast<size_t>(profile.credentialLength));
    CHECK_TRUE(std::memcmp(profile.ssid, ssid, profile.ssidLength) == 0);
    CHECK_TRUE(std::memcmp(
        profile.credential,
        credential,
        profile.credentialLength) == 0);
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

void testProtectionAndRecordValidation() {
    FaultAtomicStore plainSelector(blinker::StorageProtection::PlainFlash);
    FaultAtomicStore plain0(blinker::StorageProtection::PlainFlash);
    FaultAtomicStore plain1(blinker::StorageProtection::PlainFlash);
    blinker::WifiCredentialStore rejected(plainSelector, plain0, plain1);
    CHECK_EQ(blinker::ErrorCode::NotConfigured, rejected.begin().code());

    blinker::WifiCredentialStoreConfig development;
    development.allowPlainFlash = true;
    blinker::WifiCredentialStore allowed(
        plainSelector,
        plain0,
        plain1,
        development);
    CHECK_TRUE(allowed.begin());

    blinker::WifiCredentialProfile profile;
    profile.generation = 7U;
    profile.authentication = blinker::WifiAuthentication::Wpa2Personal;
    profile.ssidLength = 4U;
    profile.credentialLength = 8U;
    profile.setHidden(true);
    profile.setConfirmed(true);
    std::memcpy(profile.ssid, "ssid", 4U);
    std::memcpy(profile.credential, "password", 8U);
    uint8_t encoded[blinker::WifiCredentialStore::serializedProfileSize] = {};
    CHECK_TRUE(blinker::WifiCredentialStore::encodeProfile(
        profile,
        blinker::MutableByteSpan(encoded, sizeof(encoded))));
    CHECK_TRUE(std::memcmp(encoded, "BWF2", 4U) == 0);
    blinker::WifiCredentialProfile decoded;
    CHECK_TRUE(blinker::WifiCredentialStore::decodeProfile(
        blinker::ByteView(encoded, sizeof(encoded)),
        decoded));
    checkProfile(
        decoded,
        7U,
        "ssid",
        "password",
        blinker::WifiAuthentication::Wpa2Personal,
        true);
    CHECK_TRUE(decoded.confirmed());

    encoded[9] = 1U;
    writeU32(
        encoded + 112U,
        blinker::computeCrc32(blinker::ByteView(encoded, 112U)));
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        blinker::WifiCredentialStore::decodeProfile(
            blinker::ByteView(encoded, sizeof(encoded)),
            decoded).code());
    encoded[9] = 0U;
    encoded[20] = 1U;
    writeU32(
        encoded + 112U,
        blinker::computeCrc32(blinker::ByteView(encoded, 112U)));
    CHECK_EQ(
        blinker::ErrorCode::InvalidEncoding,
        blinker::WifiCredentialStore::decodeProfile(
            blinker::ByteView(encoded, sizeof(encoded)),
            decoded).code());
    blinker::clearWifiCredentialProfile(profile);
    blinker::clearWifiCredentialProfile(decoded);
}

void testCommitIdempotencyRollbackAndClear() {
    FaultAtomicStore selector;
    FaultAtomicStore slot0;
    FaultAtomicStore slot1;
    blinker::WifiCredentialStore store(selector, slot0, slot1);
    CHECK_TRUE(store.begin());
    blinker::WifiCredentialProfile profile;
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(profile).code());

    const blinker::WifiNetworkConfig first = network(
        "home",
        "password",
        blinker::WifiAuthentication::Wpa2Personal);
    CHECK_TRUE(store.commit(first));
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        1U,
        "home",
        "password",
        blinker::WifiAuthentication::Wpa2Personal);
    CHECK_TRUE(!profile.confirmed());
    blinker::clearWifiCredentialProfile(profile);
    CHECK_TRUE(store.confirmActive());
    CHECK_TRUE(store.load(profile));
    CHECK_TRUE(profile.confirmed());
    blinker::clearWifiCredentialProfile(profile);
    const uint32_t selectorWrites = selector.replaceCalls;
    const uint32_t slotWrites = slot0.replaceCalls;
    CHECK_TRUE(store.commit(first));
    CHECK_EQ(selectorWrites, selector.replaceCalls);
    CHECK_EQ(slotWrites, slot0.replaceCalls);

    const blinker::WifiNetworkConfig second = network(
        "office",
        "new-password",
        blinker::WifiAuthentication::Wpa3Personal,
        true);
    CHECK_TRUE(store.commit(second));
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        2U,
        "office",
        "new-password",
        blinker::WifiAuthentication::Wpa3Personal,
        true);
    CHECK_TRUE(!profile.confirmed());
    blinker::clearWifiCredentialProfile(profile);
    CHECK_TRUE(store.loadPrevious(profile));
    checkProfile(
        profile,
        1U,
        "home",
        "password",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);
    CHECK_TRUE(store.rollbackToPrevious());
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        1U,
        "home",
        "password",
        blinker::WifiAuthentication::Wpa2Personal);
    CHECK_TRUE(profile.confirmed());
    blinker::clearWifiCredentialProfile(profile);

    slot0.clearError = blinker::ErrorCode::InternalError;
    CHECK_EQ(blinker::ErrorCode::InternalError, store.clear().code());
    CHECK_EQ(blinker::ErrorCode::NotFound, store.load(profile).code());
    CHECK_TRUE(store.clear());
    store.end();
    blinker::WifiCredentialStore reopened(selector, slot0, slot1);
    CHECK_TRUE(reopened.begin());
    CHECK_EQ(blinker::ErrorCode::NotFound, reopened.load(profile).code());
}

void testFailureKeepsOldAndAmbiguousSelectorRecovers() {
    FaultAtomicStore selector;
    FaultAtomicStore slot0;
    FaultAtomicStore slot1;
    blinker::WifiCredentialStore store(selector, slot0, slot1);
    CHECK_TRUE(store.begin());
    const blinker::WifiNetworkConfig oldConfig = network(
        "old",
        "old-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    const blinker::WifiNetworkConfig newConfig = network(
        "new",
        "new-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    CHECK_TRUE(store.commit(oldConfig));
    CHECK_TRUE(store.confirmActive());

    slot1.replaceError = blinker::ErrorCode::CapacityExceeded;
    slot1.applyReplaceOnFailure = true;
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.commit(newConfig).code());
    blinker::WifiCredentialProfile profile;
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        1U,
        "old",
        "old-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);

    selector.replaceError = blinker::ErrorCode::InternalError;
    selector.applyReplaceOnFailure = false;
    CHECK_EQ(
        blinker::ErrorCode::InternalError,
        store.commit(newConfig).code());
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        1U,
        "old",
        "old-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);

    selector.replaceError = blinker::ErrorCode::InternalError;
    selector.applyReplaceOnFailure = true;
    CHECK_EQ(
        blinker::ErrorCode::InternalError,
        store.commit(newConfig).code());
    CHECK_TRUE(store.load(profile));
    checkProfile(
        profile,
        1U,
        "old",
        "old-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);

    store.end();
    blinker::WifiCredentialStore afterReset(selector, slot0, slot1);
    CHECK_TRUE(afterReset.begin());
    CHECK_TRUE(afterReset.load(profile));
    checkProfile(
        profile,
        2U,
        "new",
        "new-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);
    CHECK_TRUE(afterReset.loadPrevious(profile));
    checkProfile(
        profile,
        1U,
        "old",
        "old-pass",
        blinker::WifiAuthentication::Wpa2Personal);
    blinker::clearWifiCredentialProfile(profile);
}

} // namespace

int main() {
    testProtectionAndRecordValidation();
    testCommitIdempotencyRollbackAndClear();
    testFailureKeepsOldAndAmbiguousSelectorRecovers();
    std::cout << "WiFi credential journal tests passed; store="
              << sizeof(blinker::WifiCredentialStore)
              << " B; profile=" << sizeof(blinker::WifiCredentialProfile)
              << " B" << std::endl;
    return 0;
}
