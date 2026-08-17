#ifndef BLINKER_PROVISIONING_WIFICREDENTIALSTORE_H
#define BLINKER_PROVISIONING_WIFICREDENTIALSTORE_H

#include "../interface/IAtomicBlobStore.h"
#include "WifiCredential.h"

namespace blinker {

struct WifiCredentialProfile {
    uint32_t generation;
    WifiAuthentication authentication;
    uint8_t ssidLength;
    uint8_t credentialLength;
    uint8_t flags;
    uint8_t ssid[kWifiSsidMaxSize];
    uint8_t credential[kWifiCredentialMaxSize];

    WifiCredentialProfile();

    bool hidden() const { return (flags & 0x01U) != 0U; }
    bool confirmed() const { return (flags & 0x02U) != 0U; }
    void setHidden(bool value) {
        flags = value ? static_cast<uint8_t>(flags | 0x01U)
                      : static_cast<uint8_t>(flags & ~0x01U);
    }
    void setConfirmed(bool value) {
        flags = value ? static_cast<uint8_t>(flags | 0x02U)
                      : static_cast<uint8_t>(flags & ~0x02U);
    }

    WifiNetworkConfig networkConfig() const {
        WifiNetworkConfig config;
        config.ssid = ByteView(ssid, ssidLength);
        config.authentication = authentication;
        config.credential = ByteView(credential, credentialLength);
        config.hidden = hidden();
        return config;
    }
};

void clearWifiCredentialProfile(WifiCredentialProfile& profile);

struct WifiCredentialStoreConfig {
    bool allowPlainFlash;

    WifiCredentialStoreConfig() : allowPlainFlash(false) {}
};

// Crash-recoverable WiFi credential journal built from three independently
// atomic blobs: selector, slot 0 and slot 1. A candidate is committed to the
// inactive slot before the selector changes, so the prior profile remains a
// complete rollback target even when either replace reports an error. New
// profiles remain candidates until confirmActive() persists successful AP
// connectivity. rollbackToPrevious() only accepts candidate -> confirmed.
class WifiCredentialStore : public IWifiCredentialSink {
public:
    enum : size_t {
        serializedProfileSize = 116U,
        serializedSelectorSize = 16U
    };

    WifiCredentialStore(
        IAtomicBlobStore& selector,
        IAtomicBlobStore& slot0,
        IAtomicBlobStore& slot1,
        const WifiCredentialStoreConfig& config =
            WifiCredentialStoreConfig());

    Result begin();
    void end();
    bool ready() const { return ready_; }
    bool hasActiveProfile() const { return ready_ && hasActive_; }
    uint32_t activeGeneration() const {
        return hasActiveProfile() ? activeGeneration_ : 0U;
    }

    Result commit(const WifiNetworkConfig& config) override;
    Result load(WifiCredentialProfile& profile);
    Result loadPrevious(WifiCredentialProfile& profile);
    Result confirmActive();
    // Factory-only fresh-device path. It preserves the server-issued
    // generation, is idempotent for an exact match and never rotates an
    // already installed network.
    Result installConfirmed(const WifiCredentialProfile& profile);
    Result rollbackToPrevious();
    Result clear();

    static Result encodeProfile(
        const WifiCredentialProfile& profile,
        MutableByteSpan output);
    static Result decodeProfile(
        ByteView encoded,
        WifiCredentialProfile& profile);

private:
    struct Selector {
        uint8_t activeSlot;
        uint32_t generation;

        Selector() : activeSlot(0xFFU), generation(0U) {}
    };

    static Result encodeSelector(
        const Selector& selector,
        MutableByteSpan output);
    static Result decodeSelector(ByteView encoded, Selector& selector);
    static bool sameNetwork(
        const WifiCredentialProfile& profile,
        const WifiNetworkConfig& config);

    IAtomicBlobStore& slot(uint8_t index);
    Result loadSlot(
        uint8_t index,
        uint32_t expectedGeneration,
        WifiCredentialProfile& profile);
    Result writeSelector(uint8_t activeSlot, uint32_t generation);
    bool protectedStorage() const;

    IAtomicBlobStore& selector_;
    IAtomicBlobStore& slot0_;
    IAtomicBlobStore& slot1_;
    WifiCredentialStoreConfig config_;
    uint32_t activeGeneration_;
    uint8_t activeSlot_;
    bool ready_;
    bool hasActive_;

    WifiCredentialStore(const WifiCredentialStore&);
    WifiCredentialStore& operator=(const WifiCredentialStore&);
};

} // namespace blinker

#endif
