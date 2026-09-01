#include "PresenceKeyControlContract.h"

#include <limits.h>

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

static const uint8_t kPresenceReceiptDomain[] =
    "blinker.presence-key-receipt.v1";

cbor::Limits presenceLimits() {
    cbor::Limits limits;
    limits.maxDepth = 2U;
    limits.maxContainerItems = 7U;
    limits.maxByteStringLength = kPresenceKeyReceiptProofSize;
    limits.maxTextLength = 0U;
    return limits;
}

Result writePresenceKey(cbor::Writer& writer, uint8_t key) {
    return writer.writeUnsigned(key);
}

Result readPresenceKey(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result readPresenceU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

bool exactPresenceNonZero(ByteView value, size_t size) {
    if (value.data == nullptr || value.size != size) return false;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined != 0U;
}

Result validateVersions(
    uint32_t accessEpoch,
    uint32_t expectedVersion,
    uint32_t version) {
    return accessEpoch != 0U && expectedVersion != UINT32_MAX &&
                   version == expectedVersion + 1U
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateMutation(const PresenceKeyMutation& body) {
    Result result = validateVersions(
        body.accessEpoch,
        body.expectedVersion,
        body.presence.version);
    return result ? validateDevicePresenceKey(body.presence) : result;
}

Result validateReceipt(const PresenceKeyReceipt& body, bool proofRequired) {
    Result result = validateVersions(
        body.accessEpoch,
        body.expectedVersion,
        body.presenceKeyVersion);
    if (!result || !exactPresenceNonZero(body.keyDigest, kPresenceKeyDigestSize) ||
        body.proofKind != 1U ||
        (proofRequired
             ? !exactPresenceNonZero(body.proof, kPresenceKeyReceiptProofSize)
             : (!body.proof.empty() &&
                !exactPresenceNonZero(body.proof, kPresenceKeyReceiptProofSize)))) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    return Result::success();
}

void writeU32(uint8_t* output, uint32_t value) {
    output[0] = static_cast<uint8_t>(value >> 24U);
    output[1] = static_cast<uint8_t>(value >> 16U);
    output[2] = static_cast<uint8_t>(value >> 8U);
    output[3] = static_cast<uint8_t>(value);
}

} // namespace

Result encodePresenceKeyMutation(
    const PresenceKeyMutation& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateMutation(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(5U);
    if (result) result = writePresenceKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kPresenceKeyControlVersion);
    if (result) result = writePresenceKey(writer, 1U);
    if (result) result = writer.writeUnsigned(body.accessEpoch);
    if (result) result = writePresenceKey(writer, 2U);
    if (result) result = writer.writeUnsigned(body.expectedVersion);
    if (result) result = writePresenceKey(writer, 3U);
    if (result) result = writer.writeUnsigned(body.presence.version);
    if (result) result = writePresenceKey(writer, 4U);
    if (result) {
        result = writer.writeBytes(ByteView(
            body.presence.bytes,
            sizeof(body.presence.bytes)));
    }
    if (result) encoded = writer.view();
    return result;
}

Result decodePresenceKeyMutation(
    ByteView encoded,
    PresenceKeyMutation& body) {
    if (encoded.size > kPresenceKeyMutationMaxEncodedSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Reader reader(encoded, presenceLimits());
    Result result = cbor::validate(encoded, presenceLimits());
    size_t count = 0U;
    uint64_t version = 0U;
    PresenceKeyMutation decoded;
    ByteView key;
    if (result) result = reader.readMapSize(count);
    if (result && count != 5U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = readPresenceKey(reader, 0U);
    if (result) result = reader.readUnsigned(version);
    if (result && version != kPresenceKeyControlVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = readPresenceKey(reader, 1U);
    if (result) result = readPresenceU32(reader, decoded.accessEpoch);
    if (result) result = readPresenceKey(reader, 2U);
    if (result) result = readPresenceU32(reader, decoded.expectedVersion);
    if (result) result = readPresenceKey(reader, 3U);
    if (result) result = readPresenceU32(reader, decoded.presence.version);
    if (result) result = readPresenceKey(reader, 4U);
    if (result) result = reader.readBytes(key);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result && !exactPresenceNonZero(key, kDevicePresenceKeySize)) {
        result = Result::failure(ErrorCode::InvalidArgument);
    }
    if (result) {
        for (size_t index = 0U; index < sizeof(decoded.presence.bytes); ++index) {
            decoded.presence.bytes[index] = key.data[index];
        }
        result = validateMutation(decoded);
    }
    if (!result) {
        clearDevicePresenceKey(decoded.presence);
        return result;
    }
    body = decoded;
    clearDevicePresenceKey(decoded.presence);
    return Result::success();
}

Result encodePresenceKeyReceipt(
    const PresenceKeyReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(7U);
    if (result) result = writePresenceKey(writer, 0U);
    if (result) result = writer.writeUnsigned(kPresenceKeyControlVersion);
    if (result) result = writePresenceKey(writer, 1U);
    if (result) result = writer.writeUnsigned(body.accessEpoch);
    if (result) result = writePresenceKey(writer, 2U);
    if (result) result = writer.writeUnsigned(body.expectedVersion);
    if (result) result = writePresenceKey(writer, 3U);
    if (result) result = writer.writeUnsigned(body.presenceKeyVersion);
    if (result) result = writePresenceKey(writer, 4U);
    if (result) result = writer.writeBytes(body.keyDigest);
    if (result) result = writePresenceKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.proofKind);
    if (result) result = writePresenceKey(writer, 6U);
    if (result) result = writer.writeBytes(body.proof);
    if (result) encoded = writer.view();
    return result;
}

Result decodePresenceKeyReceipt(
    ByteView encoded,
    PresenceKeyReceipt& body) {
    if (encoded.size > kPresenceKeyReceiptMaxEncodedSize) {
        return Result::failure(ErrorCode::CapacityExceeded);
    }
    cbor::Reader reader(encoded, presenceLimits());
    Result result = cbor::validate(encoded, presenceLimits());
    size_t count = 0U;
    uint64_t version = 0U;
    uint64_t proofKind = 0U;
    PresenceKeyReceipt decoded;
    if (result) result = reader.readMapSize(count);
    if (result && count != 7U) {
        result = Result::failure(ErrorCode::InvalidEncoding);
    }
    if (result) result = readPresenceKey(reader, 0U);
    if (result) result = reader.readUnsigned(version);
    if (result && version != kPresenceKeyControlVersion) {
        result = Result::failure(ErrorCode::UnsupportedVersion);
    }
    if (result) result = readPresenceKey(reader, 1U);
    if (result) result = readPresenceU32(reader, decoded.accessEpoch);
    if (result) result = readPresenceKey(reader, 2U);
    if (result) result = readPresenceU32(reader, decoded.expectedVersion);
    if (result) result = readPresenceKey(reader, 3U);
    if (result) result = readPresenceU32(reader, decoded.presenceKeyVersion);
    if (result) result = readPresenceKey(reader, 4U);
    if (result) result = reader.readBytes(decoded.keyDigest);
    if (result) result = readPresenceKey(reader, 5U);
    if (result) result = reader.readUnsigned(proofKind);
    if (result && proofKind > 0xFFU) {
        result = Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) decoded.proofKind = static_cast<uint8_t>(proofKind);
    if (result) result = readPresenceKey(reader, 6U);
    if (result) result = reader.readBytes(decoded.proof);
    if (result && !reader.finished()) {
        result = Result::failure(ErrorCode::TrailingData);
    }
    if (result) result = validateReceipt(decoded, true);
    if (result) body = decoded;
    return result;
}

Result computePresenceKeyReceiptProof(
    const DevicePresenceKey& presence,
    const PresenceKeyReceipt& body,
    MutableByteSpan digestOutput,
    MutableByteSpan proofOutput) {
    Result result = validateDevicePresenceKey(presence);
    if (!result || digestOutput.data == nullptr ||
        digestOutput.size < kPresenceKeyDigestSize ||
        proofOutput.data == nullptr ||
        proofOutput.size < kPresenceKeyReceiptProofSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    result = validateReceipt(body, false);
    if (!result) return result;
    uint8_t metadata[12U];
    writeU32(metadata, body.accessEpoch);
    writeU32(metadata + 4U, body.expectedVersion);
    writeU32(metadata + 8U, body.presenceKeyVersion);
    result = sha256(
        ByteView(presence.bytes, sizeof(presence.bytes)),
        MutableByteSpan(digestOutput.data, kPresenceKeyDigestSize));
    if (result && body.keyDigest.data != nullptr &&
        !constantTimeEqual(
            ByteView(digestOutput.data, kPresenceKeyDigestSize),
            body.keyDigest)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    HmacSha256 hmac;
    if (result) {
        result = hmac.begin(ByteView(
            presence.bytes,
            sizeof(presence.bytes)));
    }
    if (result) {
        result = hmac.update(ByteView(
            kPresenceReceiptDomain,
            sizeof(kPresenceReceiptDomain)));
    }
    if (result) result = hmac.update(ByteView(metadata, sizeof(metadata)));
    if (result) {
        result = hmac.update(ByteView(
            digestOutput.data,
            kPresenceKeyDigestSize));
    }
    if (result) {
        result = hmac.finish(MutableByteSpan(
            proofOutput.data,
            kPresenceKeyReceiptProofSize));
    }
    hmac.clear();
    secureZero(MutableByteSpan(metadata, sizeof(metadata)));
    if (!result) {
        secureZero(MutableByteSpan(digestOutput.data, kPresenceKeyDigestSize));
        secureZero(MutableByteSpan(proofOutput.data, kPresenceKeyReceiptProofSize));
    }
    return result;
}

Result verifyPresenceKeyReceiptProof(
    const DevicePresenceKey& presence,
    const PresenceKeyReceipt& body) {
    uint8_t digest[kPresenceKeyDigestSize] = {};
    uint8_t proof[kPresenceKeyReceiptProofSize] = {};
    Result result = computePresenceKeyReceiptProof(
        presence,
        body,
        MutableByteSpan(digest, sizeof(digest)),
        MutableByteSpan(proof, sizeof(proof)));
    if (result && !constantTimeEqual(
            ByteView(proof, sizeof(proof)),
            body.proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(digest, sizeof(digest)));
    secureZero(MutableByteSpan(proof, sizeof(proof)));
    return result;
}

} // namespace blinker
