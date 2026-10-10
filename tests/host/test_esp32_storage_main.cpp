#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Esp32Storage.h"

namespace {

#define CHECK_TRUE(condition)                                                   \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << "ESP32 storage check failed at line " << __LINE__     \
                      << ": " #condition << std::endl;                         \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

#define CHECK_EQ(expected, actual)                                              \
    do {                                                                        \
        if (!((expected) == (actual))) {                                        \
            std::cerr << "ESP32 storage equality failed at line " << __LINE__  \
                      << std::endl;                                             \
            std::exit(1);                                                       \
        }                                                                       \
    } while (false)

struct FakeNvsState {
    enum : size_t { capacity = 512U };

    uint8_t committed[capacity];
    uint8_t working[capacity];
    size_t committedSize;
    size_t workingSize;
    bool committedPresent;
    bool workingPresent;
    bool opened;
    esp_err_t openResult;
    esp_err_t getResult;
    esp_err_t setResult;
    esp_err_t eraseResult;
    esp_err_t commitResult;
    uint32_t openCalls;
    uint32_t partitionOpenCalls;
    uint32_t closeCalls;
    uint32_t getCalls;
    uint32_t setCalls;
    uint32_t eraseCalls;
    uint32_t commitCalls;
    char openedNamespace[32];
    char openedPartition[32];
    char accessedKey[32];

    FakeNvsState() { reset(); }

    void reset() {
        std::memset(this, 0, sizeof(*this));
        openResult = ESP_OK;
        getResult = ESP_OK;
        setResult = ESP_OK;
        eraseResult = ESP_OK;
        commitResult = ESP_OK;
    }

    void powerLoss() {
        std::memcpy(working, committed, sizeof(working));
        workingSize = committedSize;
        workingPresent = committedPresent;
        opened = false;
    }
};

FakeNvsState gNvs;

void copyText(char* output, size_t capacity, const char* input) {
    if (capacity == 0U) return;
    const size_t length = input == nullptr ? 0U : std::strlen(input);
    const size_t copied = length < capacity - 1U ? length : capacity - 1U;
    if (copied > 0U) std::memcpy(output, input, copied);
    output[copied] = '\0';
}

bool acceptEncryption(const char*) {
    return true;
}

bool rejectEncryption(const char*) {
    return false;
}

void testDefaultLifecycleAndAtomicFailure() {
    gNvs.reset();
    blinker::Esp32NvsAtomicBlobStore store;
    uint8_t output[16] = {};
    size_t written = 99U;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        store.load(blinker::MutableByteSpan(output, sizeof(output)), written)
            .code());
    CHECK_EQ(static_cast<size_t>(0U), written);
    CHECK_TRUE(store.begin().ok());
    CHECK_TRUE(store.begin().ok());
    CHECK_EQ(static_cast<uint32_t>(1U), gNvs.openCalls);
    CHECK_TRUE(store.opened());
    CHECK_EQ(static_cast<size_t>(256U), store.maximumSize());
    CHECK_EQ(
        blinker::StorageProtection::PlainFlash,
        store.protection());
    CHECK_TRUE(std::strcmp(gNvs.openedNamespace, "bl_v2_blob") == 0);

    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.load(blinker::MutableByteSpan(output, sizeof(output)), written)
            .code());
    const uint8_t first[] = {1U, 2U, 3U, 4U};
    CHECK_TRUE(store.replace(blinker::ByteView(first, sizeof(first))).ok());
    CHECK_EQ(static_cast<uint32_t>(1U), gNvs.setCalls);
    CHECK_EQ(static_cast<uint32_t>(1U), gNvs.commitCalls);
    CHECK_TRUE(store.load(
        blinker::MutableByteSpan(output, sizeof(output)), written).ok());
    CHECK_EQ(sizeof(first), written);
    CHECK_TRUE(std::memcmp(first, output, sizeof(first)) == 0);
    CHECK_TRUE(std::strcmp(gNvs.accessedKey, "record") == 0);

    const uint8_t second[] = {9U, 8U, 7U};
    gNvs.commitResult = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.replace(blinker::ByteView(second, sizeof(second))).code());
    CHECK_TRUE(store.load(
        blinker::MutableByteSpan(output, sizeof(output)), written).ok());
    CHECK_EQ(sizeof(second), written);
    CHECK_TRUE(std::memcmp(second, output, sizeof(second)) == 0);

    store.end();
    gNvs.powerLoss();
    gNvs.commitResult = ESP_OK;
    CHECK_TRUE(store.begin().ok());
    CHECK_TRUE(store.load(
        blinker::MutableByteSpan(output, sizeof(output)), written).ok());
    CHECK_EQ(sizeof(first), written);
    CHECK_TRUE(std::memcmp(first, output, sizeof(first)) == 0);
    store.end();
    CHECK_EQ(static_cast<uint32_t>(2U), gNvs.closeCalls);
}

