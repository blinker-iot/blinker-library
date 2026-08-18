#ifndef BLINKER_RENESAS_UNO_STORAGE_ADAPTER_H
#define BLINKER_RENESAS_UNO_STORAGE_ADAPTER_H

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "BlinkerRenesasUnoStorageAdapter requires an Arduino Renesas target"
#endif

#include <BlinkerV2/identity/CloudCredentialStore.h>
#include <BlinkerV2/identity/ControllerCredentialStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/identity/OwnershipRecordStore.h>
#include <BlinkerV2/provisioning/CloudEnrollmentRecordStore.h>
#include <BlinkerV2/provisioning/LocalSetupSagaStore.h>
#include <BlinkerV2/provisioning/OwnershipClaimRecordStore.h>
#include <BlinkerV2/provisioning/OwnershipReleaseRecordStore.h>
#include <BlinkerV2/provisioning/WifiCredentialStore.h>
#include <Preferences.h>

namespace blinker {

struct RenesasUnoStorageConfig {
    const char* namespaceName;

    RenesasUnoStorageConfig() : namespaceName("bl_v2") {}
};

// WiFiS3 Preferences is hosted by the UNO R4 WiFi connectivity module. This
// bank adds the IAtomicBlobStore contract above that plain key-value API: each
// logical blob alternates between two keys and carries generation, CRC and a
// tombstone. A failed replacement therefore leaves one complete prior record.
// The views share one Preferences handle and must be used single-threaded.
class RenesasUnoPreferencesBlobBank {
public:
    explicit RenesasUnoPreferencesBlobBank(
        const RenesasUnoStorageConfig& config = RenesasUnoStorageConfig());
    ~RenesasUnoPreferencesBlobBank();

    Result begin();
    void end();
    bool opened() const { return opened_; }

    IAtomicBlobStore& cloudBlob() { return cloud_; }
    IAtomicBlobStore& ownershipBlob() { return ownership_; }
    IAtomicBlobStore& controllerBlob() { return controller_; }
    IAtomicBlobStore& wifiSelectorBlob() { return wifiSelector_; }
    IAtomicBlobStore& wifiSlot0Blob() { return wifiSlot0_; }
    IAtomicBlobStore& wifiSlot1Blob() { return wifiSlot1_; }
    IAtomicBlobStore& deviceInstanceBlob() { return deviceInstance_; }
    IAtomicBlobStore& ownershipClaimBlob() { return ownershipClaim_; }
    IAtomicBlobStore& cloudEnrollmentBlob() { return cloudEnrollment_; }
    IAtomicBlobStore& ownershipReleaseBlob() { return ownershipRelease_; }
    IAtomicBlobStore& localSetupSagaBlob() { return localSetupSaga_; }

private:
    enum BlobId : uint8_t {
        BlobCloud = 0U,
        BlobWifiSelector,
        BlobWifiSlot0,
        BlobWifiSlot1,
        BlobOwnership,
        BlobController,
        BlobDeviceInstance,
        BlobOwnershipClaim,
        BlobCloudEnrollment,
        BlobOwnershipRelease,
        BlobLocalSetupSaga,
        BlobCount
    };

    class BlobView final : public IAtomicBlobStore {
    public:
        BlobView(RenesasUnoPreferencesBlobBank& bank, BlobId id)
            : bank_(bank), id_(id) {}

        Result load(MutableByteSpan output, size_t& written) override {
            return bank_.load(id_, output, written);
        }
        Result replace(ByteView value) override {
            return bank_.replace(id_, value);
        }
        Result clear() override { return bank_.clear(id_); }
        StorageProtection protection() const override {
            return StorageProtection::PlainFlash;
        }

    private:
        RenesasUnoPreferencesBlobBank& bank_;
        BlobId id_;
    };

    struct SlotInfo {
        uint32_t generation;
        uint16_t payloadSize;
        uint8_t state;
        bool present;
        bool valid;

        SlotInfo()
            : generation(0U),
              payloadSize(0U),
              state(0U),
              present(false),
              valid(false) {}
    };

    enum : size_t {
        kNamespaceCapacity = 15U,
        kHeaderSize = 16U,
        kCrcSize = 4U,
        // Scratch capacity follows the largest routed blob, not one schema
        // that may legitimately shrink during protocol refinement.
        kMaximumPayloadSize = LocalSetupSagaStore::serializedSize,
        kMaximumRecordSize =
            kHeaderSize + kMaximumPayloadSize + kCrcSize
    };

    static_assert(
        static_cast<size_t>(ControllerCredentialStore::serializedSize) <=
            static_cast<size_t>(kMaximumPayloadSize),
        "controller store exceeds Renesas journal scratch");
    static_assert(
        static_cast<size_t>(CloudEnrollmentRecordStore::serializedSize) <=
            static_cast<size_t>(kMaximumPayloadSize),
        "cloud enrollment store exceeds Renesas journal scratch");

    static bool copyNamespace(const char* source, char* destination);
    static size_t maximumSize(BlobId id);
    static const char* key(BlobId id, uint8_t slot);
    SlotInfo inspect(
        BlobId id,
        uint8_t slot,
        uint8_t* scratch,
        size_t scratchSize);
    Result newest(
        BlobId id,
        MutableByteSpan output,
        size_t& written,
        uint32_t& generation,
        uint8_t& slot,
        uint8_t& state);
    Result writeRecord(
        BlobId id,
        uint8_t slot,
        uint32_t generation,
        uint8_t state,
        ByteView payload);
    Result load(BlobId id, MutableByteSpan output, size_t& written);
    Result replace(BlobId id, ByteView value);
    Result clear(BlobId id);

    char namespaceName_[kNamespaceCapacity + 1U];
    Preferences preferences_;
    BlobView cloud_;
    BlobView wifiSelector_;
    BlobView wifiSlot0_;
    BlobView wifiSlot1_;
    BlobView ownership_;
    BlobView controller_;
    BlobView deviceInstance_;
    BlobView ownershipClaim_;
    BlobView cloudEnrollment_;
    BlobView ownershipRelease_;
    BlobView localSetupSaga_;
    bool configValid_;
    bool opened_;

    RenesasUnoPreferencesBlobBank(
        const RenesasUnoPreferencesBlobBank&);
    RenesasUnoPreferencesBlobBank& operator=(
        const RenesasUnoPreferencesBlobBank&);
};

} // namespace blinker

#endif
