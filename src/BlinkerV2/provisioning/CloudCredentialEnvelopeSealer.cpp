#include "CloudCredentialEnvelopeSealer.h"

#include <string.h>

#include "../core/HkdfSha256.h"
#include "../core/SecureMemory.h"

namespace blinker {

namespace {

static const char kHkdfSaltDomain[] =
    "blinker.cloud-credential-envelope.hkdf-salt.v1";

} // namespace

Result CloudCredentialEnvelopeSealer::seal(
    CloudCredentialEnrollmentRequest& request,
    ByteView ephemeralPrivateKey,
    ByteView cloudSecret,
    MutableByteSpan transcriptWorkspace,
    MutableByteSpan ephemeralPublicKeyOutput,
    MutableByteSpan nonceOutput,
    MutableByteSpan encryptedCredentialOutput) {
    if (ephemeralPrivateKey.data == nullptr ||
        ephemeralPrivateKey.size != kCloudEnrollmentEphemeralPrivateKeySize ||
        cloudSecret.data == nullptr ||
        cloudSecret.size != kCloudCredentialSecretSize ||
        transcriptWorkspace.data == nullptr ||
        transcriptWorkspace.size < kCloudEnrollmentWorkspaceSize ||
        ephemeralPublicKeyOutput.data == nullptr ||
        ephemeralPublicKeyOutput.size <
            kCloudEnrollmentEphemeralPublicKeySize ||
        nonceOutput.data == nullptr ||
        nonceOutput.size < kCloudEnrollmentNonceSize ||
        encryptedCredentialOutput.data == nullptr ||
        encryptedCredentialOutput.size <
            kCloudEnrollmentEncryptedCredentialSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    uint8_t serverPublicKey[kX25519KeySize] = {};
    uint8_t sharedSecret[kX25519KeySize] = {};
    uint8_t salt[kSha256Size] = {};
    uint8_t transcriptDigest[kSha256Size] = {};
    uint8_t encryptionKey[kAes256KeySize] = {};

    Result result = keys_.loadPublicKey(
        request.encryptionKeyId,
        MutableByteSpan(serverPublicKey, sizeof(serverPublicKey)));
    if (result) {
        result = crypto_.x25519PublicKey(
            ephemeralPrivateKey,
            MutableByteSpan(
                ephemeralPublicKeyOutput.data,
                kCloudEnrollmentEphemeralPublicKeySize));
    }
    request.ephemeralPublicKey = ByteView(
        ephemeralPublicKeyOutput.data,
        kCloudEnrollmentEphemeralPublicKeySize);
    ByteView transcript;
    if (result) {
        result = encodeCloudCredentialEnvelopeTranscript(
            request,
            transcriptWorkspace,
            transcript);
    }
    if (result) {
        result = sha256(
            transcript,
            MutableByteSpan(transcriptDigest, sizeof(transcriptDigest)));
    }
    if (result) {
        memcpy(
            nonceOutput.data,
            transcriptDigest,
            kCloudEnrollmentNonceSize);
        request.nonce = ByteView(nonceOutput.data, kCloudEnrollmentNonceSize);
        result = crypto_.x25519(
            ephemeralPrivateKey,
            ByteView(serverPublicKey, sizeof(serverPublicKey)),
            MutableByteSpan(sharedSecret, sizeof(sharedSecret)));
    }
    if (result) {
        result = sha256(
            ByteView(
                reinterpret_cast<const uint8_t*>(kHkdfSaltDomain),
                sizeof(kHkdfSaltDomain) - 1U),
            MutableByteSpan(salt, sizeof(salt)));
    }
    if (result) {
        result = hkdfSha256(
            ByteView(salt, sizeof(salt)),
            ByteView(sharedSecret, sizeof(sharedSecret)),
            ByteView(transcriptDigest, sizeof(transcriptDigest)),
            MutableByteSpan(encryptionKey, sizeof(encryptionKey)));
    }
    size_t written = 0U;
    if (result) {
        result = crypto_.aes256GcmEncrypt(
            ByteView(encryptionKey, sizeof(encryptionKey)),
            request.nonce,
            transcript,
            cloudSecret,
            MutableByteSpan(
                encryptedCredentialOutput.data,
                kCloudEnrollmentEncryptedCredentialSize),
            written);
    }
    if (result && written != kCloudEnrollmentEncryptedCredentialSize) {
        result = Result::failure(ErrorCode::InternalError);
    }
    if (result) {
        request.encryptedCredential = ByteView(
            encryptedCredentialOutput.data,
            kCloudEnrollmentEncryptedCredentialSize);
    } else {
        request.ephemeralPublicKey = ByteView();
        request.nonce = ByteView();
        request.encryptedCredential = ByteView();
        secureZero(MutableByteSpan(
            ephemeralPublicKeyOutput.data,
            kCloudEnrollmentEphemeralPublicKeySize));
        secureZero(MutableByteSpan(
            nonceOutput.data,
            kCloudEnrollmentNonceSize));
        secureZero(MutableByteSpan(
            encryptedCredentialOutput.data,
            kCloudEnrollmentEncryptedCredentialSize));
    }

    secureZero(MutableByteSpan(serverPublicKey, sizeof(serverPublicKey)));
    secureZero(MutableByteSpan(sharedSecret, sizeof(sharedSecret)));
    secureZero(MutableByteSpan(salt, sizeof(salt)));
    secureZero(MutableByteSpan(transcriptDigest, sizeof(transcriptDigest)));
    secureZero(MutableByteSpan(encryptionKey, sizeof(encryptionKey)));
    return result;
}

} // namespace blinker
