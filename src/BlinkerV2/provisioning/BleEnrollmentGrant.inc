#include "BleEnrollmentGrant.h"

#include "../core/SecureMemory.h"
#include "../protocol/cbor/Cbor.h"

namespace blinker {

namespace {

const char kDomain[] = "blinker.ble-enrollment.v2";

cbor::Limits enrollmentGrantLimits() {
    cbor::Limits value;
    value.maxByteStringLength = kBleEnrollmentGrantSignatureSize;
    value.maxContainerItems = 16U;
    value.maxDepth = 2U;
    return value;
}

bool enrollmentGrantExactNonZero(ByteView value, size_t expected) {
    if (value.data == nullptr || value.size != expected) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

bool validAlgorithm(ServerSignatureAlgorithm algorithm) {
    return algorithm == ServerSignatureAlgorithm::Ed25519 ||
           algorithm == ServerSignatureAlgorithm::EcdsaP256Sha256Raw;
}

bool validSecurityProfile(BleEnrollmentSecurityProfile profile) {
    return profile == BleEnrollmentSecurityProfile::EducationNearby ||
           profile == BleEnrollmentSecurityProfile::ProductOob;
}

Result validate(const BleEnrollmentGrant& grant, bool requireSignature) {
    if (!enrollmentGrantExactNonZero(
            grant.grantId, kBleEnrollmentGrantIdSize) ||
        !enrollmentGrantExactNonZero(
            grant.deviceInstanceId, kDeviceInstanceIdSize) ||
        !enrollmentGrantExactNonZero(
            grant.setupSessionId, kSetupSessionIdSize) ||
        !enrollmentGrantExactNonZero(
            grant.setupTranscriptHash, kSha256Size) ||
        grant.accessEpoch == 0U ||
        !enrollmentGrantExactNonZero(
            grant.controllerId, kControllerIdSize) ||
        !enrollmentGrantExactNonZero(
            grant.controllerSecretDigest, kSha256Size) ||
        grant.controllerPermissions != kControllerPermissionAll ||
        !enrollmentGrantExactNonZero(
            grant.nonce, kBleEnrollmentGrantNonceSize) ||
        grant.issuedAt == 0U || grant.expiresAt <= grant.issuedAt ||
        grant.expiresAt - grant.issuedAt >
            kBleEnrollmentGrantMaxLifetimeSeconds ||
        !validSecurityProfile(grant.securityProfile) ||
        grant.serverKeyId == 0U || !validAlgorithm(grant.signatureAlgorithm) ||
        (requireSignature
              ? !enrollmentGrantExactNonZero(grant.signature,
                             kBleEnrollmentGrantSignatureSize)
             : (!grant.signature.empty() &&
                grant.signature.size != kBleEnrollmentGrantSignatureSize))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

Result readEnrollmentGrantKey(cbor::Reader& reader, uint8_t expected) {
    uint64_t actual = 0U;
    Result result = reader.readUnsigned(actual);
    return result && actual != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result u32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result writeKey(cbor::Writer& writer, uint8_t value) {
    return writer.writeUnsigned(value);
}

bool same(ByteView first, ByteView second) {
    return first.size == second.size && constantTimeEqual(first, second);
}

} // namespace

BleEnrollmentGrant::BleEnrollmentGrant()
    : accessEpoch(0U), controllerPermissions(0U), issuedAt(0U),
      expiresAt(0U),
      securityProfile(BleEnrollmentSecurityProfile::EducationNearby),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519) {}

BleEnrollmentGrantContext::BleEnrollmentGrantContext()
    : accessEpoch(0U),
      securityProfile(BleEnrollmentSecurityProfile::EducationNearby),
      serverKeyId(0U),
      signatureAlgorithm(ServerSignatureAlgorithm::Ed25519),
      nowEpochSeconds(0U), hasTrustedTime(false) {}

Result encodeBleEnrollmentGrant(const BleEnrollmentGrant& grant,
                                MutableByteSpan output,
                                ByteView& encoded) {
    encoded = ByteView();
    Result result = validate(grant, true);
    cbor::Writer writer(output);
    if (result) result = writer.beginMap(16U);
    if (result) result = writeKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kBleEnrollmentGrantVersion);
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(grant.grantId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(grant.deviceInstanceId);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(grant.setupSessionId);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeBytes(grant.setupTranscriptHash);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(grant.accessEpoch);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeBytes(grant.controllerId);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(grant.controllerSecretDigest);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeUnsigned(grant.controllerPermissions);
    if (result) result = writeKey(writer, 9U);
    if (result) result = writer.writeBytes(grant.nonce);
    if (result) result = writeKey(writer, 10U);
    if (result) result = writer.writeUnsigned(grant.issuedAt);
    if (result) result = writeKey(writer, 11U);
    if (result) result = writer.writeUnsigned(grant.expiresAt);
    if (result) result = writeKey(writer, 12U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(grant.securityProfile));
    }
    if (result) result = writeKey(writer, 13U);
    if (result) result = writer.writeUnsigned(grant.serverKeyId);
    if (result) result = writeKey(writer, 14U);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(grant.signatureAlgorithm));
    }
    if (result) result = writeKey(writer, 15U);
    if (result) result = writer.writeBytes(grant.signature);
    if (result) encoded = writer.view();
    return result;
}

Result decodeBleEnrollmentGrant(ByteView encoded,
                                BleEnrollmentGrant& grant) {
    grant = BleEnrollmentGrant();
    Result result = cbor::validate(encoded, enrollmentGrantLimits());
    cbor::Reader reader(encoded, enrollmentGrantLimits());
    size_t count = 0U;
    uint64_t version = 0U;
    uint64_t securityProfile = 0U;
    uint64_t algorithm = 0U;
    BleEnrollmentGrant value;
    if (result) result = reader.readMapSize(count);
    if (result && count != 16U) result = Result::failure(ErrorCode::InvalidEncoding);
    if (result) result = readEnrollmentGrantKey(reader, 0U);
    if (result) result = reader.readUnsigned(version);
    if (result && version != kBleEnrollmentGrantVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = readEnrollmentGrantKey(reader, 1U);
    if (result) result = reader.readBytes(value.grantId);
    if (result) result = readEnrollmentGrantKey(reader, 2U);
    if (result) result = reader.readBytes(value.deviceInstanceId);
    if (result) result = readEnrollmentGrantKey(reader, 3U);
    if (result) result = reader.readBytes(value.setupSessionId);
    if (result) result = readEnrollmentGrantKey(reader, 4U);
    if (result) result = reader.readBytes(value.setupTranscriptHash);
    if (result) result = readEnrollmentGrantKey(reader, 5U);
    if (result) result = u32(reader, value.accessEpoch);
    if (result) result = readEnrollmentGrantKey(reader, 6U);
    if (result) result = reader.readBytes(value.controllerId);
    if (result) result = readEnrollmentGrantKey(reader, 7U);
    if (result) result = reader.readBytes(value.controllerSecretDigest);
    if (result) result = readEnrollmentGrantKey(reader, 8U);
    if (result) result = u32(reader, value.controllerPermissions);
    if (result) result = readEnrollmentGrantKey(reader, 9U);
    if (result) result = reader.readBytes(value.nonce);
    if (result) result = readEnrollmentGrantKey(reader, 10U);
    if (result) result = reader.readUnsigned(value.issuedAt);
    if (result) result = readEnrollmentGrantKey(reader, 11U);
    if (result) result = reader.readUnsigned(value.expiresAt);
    if (result) result = readEnrollmentGrantKey(reader, 12U);
    if (result) result = reader.readUnsigned(securityProfile);
    if (result && securityProfile > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        value.securityProfile =
            static_cast<BleEnrollmentSecurityProfile>(securityProfile);
    }
    if (result) result = readEnrollmentGrantKey(reader, 13U);
    if (result) result = u32(reader, value.serverKeyId);
    if (result) result = readEnrollmentGrantKey(reader, 14U);
    if (result) result = reader.readUnsigned(algorithm);
    if (result && algorithm > UINT8_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) {
        value.signatureAlgorithm =
            static_cast<ServerSignatureAlgorithm>(algorithm);
    }
    if (result) result = readEnrollmentGrantKey(reader, 15U);
    if (result) result = reader.readBytes(value.signature);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validate(value, true);
    if (result) grant = value;
    return result;
}

Result encodeBleEnrollmentGrantTranscript(const BleEnrollmentGrant& grant,
                                          MutableByteSpan output,
                                          ByteView& encoded) {
    encoded = ByteView();
    Result result = validate(grant, false);
    cbor::Writer writer(output);
    if (result) result = writer.beginArray(16U);
    if (result) {
        result = writer.writeText(StringView(kDomain, sizeof(kDomain) - 1U));
    }
    if (result) result = writer.writeUnsigned(kBleEnrollmentGrantVersion);
    if (result) result = writer.writeBytes(grant.grantId);
    if (result) result = writer.writeBytes(grant.deviceInstanceId);
    if (result) result = writer.writeBytes(grant.setupSessionId);
    if (result) result = writer.writeBytes(grant.setupTranscriptHash);
    if (result) result = writer.writeUnsigned(grant.accessEpoch);
    if (result) result = writer.writeBytes(grant.controllerId);
    if (result) result = writer.writeBytes(grant.controllerSecretDigest);
    if (result) result = writer.writeUnsigned(grant.controllerPermissions);
    if (result) result = writer.writeBytes(grant.nonce);
    if (result) result = writer.writeUnsigned(grant.issuedAt);
    if (result) result = writer.writeUnsigned(grant.expiresAt);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(grant.securityProfile));
    }
    if (result) result = writer.writeUnsigned(grant.serverKeyId);
    if (result) {
        result = writer.writeUnsigned(
            static_cast<uint8_t>(grant.signatureAlgorithm));
    }
    if (result) encoded = writer.view();
    return result;
}

