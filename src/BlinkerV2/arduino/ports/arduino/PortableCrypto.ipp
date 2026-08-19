#include <AES.h>
#include <Curve25519.h>
#include <Ed25519.h>
#include <GCM.h>

#include <BlinkerV2/security/Ed25519Signature.h>

namespace blinker {

namespace {

bool validView(ByteView value) {
    return value.data != nullptr || value.empty();
}

Result validateX25519(
    ByteView privateKey,
    MutableByteSpan output) {
    if (privateKey.data == nullptr || privateKey.size != kX25519KeySize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < kX25519KeySize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    return Result::success();
}

Result validateGcm(
    ByteView key,
    ByteView nonce,
    ByteView associatedData) {
    return key.data != nullptr && key.size == kAes256KeySize &&
                   nonce.data != nullptr && nonce.size == kAesGcmNonceSize &&
                   validView(associatedData)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

} // namespace

inline Result ArduinoCryptoProvider::x25519PublicKey(
    ByteView privateKey,
    MutableByteSpan publicKey) {
    Result result = validateX25519(privateKey, publicKey);
    if (!result) return result;
    secureZero(MutableByteSpan(publicKey.data, kX25519KeySize));
    if (!Curve25519::eval(publicKey.data, privateKey.data, nullptr)) {
        return Result::failure(ErrorCode::InternalError);
    }
    return Result::success();
}

inline Result ArduinoCryptoProvider::x25519(
    ByteView privateKey,
    ByteView remotePublicKey,
    MutableByteSpan sharedSecret) {
    Result result = validateX25519(privateKey, sharedSecret);
    if (!result) return result;
    if (remotePublicKey.data == nullptr ||
        remotePublicKey.size != kX25519KeySize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(MutableByteSpan(sharedSecret.data, kX25519KeySize));
    if (!Curve25519::eval(
            sharedSecret.data,
            privateKey.data,
            remotePublicKey.data)) {
        secureZero(MutableByteSpan(sharedSecret.data, kX25519KeySize));
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return Result::success();
}

inline Result ArduinoCryptoProvider::aes256GcmEncrypt(
    ByteView key,
    ByteView nonce,
    ByteView associatedData,
    ByteView plaintext,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGcm(key, nonce, associatedData);
    if (!result) return result;
    if (!validView(plaintext) || plaintext.size > SIZE_MAX - kAesGcmTagSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t required = plaintext.size + kAesGcmTagSize;
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    GCM<AESTiny256> gcm;
    if (!gcm.setKey(key.data, key.size) ||
        !gcm.setIV(nonce.data, nonce.size)) {
        secureZero(MutableByteSpan(output.data, required));
        gcm.clear();
        return Result::failure(ErrorCode::InternalError);
    }
    gcm.addAuthData(associatedData.data, associatedData.size);
    gcm.encrypt(output.data, plaintext.data, plaintext.size);
    gcm.computeTag(output.data + plaintext.size, kAesGcmTagSize);
    gcm.clear();
    written = required;
    return Result::success();
}

inline Result ArduinoCryptoProvider::aes256GcmDecrypt(
    ByteView key,
    ByteView nonce,
    ByteView associatedData,
    ByteView ciphertextAndTag,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result result = validateGcm(key, nonce, associatedData);
    if (!result) return result;
    if (ciphertextAndTag.data == nullptr ||
        ciphertextAndTag.size < kAesGcmTagSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t plaintextSize = ciphertextAndTag.size - kAesGcmTagSize;
    if (plaintextSize != 0U &&
        (output.data == nullptr || output.size < plaintextSize)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

    GCM<AESTiny256> gcm;
    if (!gcm.setKey(key.data, key.size) ||
        !gcm.setIV(nonce.data, nonce.size)) {
        if (plaintextSize != 0U) {
            secureZero(MutableByteSpan(output.data, plaintextSize));
        }
        gcm.clear();
        return Result::failure(ErrorCode::InternalError);
    }
    gcm.addAuthData(associatedData.data, associatedData.size);
    gcm.decrypt(output.data, ciphertextAndTag.data, plaintextSize);
    const bool authenticated = gcm.checkTag(
        ciphertextAndTag.data + plaintextSize,
        kAesGcmTagSize);
    gcm.clear();
    if (!authenticated) {
        if (plaintextSize != 0U) {
            secureZero(MutableByteSpan(output.data, plaintextSize));
        }
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    written = plaintextSize;
    return Result::success();
}

inline Result ArduinoCryptoProvider::verifyDigest(
    ByteView publicKey,
    ByteView digest,
    ByteView signature) {
    if (publicKey.data == nullptr ||
        publicKey.size != kEd25519PublicKeySize ||
        digest.data == nullptr || digest.size != kEd25519DigestSize ||
        signature.data == nullptr ||
        signature.size != kEd25519SignatureSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (!isCanonicalEd25519PublicKey(publicKey) ||
        !isCanonicalEd25519Signature(signature)) {
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return Ed25519::verify(
               signature.data,
               publicKey.data,
               digest.data,
               digest.size)
               ? Result::success()
               : Result::failure(ErrorCode::AuthenticationRequired);
}

} // namespace blinker