void testBoundsClearAndErrors() {
    gNvs.reset();
    blinker::Esp32NvsBlobStoreConfig config;
    config.maximumSize = 4U;
    config.encrypted = true;
    config.encryptionVerifier = acceptEncryption;
    blinker::Esp32NvsAtomicBlobStore store(config);
    CHECK_TRUE(store.begin().ok());
    CHECK_EQ(
        blinker::StorageProtection::EncryptedFlash,
        store.protection());

    const uint8_t tooLarge[] = {1U, 2U, 3U, 4U, 5U};
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.replace(blinker::ByteView(tooLarge, sizeof(tooLarge))).code());
    CHECK_EQ(static_cast<uint32_t>(0U), gNvs.setCalls);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        store.replace(blinker::ByteView()).code());

    const uint8_t exact[] = {5U, 6U, 7U, 8U};
    CHECK_TRUE(store.replace(blinker::ByteView(exact, sizeof(exact))).ok());
    uint8_t small[3] = {0xAAU, 0xAAU, 0xAAU};
    size_t written = 77U;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        store.load(blinker::MutableByteSpan(small, sizeof(small)), written)
            .code());
    CHECK_EQ(static_cast<size_t>(0U), written);
    CHECK_EQ(static_cast<uint8_t>(0xAAU), small[0]);
    CHECK_TRUE(store.clear().ok());
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.clear().code());
    CHECK_EQ(
        blinker::ErrorCode::NotFound,
        store.load(blinker::MutableByteSpan(small, sizeof(small)), written)
            .code());

    gNvs.setResult = ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    CHECK_EQ(
        blinker::ErrorCode::CapacityExceeded,
        store.replace(blinker::ByteView(exact, sizeof(exact))).code());
    gNvs.setResult = ESP_OK;
    gNvs.getResult = ESP_ERR_NVS_INVALID_LENGTH;
    CHECK_EQ(
        blinker::ErrorCode::BufferTooSmall,
        store.load(blinker::MutableByteSpan(small, sizeof(small)), written)
            .code());
}

void testPartitionAndConfigurationValidation() {
    gNvs.reset();
    blinker::Esp32NvsBlobStoreConfig partitionConfig;
    partitionConfig.namespaceName = "op_v2";
    partitionConfig.key = "record";
    partitionConfig.partitionLabel = "blinker_nvs";
    blinker::Esp32NvsAtomicBlobStore partitionStore(partitionConfig);
    CHECK_TRUE(partitionStore.begin().ok());
    CHECK_EQ(static_cast<uint32_t>(1U), gNvs.partitionOpenCalls);
    CHECK_TRUE(std::strcmp(gNvs.openedPartition, "blinker_nvs") == 0);
    CHECK_TRUE(std::strcmp(gNvs.openedNamespace, "op_v2") == 0);
    partitionStore.end();

    blinker::Esp32NvsBlobStoreConfig invalidName;
    invalidName.namespaceName = "name/is/invalid";
    blinker::Esp32NvsAtomicBlobStore invalidNameStore(invalidName);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        invalidNameStore.begin().code());

    blinker::Esp32NvsBlobStoreConfig invalidCapacity;
    invalidCapacity.maximumSize = 0U;
    blinker::Esp32NvsAtomicBlobStore invalidCapacityStore(invalidCapacity);
    CHECK_EQ(
        blinker::ErrorCode::InvalidArgument,
        invalidCapacityStore.begin().code());

    blinker::Esp32NvsBlobStoreConfig unattested;
    unattested.encrypted = true;
    blinker::Esp32NvsAtomicBlobStore unattestedStore(unattested);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        unattestedStore.begin().code());
    CHECK_EQ(
        blinker::StorageProtection::PlainFlash,
        unattestedStore.protection());
    CHECK_EQ(static_cast<uint32_t>(1U), gNvs.openCalls);

    blinker::Esp32NvsBlobStoreConfig rejected;
    rejected.encrypted = true;
    rejected.encryptionVerifier = rejectEncryption;
    blinker::Esp32NvsAtomicBlobStore rejectedStore(rejected);
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        rejectedStore.begin().code());

    gNvs.openResult = ESP_ERR_NVS_NOT_INITIALIZED;
    blinker::Esp32NvsAtomicBlobStore uninitializedStore;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        uninitializedStore.begin().code());
}

