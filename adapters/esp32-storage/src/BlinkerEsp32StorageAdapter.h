#ifndef BLINKER_ESP32_STORAGE_ADAPTER_H
#define BLINKER_ESP32_STORAGE_ADAPTER_H

#if !defined(ARDUINO_ARCH_ESP32)
#error "BlinkerEsp32StorageAdapter requires an ESP32 Arduino target"
#endif

#include <BlinkerV2Advanced.h>
#include <nvs.h>

namespace blinker {

// Called by begin() before an encrypted store is opened. Product firmware
// supplies the attestation because NVS-layer encryption may be initialized
// outside this adapter. The label is "nvs" for the default partition.
typedef bool (*Esp32NvsEncryptionVerifier)(const char* partitionLabel);

struct Esp32NvsBlobStoreConfig {
    const char* namespaceName;
    const char* key;
    const char* partitionLabel;
    size_t maximumSize;
    bool encrypted;
    Esp32NvsEncryptionVerifier encryptionVerifier;

    Esp32NvsBlobStoreConfig()
        : namespaceName("bl_v2_blob"),
          key("record"),
          partitionLabel(nullptr),
          maximumSize(256U),
          encrypted(false),
          encryptionVerifier(nullptr) {}
};

// The Arduino ESP32 core owns NVS initialization. begin() only opens a
// read-write handle. Lifecycle calls are single-threaded; ESP-IDF serializes
// normal get/set/erase operations on an open handle.
class Esp32NvsAtomicBlobStore : public IAtomicBlobStore {
public:
    explicit Esp32NvsAtomicBlobStore(
        const Esp32NvsBlobStoreConfig& config = Esp32NvsBlobStoreConfig());
    ~Esp32NvsAtomicBlobStore() override;

    Result begin();
    void end();
    bool opened() const { return opened_; }
    size_t maximumSize() const { return maximumSize_; }

    Result load(MutableByteSpan output, size_t& written) override;
    Result replace(ByteView value) override;
    Result clear() override;
    StorageProtection protection() const override;

private:
    enum : size_t {
        kNvsNameCapacity = 15U,
        kPartitionLabelCapacity = 16U
    };

    static bool copyIdentifier(
        const char* source,
        char* destination,
        size_t capacity,
        bool optional);
    static Result fromNvsError(esp_err_t error);
    bool validConfig() const;

    char namespaceName_[kNvsNameCapacity + 1U];
    char key_[kNvsNameCapacity + 1U];
    char partitionLabel_[kPartitionLabelCapacity + 1U];
    size_t maximumSize_;
    nvs_handle_t handle_;
    Esp32NvsEncryptionVerifier encryptionVerifier_;
    bool encrypted_;
    bool encryptionVerified_;
    bool configValid_;
    bool opened_;

    Esp32NvsAtomicBlobStore(const Esp32NvsAtomicBlobStore&);
    Esp32NvsAtomicBlobStore& operator=(const Esp32NvsAtomicBlobStore&);
};

struct Esp32NvsWifiCredentialSinkConfig {
    const char* namespaceName;
    const char* partitionLabel;
    bool encrypted;
    bool allowPlainFlash;
    Esp32NvsEncryptionVerifier encryptionVerifier;

    Esp32NvsWifiCredentialSinkConfig()
        : namespaceName("bl_v2_net"),
          partitionLabel(nullptr),
          encrypted(false),
          allowPlainFlash(false),
          encryptionVerifier(nullptr) {}
};

// ESP32 composition wrapper for the portable two-slot WiFi credential
// journal. It owns three narrow NVS blobs in one namespace and deliberately
// has no dependency on WiFi.h or the ESP32 network lifecycle.
class Esp32NvsWifiCredentialSink : public IWifiCredentialSink {
public:
    explicit Esp32NvsWifiCredentialSink(
        const Esp32NvsWifiCredentialSinkConfig& config =
            Esp32NvsWifiCredentialSinkConfig());
    ~Esp32NvsWifiCredentialSink() override;

    Result begin();
    void end();
    bool ready() const { return journal_.ready(); }
    bool hasActiveProfile() const {
        return journal_.hasActiveProfile();
    }

    Result commit(const WifiNetworkConfig& config) override {
        return journal_.commit(config);
    }
    Result load(WifiCredentialProfile& profile) {
        return journal_.load(profile);
    }
    Result loadPrevious(WifiCredentialProfile& profile) {
        return journal_.loadPrevious(profile);
    }
    Result rollbackToPrevious() {
        return journal_.rollbackToPrevious();
    }
    Result clear() { return journal_.clear(); }
    WifiCredentialStore& credentialStore() { return journal_; }
    const WifiCredentialStore& credentialStore() const { return journal_; }

private:
    static Esp32NvsBlobStoreConfig makeBlobConfig(
        const Esp32NvsWifiCredentialSinkConfig& config,
        const char* key,
        size_t maximumSize);
    static WifiCredentialStoreConfig makeJournalConfig(
        const Esp32NvsWifiCredentialSinkConfig& config);

    Esp32NvsAtomicBlobStore selector_;
    Esp32NvsAtomicBlobStore slot0_;
    Esp32NvsAtomicBlobStore slot1_;
    WifiCredentialStore journal_;

    Esp32NvsWifiCredentialSink(const Esp32NvsWifiCredentialSink&);
    Esp32NvsWifiCredentialSink& operator=(
        const Esp32NvsWifiCredentialSink&);
};

} // namespace blinker

#endif
