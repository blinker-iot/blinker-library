#include "P256DeveloperIdentity.h"

#include <string.h>

#include "DeveloperDeviceId.h"
#include "P256DeveloperIdentityRecord.h"
#include "../core/SecureMemory.h"
#include "../security/P256Signature.h"

namespace blinker {

P256DeveloperIdentity::P256DeveloperIdentity(
    IAtomicBlobStore& storage,
    IP256CryptoProvider& crypto,
    const P256DeveloperIdentityConfig& config)
    : storage_(storage),
      crypto_(crypto),
      config_(config),
      privateKey_(),
      publicKey_(),
      physicalDeviceId_(),
      ready_(false) {}

P256DeveloperIdentity::~P256DeveloperIdentity() {
    clearCached();
}

void P256DeveloperIdentity::clearCached() {
    secureZero(MutableByteSpan(privateKey_, sizeof(privateKey_)));
    secureZero(MutableByteSpan(publicKey_, sizeof(publicKey_)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(physicalDeviceId_),
        sizeof(physicalDeviceId_)));
    ready_ = false;
}

void P256DeveloperIdentity::end() {
    clearCached();
}

Result P256DeveloperIdentity::installPrivateKey(ByteView privateKey) {
    if (!isValidP256Scalar(privateKey)) {
        return Result::failure(ErrorCode::InvalidEncoding);
    }
    uint8_t publicKey[kP256PublicKeySize] = {};
    Result result = crypto_.derivePublicKey(
        privateKey,
        MutableByteSpan(publicKey, sizeof(publicKey)));
    if (result && !isCanonicalP256PublicKey(
                      ByteView(publicKey, sizeof(publicKey)))) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    char physicalId[kDeveloperPhysicalDeviceIdSize] = {};
    StringView physicalIdView;
    if (result) {
        result = deriveDeveloperPhysicalDeviceId(
            FactoryProofAlgorithm::EcdsaP256Sha256Raw,
            ByteView(publicKey, sizeof(publicKey)),
            MutableCharSpan(physicalId, sizeof(physicalId)),
            physicalIdView);
    }
    if (!result) {
        secureZero(MutableByteSpan(publicKey, sizeof(publicKey)));
        secureZero(MutableByteSpan(
            reinterpret_cast<uint8_t*>(physicalId),
            sizeof(physicalId)));
        return result;
    }

    memcpy(privateKey_, privateKey.data, sizeof(privateKey_));
    memcpy(publicKey_, publicKey, sizeof(publicKey_));
    memcpy(physicalDeviceId_, physicalIdView.data, physicalIdView.size);
    ready_ = true;
    secureZero(MutableByteSpan(publicKey, sizeof(publicKey)));
    secureZero(MutableByteSpan(
        reinterpret_cast<uint8_t*>(physicalId),
        sizeof(physicalId)));
    return Result::success();
}

Result P256DeveloperIdentity::loadExisting(ByteView record) {
    uint8_t privateKey[kP256PrivateKeySize] = {};
    Result result = decodeP256DeveloperIdentityRecord(
        record,
        MutableByteSpan(privateKey, sizeof(privateKey)));
    if (result) {
        result = installPrivateKey(ByteView(privateKey, sizeof(privateKey)));
    }
    secureZero(MutableByteSpan(privateKey, sizeof(privateKey)));
    return result;
}

Result P256DeveloperIdentity::create() {
    uint8_t privateKey[kP256PrivateKeySize] = {};
    uint8_t record[kP256DeveloperIdentityRecordSize] = {};
    ByteView encoded;
    Result result = crypto_.generatePrivateKey(
        MutableByteSpan(privateKey, sizeof(privateKey)));
    if (result && !isValidP256Scalar(
                      ByteView(privateKey, sizeof(privateKey)))) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) {
        result = installPrivateKey(ByteView(privateKey, sizeof(privateKey)));
    }
    if (result) {
        result = encodeP256DeveloperIdentityRecord(
            ByteView(privateKey, sizeof(privateKey)),
            MutableByteSpan(record, sizeof(record)),
            encoded);
    }
    if (result) result = storage_.replace(encoded);
    if (!result) clearCached();
    secureZero(MutableByteSpan(privateKey, sizeof(privateKey)));
    secureZero(MutableByteSpan(record, sizeof(record)));
    return result;
}

Result P256DeveloperIdentity::begin() {
    if (ready_) return Result::success();
    if (storage_.protection() == StorageProtection::PlainFlash &&
        !config_.allowPlainFlash) {
        return Result::failure(ErrorCode::NotConfigured);
    }

    uint8_t record[kP256DeveloperIdentityRecordSize] = {};
    size_t written = 0U;
    Result result = storage_.load(
        MutableByteSpan(record, sizeof(record)),
        written);
    if (result) {
        if (written != sizeof(record)) {
            result = Result::failure(ErrorCode::InvalidEncoding);
        } else {
            result = loadExisting(ByteView(record, written));
        }
    } else if (result.code() == ErrorCode::NotFound &&
               config_.createIfMissing) {
        result = create();
    }
    secureZero(MutableByteSpan(record, sizeof(record)));
    if (!result) clearCached();
    return result;
}

Result P256DeveloperIdentity::physicalDeviceId(StringView& output) const {
    output = StringView();
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    output = StringView(
        physicalDeviceId_,
        kDeveloperPhysicalDeviceIdSize);
    return Result::success();
}

Result P256DeveloperIdentity::publicKey(
    MutableByteSpan output,
    size_t& written) const {
    written = 0U;
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (output.data == nullptr || output.size < sizeof(publicKey_)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    memcpy(output.data, publicKey_, sizeof(publicKey_));
    written = sizeof(publicKey_);
    return Result::success();
}

Result P256DeveloperIdentity::prove(
    ByteView challenge,
    MutableByteSpan proof,
    size_t& written) {
    written = 0U;
    if (!ready_) return Result::failure(ErrorCode::NotConfigured);
    if (challenge.data == nullptr || challenge.size != kP256DigestSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (proof.data == nullptr || proof.size < kP256SignatureSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    uint8_t signature[kP256SignatureSize] = {};
    Result result = crypto_.signDigest(
        ByteView(privateKey_, sizeof(privateKey_)),
        challenge,
        MutableByteSpan(signature, sizeof(signature)));
    if (result) {
        result = normalizeP256Signature(
            MutableByteSpan(signature, sizeof(signature)));
    }
    if (result) {
        memcpy(proof.data, signature, sizeof(signature));
        written = sizeof(signature);
    }
    secureZero(MutableByteSpan(signature, sizeof(signature)));
    return result;
}

} // namespace blinker
