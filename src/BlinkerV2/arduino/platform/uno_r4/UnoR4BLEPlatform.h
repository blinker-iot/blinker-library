#ifndef BLINKER_V2_ARDUINO_UNO_R4_BLE_PLATFORM_H
#define BLINKER_V2_ARDUINO_UNO_R4_BLE_PLATFORM_H

#include "../../ports/arduino/ArduinoBleLink.h"
#include "../../ports/arduino/PortableCrypto.h"
#include "../../ports/uno_r4/UnoR4BleSecurity.h"
#include "../../ports/uno_r4/UnoR4Storage.h"

#include <BlinkerV2/security/Ed25519ServerKeyRingVerifier.h>
#include <BlinkerV2/arduino/config/PortableServerKeys.h>
#include <BlinkerV2/provisioning/BleEnrollmentContract.h>

#if !defined(ARDUINO_ARCH_RENESAS_UNO) && \
    !defined(ARDUINO_ARCH_RENESAS)
#error "This official platform requires an Arduino Renesas target"
#endif

#if BLINKER_RESOURCE_PROFILE != BLINKER_RESOURCE_PROFILE_SMALL
#error "This official platform is sized for the Small resource profile"
#endif

namespace blinker {
namespace integration {
namespace official_detail {

inline ArduinoBleLinkConfig renesasEncryptedBleConfig() {
    ArduinoBleLinkConfig config;
    config.requireEncryption = true;
    return config;
}

class RenesasUnoBleRadio {
public:
    enum : size_t { maximumPacketSize = 20U };

    RenesasUnoBleRadio()
        : security_(),
          link_(security_, renesasEncryptedBleConfig()) {}

    ArduinoBleLink& link() { return link_; }
    void stop() { link_.stop(); }

private:
    RenesasUnoBleSecuritySource security_;
    ArduinoBleLink link_;
};

#if BLINKER_OFFICIAL_HAS_SERVER_SIGNATURE
class RenesasUnoPlatformBlePlatform {
public:
    enum : size_t {
        maximumBlePacketSize = RenesasUnoBleRadio::maximumPacketSize
    };

    RenesasUnoPlatformBlePlatform()
        : storage_(),
          crypto_(),
          signatureVerifier_(
              crypto_,
              official::serverSigningKeys,
              official::serverSigningKeysCount),
          radio_() {}

    Result begin() { return storage_.begin(); }
    void end() {
        radio_.stop();
        storage_.end();
    }

    IAtomicBlobStore& deviceInstanceBlob() {
        return storage_.deviceInstanceBlob();
    }
    IAtomicBlobStore& deviceAccessBlob() {
        return storage_.deviceAccessBlob();
    }
    IServerSignatureVerifier& serverSignatureVerifier() {
        return signatureVerifier_;
    }
    BleEnrollmentApplicationConfig bleEnrollmentConfig() const {
        BleEnrollmentApplicationConfig config;
        config.serverKeyId = official::serverSigningKeys[0].keyId;
        config.signatureAlgorithm = ServerSignatureAlgorithm::Ed25519;
        return config;
    }
    INoiseCryptoProvider& noiseCrypto() { return crypto_; }
    ArduinoBleLink& bleLink() { return radio_.link(); }

private:
    RenesasUnoPreferencesBlobBank storage_;
    ArduinoCryptoProvider crypto_;
    Ed25519ServerKeyRingVerifier signatureVerifier_;
    RenesasUnoBleRadio radio_;
};
#endif

} // namespace official_detail
} // namespace integration
} // namespace blinker

#endif
