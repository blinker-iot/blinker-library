// Reuse the established fake NVS/transport fixture, not a replacement product
// policy. Only this new main runs; no historical task/Presence suite is rerun.
#define BLINKER_TASK_CAPACITY 4
#define BLINKER_TASK_WINDOW_MS 100
#define BLINKER_TASK_WRITES 100
#define BLINKER_TASK_MIN_START_MS 10
#define BLINKER_TASK_MAX_COMMIT_MS 100
#define main bc2ExistingTaskProductMain
#define nvs_open bc2OriginalNvsOpen
#define nvs_open_from_partition bc2OriginalNvsOpenPartition
#define nvs_get_blob bc2OriginalNvsGetBlob
#include "test_task_product_main.cpp"
#undef nvs_get_blob
#undef nvs_open_from_partition
#undef nvs_open
#undef main

#include "BlinkerV2/arduino/platform/esp32/Esp32ProvisionedTaskExtension.h"
#include <new>

// External crypto adapter only. This transaction performs no signature work;
// reject all such calls rather than substituting authorization decisions.
namespace blinker {
Result Esp32MbedTlsCryptoProvider::x25519PublicKey(ByteView, MutableByteSpan) {
    return Result::failure(ErrorCode::UnsupportedFeature);
}
Result Esp32MbedTlsCryptoProvider::x25519(ByteView, ByteView, MutableByteSpan) {
    return Result::failure(ErrorCode::UnsupportedFeature);
}
Result Esp32MbedTlsCryptoProvider::aes256GcmEncrypt(ByteView, ByteView, ByteView,
    ByteView, MutableByteSpan, size_t& written) {
    written = 0U; return Result::failure(ErrorCode::UnsupportedFeature);
}
Result Esp32MbedTlsCryptoProvider::aes256GcmDecrypt(ByteView, ByteView, ByteView,
    ByteView, MutableByteSpan, size_t& written) {
    written = 0U; return Result::failure(ErrorCode::UnsupportedFeature);
}
Result Esp32MbedTlsCryptoProvider::verifyDigest(ByteView, ByteView, ByteView) {
    return Result::failure(ErrorCode::UnsupportedFeature);
}
}