Result BleEnrollmentGrantVerifier::verify(
    ByteView encodedGrant,
    const BleEnrollmentGrantContext& context,
    MutableByteSpan workspace,
    BleEnrollmentGrant& grant) {
    grant = BleEnrollmentGrant();
    if (!enrollmentGrantExactNonZero(
            context.deviceInstanceId, kDeviceInstanceIdSize) ||
        !enrollmentGrantExactNonZero(
            context.setupSessionId, kSetupSessionIdSize) ||
        !enrollmentGrantExactNonZero(
            context.setupTranscriptHash, kSha256Size) ||
        context.accessEpoch == 0U ||
        !validSecurityProfile(context.securityProfile) ||
        context.serverKeyId == 0U ||
        !validAlgorithm(context.signatureAlgorithm) ||
        (context.hasTrustedTime && context.nowEpochSeconds == 0U) ||
        workspace.data == nullptr ||
        workspace.size < kBleEnrollmentGrantWorkspaceSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    BleEnrollmentGrant decoded;
    Result result = decodeBleEnrollmentGrant(encodedGrant, decoded);
    if (result &&
        (!same(decoded.deviceInstanceId, context.deviceInstanceId) ||
         !same(decoded.setupSessionId, context.setupSessionId) ||
         !same(decoded.setupTranscriptHash, context.setupTranscriptHash) ||
         decoded.accessEpoch != context.accessEpoch ||
         decoded.securityProfile != context.securityProfile ||
         decoded.serverKeyId != context.serverKeyId ||
         decoded.signatureAlgorithm != context.signatureAlgorithm)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    if (result && context.hasTrustedTime &&
        (context.nowEpochSeconds < decoded.issuedAt ||
         context.nowEpochSeconds >= decoded.expiresAt)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    ByteView transcript;
    uint8_t digest[kSha256Size] = {};
    if (result) {
        result = encodeBleEnrollmentGrantTranscript(
            decoded, workspace, transcript);
    }
    if (result) {
        result = sha256(transcript, MutableByteSpan(digest, sizeof(digest)));
    }
    if (result) {
        result = signatureVerifier_.verifyDigest(
            decoded.serverKeyId,
            decoded.signatureAlgorithm,
            ByteView(digest, sizeof(digest)),
            decoded.signature);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(workspace);
    if (result) grant = decoded;
    return result;
}

} // namespace blinker
