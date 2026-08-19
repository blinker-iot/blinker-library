#if defined(ARDUINO_ARCH_ESP32)

#include "Esp32Storage.h"

#include <string.h>

namespace blinker {

namespace {

bool validIdentifierCharacter(char value) {
    return (value >= 'a' && value <= 'z') ||
           (value >= 'A' && value <= 'Z') ||
           (value >= '0' && value <= '9') || value == '_' || value == '-' ||
           value == '.';
}

} // namespace

Esp32NvsAtomicBlobStore::Esp32NvsAtomicBlobStore(
    const Esp32NvsBlobStoreConfig& config)
    : namespaceName_(),
      key_(),
      partitionLabel_(),
      maximumSize_(config.maximumSize),
      handle_(0),
      encryptionVerifier_(config.encryptionVerifier),
      encrypted_(config.encrypted),
      encryptionVerified_(false),
      configValid_(false),
      opened_(false) {
    const bool validNamespace = copyIdentifier(
        config.namespaceName,
        namespaceName_,
        kNvsNameCapacity,
        false);
    const bool validKey = copyIdentifier(
        config.key,
        key_,
        kNvsNameCapacity,
        false);
    const bool validPartition = copyIdentifier(
        config.partitionLabel,
        partitionLabel_,
        kPartitionLabelCapacity,
        true);
    configValid_ = validNamespace && validKey && validPartition &&
                   maximumSize_ > 0U;
}

Esp32NvsAtomicBlobStore::~Esp32NvsAtomicBlobStore() {
    end();
}

bool Esp32NvsAtomicBlobStore::copyIdentifier(
    const char* source,
    char* destination,
    size_t capacity,
    bool optional) {
    if (destination == nullptr || capacity == 0U) return false;
    destination[0] = '\0';
    if (source == nullptr || source[0] == '\0') return optional;
    size_t length = 0U;
    while (source[length] != '\0') {
        if (length >= capacity || !validIdentifierCharacter(source[length])) {
            destination[0] = '\0';
            return false;
        }
        destination[length] = source[length];
        ++length;
    }
    destination[length] = '\0';
    return length > 0U;
}

Result Esp32NvsAtomicBlobStore::fromNvsError(esp_err_t error) {
    if (error == ESP_OK) return Result::success();
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return Result::failure(ErrorCode::NotFound);
    }
    if (error == ESP_ERR_NVS_INVALID_LENGTH) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (error == ESP_ERR_NVS_NOT_ENOUGH_SPACE ||
        error == ESP_ERR_NVS_NO_FREE_PAGES ||
        error == ESP_ERR_NVS_VALUE_TOO_LONG) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (error == ESP_ERR_INVALID_ARG || error == ESP_ERR_NVS_INVALID_NAME ||
        error == ESP_ERR_NVS_KEY_TOO_LONG) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (error == ESP_ERR_NVS_NOT_INITIALIZED ||
        error == ESP_ERR_NVS_INVALID_HANDLE ||
        error == ESP_ERR_NVS_PART_NOT_FOUND) {
        return Result::failure(ErrorCode::NotConfigured);
    }
    return Result::failure(ErrorCode::InternalError);
}

bool Esp32NvsAtomicBlobStore::validConfig() const {
    return configValid_;
}

Result Esp32NvsAtomicBlobStore::begin() {
    if (opened_) return Result::success();
    if (!validConfig()) return Result::failure(ErrorCode::InvalidArgument);
    if (encrypted_) {
        const char* label = partitionLabel_[0] == '\0'
                                ? "nvs"
                                : partitionLabel_;
        if (encryptionVerifier_ == nullptr ||
            !encryptionVerifier_(label)) {
            return Result::failure(ErrorCode::NotConfigured);
        }
    }
    nvs_handle_t openedHandle = 0;
    const esp_err_t error = partitionLabel_[0] == '\0'
                                ? nvs_open(
                                      namespaceName_,
                                      NVS_READWRITE,
                                      &openedHandle)
                                : nvs_open_from_partition(
                                      partitionLabel_,
                                      namespaceName_,
                                      NVS_READWRITE,
                                      &openedHandle);
    Result result = fromNvsError(error);
    if (!result) return result;
    handle_ = openedHandle;
    encryptionVerified_ = encrypted_;
    opened_ = true;
    return Result::success();
}