namespace {
unsigned bc2Opens = 0U, bc2SnapshotReads = 0U, bc2RootReads = 0U;
esp_err_t bc2SnapshotError = ESP_OK, bc2RootError = ESP_OK;
std::function<void()> bc2DuringSnapshotRead;

struct Bc2Mqtt : IMqttClient {
    Result connect(const MqttConnectOptions&) override { return Result::failure(ErrorCode::NotConfigured); }
    void disconnect() override {}
    bool connected() const override { return false; }
    Result subscribe(StringView, uint8_t) override { return Result::failure(ErrorCode::NotConnected); }
    Result publish(StringView, ByteView, const MqttPublishOptions&) override { return Result::failure(ErrorCode::NotConnected); }
    void poll(uint32_t) override {}
    void setMessageHandler(MqttMessageHandler, void*) override {}
};
struct Bc2Cloud : IFrameTransport {
    bool available = false;
    Result start() override { return Result::success(); }
    void stop() override {}
    void poll(uint32_t) override {}
    void poll(uint32_t, bool (*)(void*), void*) {}
    TransportState state() const override { return TransportState::Stopped; }
    ErrorCode lastError() const { return ErrorCode::Ok; }
    TransportCapabilities capabilities() const override {
        TransportCapabilities value;
        value.maxFrameSize = 512U;
        value.features = TransportFeatureCloud | TransportFeatureAuthenticated;
        return value;
    }
    Result send(ByteView, const SendTarget&) override { return Result::failure(ErrorCode::NotConnected); }
    void setReceiver(FrameReceiver, void*) override {}
    void setSessionHandlers(FrameSessionHandler, FrameSessionHandler, void*) override {}
    void setNetworkAvailable(bool value) { available = value; }
    bool networkAvailable() const { return available; }
};
struct Bc2Platform {
    DeviceAccessStore& access;
    explicit Bc2Platform(DeviceAccessStore& value) : access(value) {}
    DeviceAccessStore& deviceKeys() { return access; }
};
struct Bc2Stack {
    typedef Bc2Cloud CloudTransport;
    Fixture& f;
    Bc2Platform hardware;
    DeviceInstanceId instance;
    Bc2Mqtt mqtt;
    MqttFrameTransport frames;
    Bc2Cloud cloud;
    Bc2Stack(Fixture& fixture, DeviceAccessStore& access)
        : f(fixture), hardware(access), instance(), mqtt(), frames(mqtt, f.tick), cloud() {
        for (size_t i = 0U; i < sizeof(instance.bytes); ++i) instance.bytes[i] = static_cast<uint8_t>(i + 1U);
    }
    Bc2Platform& platform() { return hardware; }
    IDeviceKeySource& deviceKeySource() { return hardware.access; }
    const DeviceInstanceId& deviceInstance() const { return instance; }
    IRandom& random() { return f.random; }
    IClock& clock() { return f.tick; }
    MqttFrameTransport& mqttTransport() { return frames; }
    Bc2Cloud& cloudTransport() { return cloud; }
};
typedef blinker::integration::official_detail::Esp32ProvisionedTaskExtension<Bc2Stack, 4U> Bc2Extension;

// configureTaskProperties deliberately owns one static product target per
// property signature. Reconstruct the ONE host product graph at its fixed
// address between cases, without replacing its real configuration decision.
Bc2Extension& bc2ConstructExtension(Bc2Stack& stack, IProductLifecycle& lifecycle) {
    alignas(Bc2Extension) static unsigned char storage[sizeof(Bc2Extension)];
    return *new (storage) Bc2Extension(stack, lifecycle);
}
Product& bc2ConstructProduct(Device& device, IProductLifecycle& lifecycle) {
    alignas(Product) static unsigned char storage[sizeof(Product)];
    return *new (storage) Product(device, lifecycle);
}

DeviceKey bc2Key() { DeviceKey key; std::memset(key.bytes, 0x55, sizeof(key.bytes)); return key; }
void bc2Reset() {
    nvs = Nvs(); bc2Opens = 0U; bc2SnapshotReads = 0U; bc2RootReads = 0U;
    bc2SnapshotError = ESP_OK; bc2RootError = ESP_OK; bc2DuringSnapshotRead = nullptr;
}
void bc2SeedTask(bool retired, bool corrupt = false) {
    TaskSnapshotHeader header; header.authority = testAuthority();
    if (retired) { header.retired = true; header.tableRevision = 1U; header.requestId[0] = 1U; header.requestDigest[0] = 2U; }
    Bytes encoded(TaskSnapshotCodec::encodedSize(0U)); size_t written = 0U;
    CHECK(TaskSnapshotCodec::encode(header, nullptr, 0U, MutableByteSpan(encoded.data(), encoded.size()), written));
    if (corrupt) encoded[0] ^= 1U;
    nvs.durable["nvs/bl_v2_tasks/snapshot"] = encoded;
    nvs.working = nvs.durable;
}
struct Bc2Fixture {
    Fixture f;
    Esp32NvsAtomicBlobStore root;
    DeviceAccessStore access;
    Bc2Stack stack;
    Bc2Extension& extension;
    Product& product;
    Bc2Fixture() : f(false), root(TaskProductFixture::rootConfig()), access(root), stack(f, access),
        extension(bc2ConstructExtension(stack, f.lifecycle)),
        product(bc2ConstructProduct(f.device, extension.lifecycle(f.lifecycle))) {
        CHECK(root.begin()); CHECK(access.bootstrapAccess(1U, ip4Admin()));
        CHECK(extension.configure(f.power));
        CHECK(product.begin(&f.time)); // Provisioning: no TaskStore open/Engine begin.
        CHECK(bc2Opens == 1U && bc2SnapshotReads == 0U);
    }
    ~Bc2Fixture() { product.~Product(); extension.~Bc2Extension(); }
    Result install(const ControllerCredential& admin = ip4Admin(), uint32_t epoch = 1U) {
        return extension.installInitialCloudKeyVerified(bc2Key(), epoch, admin);
    }
    void noTaskWrite() {
        CHECK(nvs.working.count("nvs/bl_v2_tasks/snapshot") == nvs.durable.count("nvs/bl_v2_tasks/snapshot"));
        if (nvs.working.count("nvs/bl_v2_tasks/snapshot"))
            CHECK(nvs.working["nvs/bl_v2_tasks/snapshot"] == nvs.durable["nvs/bl_v2_tasks/snapshot"]);
        CHECK(nvs.erases == 0U && f.calls == 0U);
    }
};

void bc2ProductOwnsProvisioningTransactionAndReusesTheSameTaskHandle() {
    bc2Reset(); Bc2Fixture t;
    const unsigned writes = nvs.sets, commits = nvs.commits;
    CHECK(t.install());
    CHECK(bc2Opens == 2U && bc2SnapshotReads == 1U);
    CHECK(nvs.sets == writes + 1U && nvs.commits == commits + 1U);
    DeviceKey actual; CHECK(t.access.load(actual) && sameDeviceKey(actual, bc2Key()));
    ControllerCredential admin; CHECK(t.access.loadActive(ip4Admin().id(), ControllerCredentialDomain::PlatformAccess, 1U, admin));
    CHECK(sameControllerCredential(admin, ip4Admin())); clearControllerCredential(admin); clearDeviceKey(actual);
    CHECK(t.extension.quiescent());
    CHECK(t.extension.beginCooperativeInitialCloudKeyChange().code() == ErrorCode::NotConfigured);
    CHECK(t.access.installInitialCloudKeyVerified(bc2Key(), 1U, ip4Admin()).code() == ErrorCode::NotConfigured);
    t.extension.active(); t.product.run(); // Original Engine begin uses the one already-open TaskStore.
    CHECK(bc2Opens == 2U && bc2SnapshotReads == 2U);
    CHECK(t.extension.taskError() == ErrorCode::Ok);
    t.noTaskWrite();
}

void bc2ObservationCannotBorrowCommitOrReenterLifecycleEvenWhenInactive() {
    bc2Reset(); Bc2Fixture t;
    const unsigned polls = t.f.lifecycle.polls, writes = nvs.sets;
    bc2DuringSnapshotRead = [&]() {
        CHECK(!t.extension.enterTaskOwner());
        CHECK(t.install().code() == ErrorCode::WouldBlock);
        CHECK(t.extension.beginCooperativeInitialCloudKeyChange().code() == ErrorCode::NotConfigured);
        CHECK(t.access.installInitialCloudKeyVerified(bc2Key(), 1U, ip4Admin()).code() == ErrorCode::NotConfigured);
        t.product.run();
        CHECK(t.f.lifecycle.polls == polls && nvs.sets == writes);
        CHECK(bc2Opens == 2U && bc2SnapshotReads == 1U);
    };
    CHECK(t.install());
    CHECK(!bc2DuringSnapshotRead && nvs.sets == writes + 1U);
    CHECK(t.extension.quiescent()); t.noTaskWrite();
}

void bc2BoundZeroTableAndRetirementBothBlockWithoutClearingScope() {
    for (bool retired : {false, true}) {
        bc2Reset(); bc2SeedTask(retired); Bc2Fixture t;
        const auto original = nvs.durable; const unsigned writes = nvs.sets;
        CHECK(t.install().code() == ErrorCode::StateConflict);
        CHECK(nvs.durable == original && nvs.sets == writes && bc2SnapshotReads == 2U);
        CHECK(t.install().code() == ErrorCode::StateConflict && bc2SnapshotReads == 4U);
        t.product.end(); t.extension.active(); CHECK(t.product.begin()); t.product.run();
        CHECK(nvs.durable == original && bc2SnapshotReads == 6U && t.extension.quiescent());
        t.noTaskWrite();
    }
}

void bc2TaskReadAndOpenFailuresNeverReopenOrUnlock() {
    for (unsigned mode = 0U; mode < 3U; ++mode) {
        bc2Reset(); if (mode == 0U) bc2SeedTask(false, true);
        Bc2Fixture t;
        if (mode == 1U) bc2SnapshotError = ESP_ERR_NVS_INVALID_HANDLE;
        if (mode == 2U) nvs.openError = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
        const auto original = nvs.durable; const unsigned writes = nvs.sets;
        CHECK(!t.install());
        const unsigned opens = bc2Opens, reads = bc2SnapshotReads;
        bc2SnapshotError = ESP_OK; nvs.openError = ESP_OK;
        CHECK(t.install().code() == ErrorCode::StateConflict);
        t.product.end(); t.extension.provisioning(); t.extension.active(); CHECK(t.product.begin()); t.product.run();
        CHECK(bc2Opens == opens && bc2SnapshotReads == reads && nvs.sets == writes && nvs.durable == original);
        CHECK(t.extension.quiescent()); t.noTaskWrite();
    }
}

void bc2EngineInitializedCannotBorrowItsRecords() {
    bc2Reset(); Bc2Fixture t;
    t.extension.active(); t.product.run();
    CHECK(bc2Opens == 2U && bc2SnapshotReads == 1U);
    const unsigned writes = nvs.sets, reads = bc2SnapshotReads;
    CHECK(t.install().code() == ErrorCode::NotConfigured);
    CHECK(nvs.sets == writes && bc2SnapshotReads == reads);
    CHECK(t.extension.quiescent()); t.noTaskWrite();
}

void bc2WrongObserverOrOwnerNeverOpensTheTaskNamespace() {
    bc2Reset(); Bc2Fixture t;
    t.access.setChangeObserver(nullptr);
    CHECK(t.install().code() == ErrorCode::NotConfigured && bc2Opens == 1U && bc2SnapshotReads == 0U);
    t.access.setChangeObserver(&t.extension);
    CHECK(t.extension.enterMaintenance());
    CHECK(t.install().code() == ErrorCode::WouldBlock && bc2Opens == 1U && bc2SnapshotReads == 0U);
    t.extension.leaveMaintenance();
    CHECK(t.install()); CHECK(t.extension.quiescent()); t.noTaskWrite();
}

void bc2ExpectedAdminAndEpochAreActuallyChecked() {
    for (unsigned mode = 0U; mode < 4U; ++mode) {
        bc2Reset(); Bc2Fixture t; ControllerCredential expected = ip4Admin(); uint32_t epoch = 1U;
        if (mode == 0U) epoch = 2U;
        if (mode == 1U) expected.controllerId[0] ^= 1U;
        if (mode == 2U) expected.secret[0] ^= 1U;
        if (mode == 3U) expected.credentialVersion++;
        const unsigned writes = nvs.sets;
        CHECK(!t.install(expected, epoch));
        DeviceKey missing; CHECK(t.access.load(missing).code() == ErrorCode::NotFound);
        CHECK(nvs.sets == writes && t.extension.quiescent());
        CHECK(t.install()); // Explicit NEW trusted operation after known no-write CAS refusal.
        CHECK(nvs.sets == writes + 1U && t.extension.quiescent()); t.noTaskWrite();
    }
}

void bc2UnknownAccessWriteRemainsPoisonedEvenIfTheHandleShowsTheKey() {
    for (bool durable : {false, true}) {
        bc2Reset(); Bc2Fixture t;
        nvs.failCommit = nvs.commits + 1U; nvs.commitThenFail = durable;
        CHECK(!t.install());
        DeviceKey attempted; CHECK(t.access.load(attempted) && sameDeviceKey(attempted, bc2Key()));
        clearDeviceKey(attempted);
        const unsigned writes = nvs.sets, opens = bc2Opens, reads = bc2SnapshotReads, rootReads = bc2RootReads;
        CHECK(t.install().code() == ErrorCode::StateConflict);
        CHECK(t.access.installInitialCloudKeyVerified(bc2Key(), 1U, ip4Admin()).code() == ErrorCode::NotConfigured);
        t.product.end(); t.extension.provisioning(); t.extension.active(); CHECK(t.product.begin()); t.product.run();
        CHECK(nvs.sets == writes && bc2Opens == opens && bc2SnapshotReads == reads && bc2RootReads == rootReads);
        CHECK(t.extension.quiescent()); t.noTaskWrite();
    }
}

void bc2PreAdmissionRootReadFailureIsNotAnAdmittedUnknownWrite() {
    bc2Reset(); Bc2Fixture t;
    const unsigned writes = nvs.sets, commits = nvs.commits, rootReads = bc2RootReads;
    bc2RootError = ESP_ERR_NVS_INVALID_HANDLE;
    CHECK(t.install().code() == ErrorCode::NotConfigured); // Preserve the original read error.
    CHECK(bc2RootReads == rootReads + 1U); // No implicit root retry.
    CHECK(bc2Opens == 1U && bc2SnapshotReads == 0U && nvs.sets == writes && nvs.commits == commits);
    CHECK(t.extension.quiescent());
    bc2RootError = ESP_OK;
    // A separate explicit trusted call rechecks the actual root; this is not
    // an automatic retry, UNKNOWN readback, or reopening the persistence port.
    CHECK(t.install());
    CHECK(bc2Opens == 2U && bc2SnapshotReads == 1U && nvs.sets == writes + 1U && nvs.commits == commits + 1U);
    CHECK(t.extension.quiescent()); t.noTaskWrite();
}
} // namespace

