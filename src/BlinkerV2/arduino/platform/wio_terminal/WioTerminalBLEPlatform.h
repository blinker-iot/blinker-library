#ifndef BLINKER_V2_ARDUINO_WIO_TERMINAL_BLE_PLATFORM_H
#define BLINKER_V2_ARDUINO_WIO_TERMINAL_BLE_PLATFORM_H

#if !defined(SEEED_WIO_TERMINAL) && !defined(WIO_TERMINAL)
#error "WioTerminalBLEPlatform requires a Wio Terminal target"
#endif

#include "../../ports/arduino/PortableCrypto.h"
#include "../../ports/wio_terminal/WioTerminalRpcBle.h"
#include "../../ports/wio_terminal/WioTerminalStorage.h"

#include <BlinkerV2/arduino/config/PortableServerKeys.h>
#include <BlinkerV2/identity/OwnershipRecordStore.h>
#include <BlinkerV2/security/Ed25519ServerKeyRingVerifier.h>

namespace blinker {
namespace integration {
namespace official_detail {

#if BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE
class WioTerminalPlatformBlePlatform {
public:
    enum : size_t {
        maximumBlePacketSize =
            WioTerminalRpcBleLink::maximumPacketSize
    };

    WioTerminalPlatformBlePlatform()
        : storage_(),
          ownershipStore_(storage_.ownershipBlob()),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          link_() {}

    Result begin() { return storage_.begin(); }
    void end() {
        link_.stop();
        storage_.end();
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return storage_.deviceInstanceBlob();
    }
    OwnershipRecordStore& ownershipStore() {
        return ownershipStore_;
    }
    IAtomicBlobStore& ownershipClaimBlob() {
        return storage_.ownershipClaimBlob();
    }
    IAtomicBlobStore& controllerCredentialBlob() {
        return storage_.controllerBlob();
    }
    IAtomicBlobStore& localSetupSagaBlob() {
        return storage_.localSetupSagaBlob();
    }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    WioTerminalRpcBleLink& bleLink() { return link_; }

private:
    WioTerminalFlashBlobBank storage_;
    OwnershipRecordStore ownershipStore_;
    ArduinoCryptoProvider crypto_;
    Ed25519ServerKeyRingVerifier signatureVerifier_;
    WioTerminalRpcBleLink link_;
};
#endif

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
