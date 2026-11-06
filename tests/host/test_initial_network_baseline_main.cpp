#include <BlinkerV2/provisioning/WifiCredentialInitialNetworkBaseline.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            std::cerr << "Initial network baseline failed at " << __LINE__    \
                      << ": " << #condition << std::endl;                    \
            std::exit(1);                                                      \
        }                                                                      \
    } while (false)

class ObservedBlob : public blinker::IAtomicBlobStore {
public:
    ObservedBlob()
        : bytes(), size(0U), present(false), readError(blinker::ErrorCode::Ok),
          absentWritten(0U), readCapacity(0U), reads(0U), replaces(0U),
          clears(0U), encrypted(true), failReplaceAfterApply(false) {}

    blinker::Result load(blinker::MutableByteSpan output,
                         size_t& written) override {
        ++reads;
        readCapacity = output.size;
        written = 0U;
        if (readError != blinker::ErrorCode::Ok)
            return blinker::Result::failure(readError);
        if (!present) {
            written = absentWritten;
            return blinker::Result::failure(blinker::ErrorCode::NotFound);
        }
        if (size > output.size)
            return blinker::Result::failure(blinker::ErrorCode::BufferTooSmall);
        if (size != 0U) std::memcpy(output.data, bytes, size);
        written = size;
        return blinker::Result::success();
    }

    blinker::Result replace(blinker::ByteView input) override {
        ++replaces;
        CHECK(input.size <= sizeof(bytes));
        std::memset(bytes, 0, sizeof(bytes));
        std::memcpy(bytes, input.data, input.size);
        size = input.size;
        present = true;
        return failReplaceAfterApply
            ? blinker::Result::failure(blinker::ErrorCode::InternalError)
            : blinker::Result::success();
    }

    blinker::Result clear() override {
        ++clears;
        std::memset(bytes, 0, sizeof(bytes));
        size = 0U;
        present = false;
        return blinker::Result::success();
    }

    blinker::StorageProtection protection() const override {
        return encrypted ? blinker::StorageProtection::EncryptedFlash
                         : blinker::StorageProtection::PlainFlash;
    }

    void resetCounts() { reads = replaces = clears = 0U; }

    uint8_t bytes[128];
    size_t size;
    bool present;
    blinker::ErrorCode readError;
    size_t absentWritten;
    size_t readCapacity;
    unsigned reads;
    unsigned replaces;
    unsigned clears;
    bool encrypted;
    bool failReplaceAfterApply;
};

struct Fixture {
    ObservedBlob selector, slot0, slot1;
    blinker::WifiCredentialStore store;
    blinker::WifiCredentialInitialNetworkBaseline baseline;

    Fixture() : selector(), slot0(), slot1(), store(selector, slot0, slot1),
                baseline(store) {}

    void resetCounts() {
        selector.resetCounts(); slot0.resetCounts(); slot1.resetCounts();
    }

    void checkReadOnly(unsigned reads = 1U) const {
        CHECK(selector.reads == reads && slot0.reads == reads && slot1.reads == reads);
        CHECK(selector.replaces == 0U && slot0.replaces == 0U && slot1.replaces == 0U);
        CHECK(selector.clears == 0U && slot0.clears == 0U && slot1.clears == 0U);
        CHECK(selector.readCapacity == blinker::WifiCredentialStore::serializedSelectorSize);
        CHECK(slot0.readCapacity == blinker::WifiCredentialStore::serializedProfileSize);
        CHECK(slot1.readCapacity == blinker::WifiCredentialStore::serializedProfileSize);
    }
};

blinker::WifiNetworkConfig network() {
    blinker::WifiNetworkConfig config;
    config.ssid = blinker::ByteView(reinterpret_cast<const uint8_t*>("test"), 4U);
    config.authentication = blinker::WifiAuthentication::Wpa2Personal;
    config.credential = blinker::ByteView(
        reinterpret_cast<const uint8_t*>("password"), 8U);
    return config;
}

void testRepeatableEmptyWithoutBegin() {
    Fixture f;
    blinker::IInitialCloudNetworkBaseline& baseline = f.baseline;
    CHECK(baseline.validateEmptyNetwork());
    CHECK(baseline.validateEmptyNetwork());
    f.checkReadOnly(2U);
    CHECK(!f.store.ready() && !f.store.hasActiveProfile());
    CHECK(f.store.activeGeneration() == 0U);
}