extern "C" esp_err_t nvs_open_from_partition(const char* partition, const char* name,
    nvs_open_mode_t mode, nvs_handle_t* out) {
    ++bc2Opens; return bc2OriginalNvsOpenPartition(partition, name, mode, out);
}
extern "C" esp_err_t nvs_open(const char* name, nvs_open_mode_t mode, nvs_handle_t* out) {
    return nvs_open_from_partition("nvs", name, mode, out);
}
extern "C" esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* output, size_t* length) {
    const auto found = nvs.handles.find(handle);
    if (found != nvs.handles.end() && found->second == "nvs/bl_v2_access" && std::strcmp(key, "record") == 0) {
        ++bc2RootReads;
        if (bc2RootError != ESP_OK) return bc2RootError;
    }
    if (found != nvs.handles.end() && found->second == "nvs/bl_v2_tasks" && std::strcmp(key, "snapshot") == 0) {
        ++bc2SnapshotReads;
        if (bc2DuringSnapshotRead) { const auto callback = bc2DuringSnapshotRead; bc2DuringSnapshotRead = nullptr; callback(); }
        if (bc2SnapshotError != ESP_OK) return bc2SnapshotError;
    }
    return bc2OriginalNvsGetBlob(handle, key, output, length);
}

#if defined(BLINKER_BC2A_TEST_MAIN)
int BLINKER_BC2A_TEST_MAIN() {
#else
int main() {
#endif
    bc2ProductOwnsProvisioningTransactionAndReusesTheSameTaskHandle();
    bc2ObservationCannotBorrowCommitOrReenterLifecycleEvenWhenInactive();
    bc2BoundZeroTableAndRetirementBothBlockWithoutClearingScope();
    bc2TaskReadAndOpenFailuresNeverReopenOrUnlock();
    bc2EngineInitializedCannotBorrowItsRecords();
    bc2WrongObserverOrOwnerNeverOpensTheTaskNamespace();
    bc2ExpectedAdminAndEpochAreActuallyChecked();
    bc2UnknownAccessWriteRemainsPoisonedEvenIfTheHandleShowsTheKey();
    bc2PreAdmissionRootReadFailureIsNotAnAdmittedUnknownWrite();
    std::cout << "Initial Cloud Key product: 9 finite host groups passed (fake NVS/transport, no network authorization)\n";
    return 0;
}