void testWifiCredentialSinkCompositionLifecycle() {
    gNvs.reset();
    blinker::Esp32NvsWifiCredentialSink defaultSink;
    CHECK_EQ(
        blinker::ErrorCode::NotConfigured,
        defaultSink.begin().code());
    CHECK_TRUE(!defaultSink.ready());
    CHECK_EQ(static_cast<uint32_t>(3U), gNvs.openCalls);
    CHECK_EQ(static_cast<uint32_t>(3U), gNvs.closeCalls);

    gNvs.reset();
    blinker::Esp32NvsWifiCredentialSinkConfig encryptedConfig;
    encryptedConfig.encrypted = true;
    encryptedConfig.encryptionVerifier = acceptEncryption;
    blinker::Esp32NvsWifiCredentialSink encryptedSink(encryptedConfig);
    CHECK_TRUE(encryptedSink.begin().ok());
    CHECK_TRUE(encryptedSink.ready());
    CHECK_TRUE(!encryptedSink.hasActiveProfile());
    CHECK_EQ(static_cast<uint32_t>(3U), gNvs.openCalls);
    CHECK_TRUE(std::strcmp(gNvs.openedNamespace, "bl_v2_net") == 0);
    encryptedSink.end();
    CHECK_EQ(static_cast<uint32_t>(3U), gNvs.closeCalls);

    gNvs.reset();
    blinker::Esp32NvsWifiCredentialSinkConfig developmentConfig;
    developmentConfig.allowPlainFlash = true;
    blinker::Esp32NvsWifiCredentialSink developmentSink(developmentConfig);
    CHECK_TRUE(developmentSink.begin().ok());
    CHECK_TRUE(developmentSink.ready());
    developmentSink.end();
}

} // namespace

extern "C" esp_err_t nvs_open(
    const char* namespaceName,
    nvs_open_mode_t mode,
    nvs_handle_t* handle) {
    (void)mode;
    ++gNvs.openCalls;
    copyText(gNvs.openedNamespace, sizeof(gNvs.openedNamespace), namespaceName);
    if (gNvs.openResult != ESP_OK) return gNvs.openResult;
    gNvs.opened = true;
    *handle = 1U;
    return ESP_OK;
}

extern "C" esp_err_t nvs_open_from_partition(
    const char* partition,
    const char* namespaceName,
    nvs_open_mode_t mode,
    nvs_handle_t* handle) {
    ++gNvs.partitionOpenCalls;
    copyText(gNvs.openedPartition, sizeof(gNvs.openedPartition), partition);
    return nvs_open(namespaceName, mode, handle);
}

extern "C" void nvs_close(nvs_handle_t handle) {
    (void)handle;
    ++gNvs.closeCalls;
    gNvs.opened = false;
}

extern "C" esp_err_t nvs_get_blob(
    nvs_handle_t handle,
    const char* key,
    void* output,
    size_t* length) {
    ++gNvs.getCalls;
    copyText(gNvs.accessedKey, sizeof(gNvs.accessedKey), key);
    if (!gNvs.opened || handle != 1U) return ESP_ERR_NVS_INVALID_HANDLE;
    if (gNvs.getResult != ESP_OK) return gNvs.getResult;
    if (!gNvs.workingPresent) return ESP_ERR_NVS_NOT_FOUND;
    if (output == nullptr) {
        *length = gNvs.workingSize;
        return ESP_OK;
    }
    if (*length < gNvs.workingSize) {
        *length = gNvs.workingSize;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    std::memcpy(output, gNvs.working, gNvs.workingSize);
    *length = gNvs.workingSize;
    return ESP_OK;
}

extern "C" esp_err_t nvs_set_blob(
    nvs_handle_t handle,
    const char* key,
    const void* value,
    size_t length) {
    ++gNvs.setCalls;
    copyText(gNvs.accessedKey, sizeof(gNvs.accessedKey), key);
    if (!gNvs.opened || handle != 1U) return ESP_ERR_NVS_INVALID_HANDLE;
    if (gNvs.setResult != ESP_OK) return gNvs.setResult;
    if (length > sizeof(gNvs.working)) return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    std::memcpy(gNvs.working, value, length);
    gNvs.workingSize = length;
    gNvs.workingPresent = true;
    return ESP_OK;
}

extern "C" esp_err_t nvs_erase_key(nvs_handle_t handle, const char* key) {
    ++gNvs.eraseCalls;
    copyText(gNvs.accessedKey, sizeof(gNvs.accessedKey), key);
    if (!gNvs.opened || handle != 1U) return ESP_ERR_NVS_INVALID_HANDLE;
    if (gNvs.eraseResult != ESP_OK) return gNvs.eraseResult;
    if (!gNvs.workingPresent) return ESP_ERR_NVS_NOT_FOUND;
    gNvs.workingPresent = false;
    gNvs.workingSize = 0U;
    return ESP_OK;
}

extern "C" esp_err_t nvs_commit(nvs_handle_t handle) {
    ++gNvs.commitCalls;
    if (!gNvs.opened || handle != 1U) return ESP_ERR_NVS_INVALID_HANDLE;
    if (gNvs.commitResult != ESP_OK) return gNvs.commitResult;
    std::memcpy(gNvs.committed, gNvs.working, sizeof(gNvs.committed));
    gNvs.committedSize = gNvs.workingSize;
    gNvs.committedPresent = gNvs.workingPresent;
    return ESP_OK;
}

int main() {
    testDefaultLifecycleAndAtomicFailure();
    testBoundsClearAndErrors();
    testPartitionAndConfigurationValidation();
    testWifiCredentialSinkCompositionLifecycle();
    std::cout << "ESP32 NVS storage adapter host tests passed" << std::endl;
    return 0;
}
