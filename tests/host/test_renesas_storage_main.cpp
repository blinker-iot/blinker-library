#include <cstdlib>
#include <cstring>
#include <iostream>

#include "UnoR4Storage.h"

namespace {

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::cerr << "UNO storage check failed at line " << __LINE__   \
                      << ": " #condition << std::endl;                     \
            std::exit(1);                                                    \
        }                                                                    \
    } while (false)

void expect(blinker::IAtomicBlobStore& store,
            const uint8_t* expected,
            size_t expectedSize) {
    uint8_t output[blinker::DeviceAccessStore::serializedSize] = {};
    size_t written = 0U;
    CHECK(store.load(
        blinker::MutableByteSpan(output, sizeof(output)), written));
    CHECK(written == expectedSize);
    CHECK(std::memcmp(output, expected, expectedSize) == 0);
}

void testLifecycleCapacityAndIsolation() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    uint8_t output[blinker::DeviceAccessStore::serializedSize + 1U] = {};
    size_t written = 99U;
    CHECK(bank.deviceAccessBlob()
              .load(blinker::MutableByteSpan(output, sizeof(output)), written)
              .code() == blinker::ErrorCode::NotConfigured);
    CHECK(written == 0U);
    CHECK(bank.begin());
    CHECK(bank.begin());
    CHECK(fake_preferences::state().beginCalls == 1U);

    blinker::IAtomicBlobStore* stores[] = {
        &bank.wifiSelectorBlob(),
        &bank.wifiSlot0Blob(),
        &bank.wifiSlot1Blob(),
        &bank.deviceInstanceBlob(),
        &bank.deviceAccessBlob()};
    for (size_t index = 0U; index < 5U; ++index) {
        CHECK(stores[index]->protection() ==
              blinker::StorageProtection::PlainFlash);
    }

    const uint8_t selector[] = {1U, 2U};
    const uint8_t wifi0[] = {3U, 4U, 5U};
    const uint8_t wifi1[] = {6U, 7U};
    const uint8_t instance[] = {8U, 9U, 10U};
    uint8_t access[blinker::DeviceAccessStore::serializedSize] = {};
    for (size_t index = 0U; index < sizeof(access); ++index) {
        access[index] = static_cast<uint8_t>(index);
    }
    CHECK(bank.wifiSelectorBlob().replace(
        blinker::ByteView(selector, sizeof(selector))));
    CHECK(bank.wifiSlot0Blob().replace(
        blinker::ByteView(wifi0, sizeof(wifi0))));
    CHECK(bank.wifiSlot1Blob().replace(
        blinker::ByteView(wifi1, sizeof(wifi1))));
    CHECK(bank.deviceInstanceBlob().replace(
        blinker::ByteView(instance, sizeof(instance))));
    CHECK(bank.deviceAccessBlob().replace(
        blinker::ByteView(access, sizeof(access))));
    expect(bank.wifiSelectorBlob(), selector, sizeof(selector));
    expect(bank.wifiSlot0Blob(), wifi0, sizeof(wifi0));
    expect(bank.wifiSlot1Blob(), wifi1, sizeof(wifi1));
    expect(bank.deviceInstanceBlob(), instance, sizeof(instance));
    expect(bank.deviceAccessBlob(), access, sizeof(access));

    CHECK(bank.deviceAccessBlob()
              .replace(blinker::ByteView(output, sizeof(output)))
              .code() == blinker::ErrorCode::CapacityExceeded);
    CHECK(bank.deviceInstanceBlob()
              .replace(blinker::ByteView(
                  output,
                  blinker::DeviceInstanceIdStore::serializedSize + 1U))
              .code() == blinker::ErrorCode::CapacityExceeded);
    CHECK(bank.deviceAccessBlob().replace(blinker::ByteView()).code() ==
          blinker::ErrorCode::InvalidArgument);

    bank.end();
    bank.end();
    CHECK(fake_preferences::state().endCalls == 1U);

    blinker::RenesasUnoStorageConfig invalidConfig;
    invalidConfig.namespaceName = "invalid/name";
    blinker::RenesasUnoPreferencesBlobBank invalid(invalidConfig);
    CHECK(invalid.begin().code() == blinker::ErrorCode::InvalidArgument);
}

void testFaultRecoveryAndCorruption() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    blinker::IAtomicBlobStore& store = bank.deviceAccessBlob();
    const uint8_t first[] = {1U, 3U, 5U, 7U};
    const uint8_t second[] = {2U, 4U, 6U};
    CHECK(store.replace(blinker::ByteView(first, sizeof(first))));

    fake_preferences::failNext(fake_preferences::RejectNextPut);
    CHECK(store.replace(blinker::ByteView(second, sizeof(second))).code() ==
          blinker::ErrorCode::WouldBlock);
    expect(store, first, sizeof(first));
    fake_preferences::failNext(fake_preferences::CorruptNextPut);
    CHECK(store.replace(blinker::ByteView(second, sizeof(second))).code() ==
          blinker::ErrorCode::InternalError);
    expect(store, first, sizeof(first));
    fake_preferences::failNext(fake_preferences::ApplyButReportFailure);
    CHECK(store.replace(blinker::ByteView(second, sizeof(second))));
    expect(store, second, sizeof(second));

    CHECK(fake_preferences::corrupt("ac0"));
    expect(store, first, sizeof(first));
    CHECK(fake_preferences::corrupt("ac1"));
    uint8_t output[8] = {};
    size_t written = 0U;
    CHECK(store.load(blinker::MutableByteSpan(output, sizeof(output)), written)
              .code() == blinker::ErrorCode::InvalidEncoding);
}

void testPortableWifiJournalComposition() {
    fake_preferences::reset();
    blinker::RenesasUnoPreferencesBlobBank bank;
    CHECK(bank.begin());
    blinker::WifiCredentialStoreConfig config;
    config.allowPlainFlash = true;
    blinker::WifiCredentialStore wifi(
        bank.wifiSelectorBlob(),
        bank.wifiSlot0Blob(),
        bank.wifiSlot1Blob(),
        config);
    CHECK(wifi.begin());
    const uint8_t ssid[] = {'l', 'a', 'b'};
    const uint8_t password[] = {
        's', 'e', 'c', 'r', 'e', 't', '1', '2'};
    blinker::WifiNetworkConfig network;
    network.ssid = blinker::ByteView(ssid, sizeof(ssid));
    network.authentication = blinker::WifiAuthentication::Wpa2Personal;
    network.credential = blinker::ByteView(password, sizeof(password));
    CHECK(wifi.commit(network));
    CHECK(wifi.confirmActive());
    blinker::WifiCredentialProfile profile;
    CHECK(wifi.load(profile));
    CHECK(profile.confirmed());
    CHECK(std::memcmp(profile.ssid, ssid, sizeof(ssid)) == 0);
    blinker::clearWifiCredentialProfile(profile);
}

} // namespace

int main() {
    testLifecycleCapacityAndIsolation();
    testFaultRecoveryAndCorruption();
    testPortableWifiJournalComposition();
    std::cout << "UNO R4 Preferences storage adapter tests passed\n";
    return 0;
}