void testAnyRecordAndOrphanRejects() {
    for (unsigned index = 0U; index < 3U; ++index) {
        Fixture f;
        ObservedBlob* const blobs[] = {&f.selector, &f.slot0, &f.slot1};
        // Presence alone is disqualifying, including malformed/zero-size data.
        blobs[index]->present = true;
        blobs[index]->size = index == 0U ? 0U : 12U;
        std::memset(blobs[index]->bytes, 0xA5, sizeof(blobs[index]->bytes));
        CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
        f.checkReadOnly();
        CHECK(blobs[index]->present);
        CHECK(blobs[index]->bytes[0] == 0xA5U);
        CHECK(!f.store.ready());
    }
    Fixture f;
    CHECK(f.store.begin()); // Selector absence does not inspect orphan slots.
    f.slot1.present = true;
    f.slot1.size = 5U;
    f.resetCounts();
    CHECK(!f.store.hasActiveProfile());
    CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
    f.checkReadOnly();
    CHECK(f.store.ready() && !f.store.hasActiveProfile());
}

void testErrorsNeverBecomeEmptyAndAllReadsRun() {
    const blinker::ErrorCode errors[] = {
        blinker::ErrorCode::WouldBlock, blinker::ErrorCode::InternalError,
        blinker::ErrorCode::BufferTooSmall, blinker::ErrorCode::InvalidEncoding,
        blinker::ErrorCode::NotConfigured
    };
    for (size_t error = 0U; error < sizeof(errors) / sizeof(errors[0]); ++error) {
        for (unsigned index = 0U; index < 3U; ++index) {
            Fixture f;
            ObservedBlob* const blobs[] = {&f.selector, &f.slot0, &f.slot1};
            blobs[index]->readError = errors[error];
            CHECK(f.baseline.validateEmptyNetwork().code() == errors[error]);
            f.checkReadOnly();
        }
    }
    Fixture f;
    f.selector.absentWritten = 1U;
    CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::InvalidEncoding);
    f.checkReadOnly();
}

void testNoActiveSelectorAndActiveCacheRemainUnchanged() {
    Fixture cleared;
    CHECK(cleared.store.begin());
    CHECK(cleared.store.clear()); // Valid no-active selector is still a record.
    cleared.resetCounts();
    CHECK(cleared.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
    cleared.checkReadOnly();
    CHECK(cleared.store.ready() && !cleared.store.hasActiveProfile());

    Fixture active;
    CHECK(active.store.begin());
    CHECK(active.store.commit(network()));
    uint8_t selector[sizeof(active.selector.bytes)];
    uint8_t slot[sizeof(active.slot0.bytes)];
    std::memcpy(selector, active.selector.bytes, sizeof(selector));
    std::memcpy(slot, active.slot0.bytes, sizeof(slot));
    active.resetCounts();
    CHECK(active.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
    active.checkReadOnly();
    CHECK(active.store.ready() && active.store.hasActiveProfile());
    CHECK(active.store.activeGeneration() == 1U);
    CHECK(std::memcmp(selector, active.selector.bytes, sizeof(selector)) == 0);
    CHECK(std::memcmp(slot, active.slot0.bytes, sizeof(slot)) == 0);
}

void testFreshCheckAndUncertainSlotCannotBeIgnored() {
    Fixture f;
    CHECK(f.baseline.validateEmptyNetwork());
    CHECK(f.store.begin());
    f.slot0.failReplaceAfterApply = true;
    CHECK(f.store.commit(network()).code() == blinker::ErrorCode::InternalError);
    CHECK(!f.store.hasActiveProfile() && !f.selector.present && f.slot0.present);
    f.resetCounts();
    CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
    CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::StateConflict);
    f.checkReadOnly(2U);
}

void testProtectionContractIsPreserved() {
    Fixture f;
    f.slot1.encrypted = false;
    CHECK(f.baseline.validateEmptyNetwork().code() == blinker::ErrorCode::NotConfigured);
    f.checkReadOnly();
    blinker::WifiCredentialStoreConfig development;
    development.allowPlainFlash = true;
    blinker::WifiCredentialStore store(f.selector, f.slot0, f.slot1, development);
    blinker::WifiCredentialInitialNetworkBaseline baseline(store);
    f.resetCounts();
    CHECK(baseline.validateEmptyNetwork());
    f.checkReadOnly();
    CHECK(!store.ready());
}

} // namespace

int main() {
    testRepeatableEmptyWithoutBegin();
    testAnyRecordAndOrphanRejects();
    testErrorsNeverBecomeEmptyAndAllReadsRun();
    testNoActiveSelectorAndActiveCacheRemainUnchanged();
    testFreshCheckAndUncertainSlotCannotBeIgnored();
    testProtectionContractIsPreserved();
    std::cout << "Initial network baseline tests passed" << std::endl;
    return 0;
}
