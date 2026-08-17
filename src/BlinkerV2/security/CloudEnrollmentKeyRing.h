#ifndef BLINKER_SECURITY_CLOUDENROLLMENTKEYRING_H
#define BLINKER_SECURITY_CLOUDENROLLMENTKEYRING_H

#include "../interface/ICloudEnrollmentEncryptionKeySource.h"

namespace blinker {

struct CloudEnrollmentPublicKey {
    uint32_t keyId;
    ByteView publicKey;

    CloudEnrollmentPublicKey() : keyId(0U) {}
    CloudEnrollmentPublicKey(uint32_t id, ByteView key)
        : keyId(id), publicKey(key) {}
};

// Borrowed Flash/const X25519 key ring. The active key is used for new
// requests; previous entries remain addressable so an in-flight journal can
// reproduce its exact request after a server key rotation.
class CloudEnrollmentKeyRing final
    : public ICloudEnrollmentEncryptionKeySource {
public:
    CloudEnrollmentKeyRing(
        uint32_t activeKeyId,
        const CloudEnrollmentPublicKey* keys,
        size_t keyCount)
        : activeKeyId_(activeKeyId), keys_(keys), keyCount_(keyCount) {}

    Result activeKeyId(uint32_t& keyId) override;
    Result loadPublicKey(
        uint32_t keyId,
        MutableByteSpan publicKey) override;

private:
    Result validate() const;

    uint32_t activeKeyId_;
    const CloudEnrollmentPublicKey* keys_;
    size_t keyCount_;
};

} // namespace blinker

#endif
