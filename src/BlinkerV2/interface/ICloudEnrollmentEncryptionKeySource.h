#ifndef BLINKER_INTERFACE_ICLOUDENROLLMENTENCRYPTIONKEYSOURCE_H
#define BLINKER_INTERFACE_ICLOUDENROLLMENTENCRYPTIONKEYSOURCE_H

#include "IX25519AesGcmCryptoProvider.h"

namespace blinker {

// Pinned server X25519 keys are configuration, not remotely accepted key
// material. Implementations should retain an overlap key during rotation so
// a persisted in-flight request remains reproducible after reboot.
class ICloudEnrollmentEncryptionKeySource {
public:
    virtual ~ICloudEnrollmentEncryptionKeySource() {}

    virtual Result activeKeyId(uint32_t& keyId) = 0;
    virtual Result loadPublicKey(
        uint32_t keyId,
        MutableByteSpan publicKey) = 0;
};

} // namespace blinker

#endif
