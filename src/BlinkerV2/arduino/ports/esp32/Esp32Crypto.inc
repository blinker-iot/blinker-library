#if defined(ARDUINO_ARCH_ESP32)

#include "Esp32Crypto.h"

#include <esp_random.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/gcm.h>

#include <BlinkerV2/security/P256Signature.h>

namespace blinker {

namespace {

const size_t kX25519Size = 32U;
const size_t kAes256KeySize = 32U;
const size_t kGcmNonceSize = 12U;
const size_t kGcmTagSize = 16U;

bool validView(ByteView value) {
    return value.data != nullptr || value.empty();
}

Result validateX25519Input(
    ByteView privateKey,
    MutableByteSpan output) {
    if (privateKey.data == nullptr || privateKey.size != kX25519Size) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (output.data == nullptr || output.size < kX25519Size) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    return Result::success();
}

Result prepareGcm(
    ByteView key,
    ByteView nonce,
    ByteView associatedData) {
    if (key.data == nullptr || key.size != kAes256KeySize ||
        nonce.data == nullptr || nonce.size != kGcmNonceSize ||
        !validView(associatedData)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

} // namespace

int Esp32MbedTlsCryptoProvider::fillRandom(
    void*,
    unsigned char* output,
    size_t size) {
    if (output == nullptr && size != 0U) return -1;
    if (size != 0U) esp_fill_random(output, size);
    return 0;
}

Result Esp32MbedTlsCryptoProvider::x25519PublicKey(
    ByteView privateKey,
    MutableByteSpan publicKey) {
    Result validation = validateX25519Input(privateKey, publicKey);
    if (!validation) return validation;
    secureZero(MutableByteSpan(publicKey.data, kX25519Size));

#if !defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
    return Result::failure(ErrorCode::UnsupportedFeature);
#else
    mbedtls_ecp_keypair key;
    mbedtls_ecp_keypair_init(&key);
    int error = mbedtls_ecp_read_key(
        MBEDTLS_ECP_DP_CURVE25519,
        &key,
        privateKey.data,
        privateKey.size);
    if (error == 0) {
        error = mbedtls_ecp_keypair_calc_public(
            &key,
            fillRandom,
            nullptr);
    }
    size_t written = 0U;
    if (error == 0) {
        error = mbedtls_ecp_write_public_key(
            &key,
            MBEDTLS_ECP_PF_UNCOMPRESSED,
            &written,
            publicKey.data,
            kX25519Size);
    }
    mbedtls_ecp_keypair_free(&key);
    if (error != 0 || written != kX25519Size) {
        secureZero(MutableByteSpan(publicKey.data, kX25519Size));
        return Result::failure(ErrorCode::InternalError);
    }
    return Result::success();
#endif
}

Result Esp32MbedTlsCryptoProvider::x25519(
    ByteView privateKey,
    ByteView remotePublicKey,
    MutableByteSpan sharedSecret) {
    Result validation = validateX25519Input(privateKey, sharedSecret);
    if (!validation) return validation;
    if (remotePublicKey.data == nullptr ||
        remotePublicKey.size != kX25519Size) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    secureZero(MutableByteSpan(sharedSecret.data, kX25519Size));

#if !defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
    return Result::failure(ErrorCode::UnsupportedFeature);
#else
    mbedtls_ecp_keypair local;
    mbedtls_ecp_point remote;
    mbedtls_mpi shared;
    mbedtls_ecp_keypair_init(&local);
    mbedtls_ecp_point_init(&remote);
    mbedtls_mpi_init(&shared);
    int error = mbedtls_ecp_read_key(
        MBEDTLS_ECP_DP_CURVE25519,
        &local,
        privateKey.data,
        privateKey.size);
    if (error == 0) {
        error = mbedtls_ecp_point_read_binary(
            &local.MBEDTLS_PRIVATE(grp),
            &remote,
            remotePublicKey.data,
            remotePublicKey.size);
    }
    if (error == 0) {
        error = mbedtls_ecp_check_pubkey(
            &local.MBEDTLS_PRIVATE(grp),
            &remote);
    }
    if (error == 0) {
        error = mbedtls_ecdh_compute_shared(
            &local.MBEDTLS_PRIVATE(grp),
            &shared,
            &remote,
            &local.MBEDTLS_PRIVATE(d),
            fillRandom,
            nullptr);
    }
    if (error == 0) {
        error = mbedtls_mpi_write_binary_le(
            &shared,
            sharedSecret.data,
            kX25519Size);
    }
    mbedtls_mpi_free(&shared);
    mbedtls_ecp_point_free(&remote);
    mbedtls_ecp_keypair_free(&local);
    if (error != 0) {
        secureZero(MutableByteSpan(sharedSecret.data, kX25519Size));
        return Result::failure(ErrorCode::AuthenticationRequired);
    }
    return Result::success();
#endif
}

Result Esp32MbedTlsCryptoProvider::aes256GcmEncrypt(
    ByteView key,
    ByteView nonce,
    ByteView associatedData,
    ByteView plaintext,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result validation = prepareGcm(key, nonce, associatedData);
    if (!validation) return validation;
    if (!validView(plaintext) || plaintext.size > SIZE_MAX - kGcmTagSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t required = plaintext.size + kGcmTagSize;
    if (output.data == nullptr || output.size < required) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }

#if !defined(MBEDTLS_GCM_C)
    secureZero(MutableByteSpan(output.data, required));
    return Result::failure(ErrorCode::UnsupportedFeature);
#else
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int error = mbedtls_gcm_setkey(
        &context,
        MBEDTLS_CIPHER_ID_AES,
        key.data,
        256U);
    if (error == 0) {
        error = mbedtls_gcm_crypt_and_tag(
            &context,
            MBEDTLS_GCM_ENCRYPT,
            plaintext.size,
            nonce.data,
            nonce.size,
            associatedData.data,
            associatedData.size,
            plaintext.data,
            output.data,
            kGcmTagSize,
            output.data + plaintext.size);
    }
    mbedtls_gcm_free(&context);
    if (error != 0) {
        secureZero(MutableByteSpan(output.data, required));
        return Result::failure(ErrorCode::InternalError);
    }
    written = required;
    return Result::success();
#endif
}

Result Esp32MbedTlsCryptoProvider::aes256GcmDecrypt(
    ByteView key,
    ByteView nonce,
    ByteView associatedData,
    ByteView ciphertextAndTag,
    MutableByteSpan output,
    size_t& written) {
    written = 0U;
    Result validation = prepareGcm(key, nonce, associatedData);
    if (!validation) return validation;
    if (ciphertextAndTag.data == nullptr ||
        ciphertextAndTag.size < kGcmTagSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    const size_t plaintextSize = ciphertextAndTag.size - kGcmTagSize;
    if (plaintextSize != 0U &&
        (output.data == nullptr || output.size < plaintextSize)) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
#if !defined(MBEDTLS_GCM_C)
    if (plaintextSize != 0U) {
        secureZero(MutableByteSpan(output.data, plaintextSize));
    }
    return Result::failure(ErrorCode::UnsupportedFeature);
#else
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int error = mbedtls_gcm_setkey(
        &context,
        MBEDTLS_CIPHER_ID_AES,
        key.data,
        256U);
    if (error == 0) {
        error = mbedtls_gcm_auth_decrypt(
            &context,
            plaintextSize,
            nonce.data,
            nonce.size,
            associatedData.data,
            associatedData.size,
            ciphertextAndTag.data + plaintextSize,
            kGcmTagSize,
            ciphertextAndTag.data,
            output.data);
    }
    mbedtls_gcm_free(&context);
    if (error != 0) {
        if (plaintextSize != 0U) {
            secureZero(MutableByteSpan(output.data, plaintextSize));
        }
        return Result::failure(
            error == MBEDTLS_ERR_GCM_AUTH_FAILED
                ? ErrorCode::AuthenticationRequired
                : ErrorCode::InternalError);
    }
    written = plaintextSize;
    return Result::success();
#endif
}

Result Esp32MbedTlsCryptoProvider::verifyDigest(
    ByteView publicKey,
    ByteView digest,
    ByteView signature) {
    if (!isCanonicalP256PublicKey(publicKey) ||
        digest.data == nullptr || digest.size != kP256DigestSize ||
        !isCanonicalP256Signature(signature)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

#if !defined(MBEDTLS_ECDSA_C) || !defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
    return Result::failure(ErrorCode::UnsupportedFeature);
#else
    mbedtls_ecp_group group;
    mbedtls_ecp_point point;
    mbedtls_mpi r;
    mbedtls_mpi s;
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&point);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    int error = mbedtls_ecp_group_load(
        &group,
        MBEDTLS_ECP_DP_SECP256R1);
    if (error == 0) {
        error = mbedtls_ecp_point_read_binary(
            &group,
            &point,
            publicKey.data,
            publicKey.size);
    }
    if (error == 0) error = mbedtls_ecp_check_pubkey(&group, &point);
    if (error == 0) {
        error = mbedtls_mpi_read_binary(
            &r,
            signature.data,
            kP256PrivateKeySize);
    }
    if (error == 0) {
        error = mbedtls_mpi_read_binary(
            &s,
            signature.data + kP256PrivateKeySize,
            kP256PrivateKeySize);
    }
    if (error == 0) {
        error = mbedtls_ecdsa_verify(
            &group,
            digest.data,
            digest.size,
            &point,
            &r,
            &s);
    }

    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecp_point_free(&point);
    mbedtls_ecp_group_free(&group);
    return error == 0
               ? Result::success()
               : Result::failure(ErrorCode::AuthenticationRequired);
#endif
}

} // namespace blinker

#endif // ARDUINO_ARCH_ESP32