void Esp32NvsAtomicBlobStore::end() {
    if (!opened_) return;
    nvs_close(handle_);
    handle_ = 0;
    encryptionVerified_ = false;
    opened_ = false;
}

Result Esp32NvsAtomicBlobStore::load(
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (output.data == nullptr && output.size != 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    size_t required = 0U;
    Result result = fromNvsError(nvs_get_blob(handle_, key_, nullptr, &required));
    if (!result) return result;
    if (required == 0U) return Result::failure(ErrorCode::InvalidEncoding);
    if (required > maximumSize_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    size_t actual = required;
    result = fromNvsError(nvs_get_blob(handle_, key_, output.data, &actual));
    if (!result) return result;
    if (actual != required) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    written = actual;
    return Result::success();
}

Result Esp32NvsAtomicBlobStore::replace(ByteView value) {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    if (value.data == nullptr || value.empty()) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (value.size > maximumSize_) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    Result result = fromNvsError(
        nvs_set_blob(handle_, key_, value.data, value.size));
    if (result) result = fromNvsError(nvs_commit(handle_));
    return result;
}

Result Esp32NvsAtomicBlobStore::clear() {
    if (!opened_) return Result::failure(ErrorCode::NotConfigured);
    Result result = fromNvsError(nvs_erase_key(handle_, key_));
    if (result) result = fromNvsError(nvs_commit(handle_));
    return result;
}

StorageProtection Esp32NvsAtomicBlobStore::protection() const {
    return encryptionVerified_ ? StorageProtection::EncryptedFlash
                               : StorageProtection::PlainFlash;
}

Esp32NvsBlobStoreConfig Esp32NvsWifiCredentialSink::makeBlobConfig(
    const Esp32NvsWifiCredentialSinkConfig& config,
    const char* key,
    size_t maximumSize) {
    Esp32NvsBlobStoreConfig blob;
    blob.namespaceName = config.namespaceName;
    blob.key = key;
    blob.partitionLabel = config.partitionLabel;
    blob.maximumSize = maximumSize;
    blob.encrypted = config.encrypted;
    blob.encryptionVerifier = config.encryptionVerifier;
    return blob;
}

WifiCredentialStoreConfig Esp32NvsWifiCredentialSink::makeJournalConfig(
    const Esp32NvsWifiCredentialSinkConfig& config) {
    WifiCredentialStoreConfig journal;
    journal.allowPlainFlash = config.allowPlainFlash;
    return journal;
}

Esp32NvsWifiCredentialSink::Esp32NvsWifiCredentialSink(
    const Esp32NvsWifiCredentialSinkConfig& config)
    : selector_(makeBlobConfig(
          config,
          "wifi_sel",
          WifiCredentialStore::serializedSelectorSize)),
      slot0_(makeBlobConfig(
          config,
          "wifi_0",
          WifiCredentialStore::serializedProfileSize)),
      slot1_(makeBlobConfig(
          config,
          "wifi_1",
          WifiCredentialStore::serializedProfileSize)),
      journal_(selector_, slot0_, slot1_, makeJournalConfig(config)) {}

Esp32NvsWifiCredentialSink::~Esp32NvsWifiCredentialSink() {
    end();
}

Result Esp32NvsWifiCredentialSink::begin() {
    if (journal_.ready()) return Result::success();
    Result result = selector_.begin();
    if (result) result = slot0_.begin();
    if (result) result = slot1_.begin();
    if (result) result = journal_.begin();
    if (!result) end();
    return result;
}

void Esp32NvsWifiCredentialSink::end() {
    journal_.end();
    slot1_.end();
    slot0_.end();
    selector_.end();
}

} // namespace blinker

#endif // ARDUINO_ARCH_ESP32
