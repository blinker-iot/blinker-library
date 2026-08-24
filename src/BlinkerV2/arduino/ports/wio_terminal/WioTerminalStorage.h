#ifndef BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_STORAGE_H
#define BLINKER_V2_ARDUINO_PORTS_WIO_TERMINAL_STORAGE_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalStorage requires a Wio Terminal target"
#endif

#include <Arduino.h>
#include <BlinkerV2/identity/DeviceAccessStore.h>
#include <BlinkerV2/identity/DeviceInstanceIdStore.h>
#include <BlinkerV2/interface/IAtomicBlobStore.h>
#include <BlinkerV2/provisioning/WifiCredentialStore.h>

namespace blinker {

// Wio Terminal keeps Device V2 records in the final two 4 KiB sectors of its
// onboard external QSPI flash. Writes target the inactive sector and are
// accepted only after CRC and read-back verification.
class WioTerminalFlashBlobBank {
public:
    WioTerminalFlashBlobBank();

    Result begin();
    void end();

    IAtomicBlobStore& deviceInstanceBlob();
    IAtomicBlobStore& wifiSelectorBlob();
    IAtomicBlobStore& wifiSlot0Blob();
    IAtomicBlobStore& wifiSlot1Blob();
    IAtomicBlobStore& deviceAccessBlob();

private:
    enum BlobId : uint8_t {
        BlobDeviceInstance = 0U,
        BlobWifiSelector,
        BlobWifiSlot0,
        BlobWifiSlot1,
        BlobDeviceAccess,
        BlobCount
    };

    class BlobView final : public IAtomicBlobStore {
    public:
        BlobView(WioTerminalFlashBlobBank& bank, BlobId id);

        Result load(MutableByteSpan output, size_t& written) override;
        Result replace(ByteView value) override;
        Result clear() override;
        StorageProtection protection() const override;

    private:
        WioTerminalFlashBlobBank& bank_;
        BlobId id_;
    };

    enum : size_t {
        kHeaderSize = 16U,
        kEntryHeaderSize = 4U,
        kEraseBlockSize = 4096U,
        kImageSize =
            kHeaderSize + kEntryHeaderSize * BlobCount +
            DeviceInstanceIdStore::serializedSize +
            WifiCredentialStore::serializedSelectorSize +
            WifiCredentialStore::serializedProfileSize * 2U +
            DeviceAccessStore::serializedSize
    };

    static_assert(
        kImageSize <= kEraseBlockSize,
        "Wio Terminal Device V2 snapshot exceeds one QSPI sector");

    static size_t maximumSize(BlobId id);
    static size_t entryOffset(BlobId id);
    static uint8_t* entry(uint8_t* image, BlobId id);
    static const uint8_t* entry(const uint8_t* image, BlobId id);
    static bool validImage(uint8_t* image, uint32_t& generation);

    uint32_t slotAddress(uint8_t slot) const;
    bool storageRegionAvailable() const;
    Result newest(
        uint8_t* image,
        uint32_t& generation,
        uint8_t& slot) const;
    Result writeImage(
        uint8_t slot,
        uint32_t generation,
        uint8_t* image);
    Result load(BlobId id, MutableByteSpan output, size_t& written);
    Result replace(BlobId id, ByteView value);
    Result clear(BlobId id);

    BlobView deviceInstance_;
    BlobView wifiSelector_;
    BlobView wifiSlot0_;
    BlobView wifiSlot1_;
    BlobView deviceAccess_;
    uint32_t flashSize_;
    bool opened_;

    WioTerminalFlashBlobBank(const WioTerminalFlashBlobBank&);
    WioTerminalFlashBlobBank& operator=(
        const WioTerminalFlashBlobBank&);
};

} // namespace blinker

#endif
