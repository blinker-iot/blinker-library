#include "LocalControllerPairingContract.h"

#include "../core/SecureMemory.h"

namespace blinker {

namespace {

const char kReceiptDomain[] = "blinker.local-controller-receipt.v1";

cbor::Limits contractLimits() {
    cbor::Limits limits;
    limits.maxTextLength = sizeof(kReceiptDomain) - 1U;
    limits.maxByteStringLength = kControllerCredentialSecretSize;
    limits.maxContainerItems = 9U;
    limits.maxDepth = 2U;
    return limits;
}

bool exactBytes(ByteView value, size_t expected) {
    return value.data != nullptr && value.size == expected;
}

bool allZero(ByteView value) {
    if (value.data == nullptr || value.empty()) return true;
    uint8_t combined = 0U;
    for (size_t index = 0U; index < value.size; ++index) {
        combined = static_cast<uint8_t>(combined | value.data[index]);
    }
    return combined == 0U;
}

bool nonZeroExact(ByteView value, size_t expected) {
    return exactBytes(value, expected) && !allZero(value);
}

Result validateChallenge(const LocalControllerPairingChallenge& body) {
    return nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   nonZeroExact(
                       body.pairingNonce,
                       kLocalControllerPairingNonceSize) &&
                   nonZeroExact(
                       body.setupTranscriptHash,
                       kLocalControllerPairingTranscriptHashSize) &&
                   validAuthorizationPermissions(body.permissions) &&
                   body.expiresInMillis != 0U &&
                   body.expiresInMillis <=
                       kLocalControllerPairingMaxWindowMillis
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateRequest(const LocalControllerPairingRequest& body) {
    return nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) &&
                   nonZeroExact(
                       body.pairingNonce,
                       kLocalControllerPairingNonceSize) &&
                   nonZeroExact(
                       body.setupTranscriptHash,
                       kLocalControllerPairingTranscriptHashSize) &&
                   nonZeroExact(body.controllerId, kControllerIdSize) &&
                   body.credentialVersion == 1U &&
                   validAuthorizationPermissions(body.permissions) &&
                   nonZeroExact(
                       body.controllerSecret,
                       kControllerCredentialSecretSize)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result validateReceipt(const LocalControllerPairingReceipt& body,
                       bool requireProof) {
    if (!nonZeroExact(body.deviceInstanceId, kDeviceInstanceIdSize) ||
        !nonZeroExact(
            body.pairingNonce,
            kLocalControllerPairingNonceSize) ||
        !nonZeroExact(
            body.requestDigest,
            kLocalControllerPairingRequestDigestSize) ||
        !nonZeroExact(body.controllerId, kControllerIdSize) ||
        body.credentialVersion != 1U ||
        !validAuthorizationPermissions(body.permissions) ||
        !nonZeroExact(body.secretDigest, kSha256Size)) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    if (requireProof) {
        return exactBytes(body.proof, kLocalControllerPairingProofSize)
                   ? Result::success()
                   : Result::failure(ErrorCode::InvalidArgument);
    }
    return body.proof.empty() ||
                   exactBytes(body.proof, kLocalControllerPairingProofSize)
               ? Result::success()
               : Result::failure(ErrorCode::InvalidArgument);
}

Result writeKey(cbor::Writer& writer, uint8_t key) {
    return writer.writeUnsigned(key);
}

Result readKey(cbor::Reader& reader, uint8_t expected) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    return result && value != expected
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result readU32(cbor::Reader& reader, uint32_t& output) {
    uint64_t value = 0U;
    Result result = reader.readUnsigned(value);
    if (result && value > UINT32_MAX) {
        return Result::failure(ErrorCode::ValueOutOfRange);
    }
    if (result) output = static_cast<uint32_t>(value);
    return result;
}

Result beginMapDecode(ByteView encoded,
                      cbor::Reader& reader,
                      size_t expectedCount) {
    Result result = cbor::validate(encoded, contractLimits());
    size_t count = 0U;
    if (result) result = reader.readMapSize(count);
    return result && count != expectedCount
               ? Result::failure(ErrorCode::InvalidEncoding)
               : result;
}

Result finishDecode(cbor::Reader& reader) {
    return reader.finished()
               ? Result::success()
               : Result::failure(ErrorCode::TrailingData);
}

Result readVersion(cbor::Reader& reader) {
    Result result = readKey(reader, 0U);
    uint64_t version = 0U;
    if (result) result = reader.readUnsigned(version);
    return result && version != kLocalControllerPairingContractVersion
               ? Result::failure(ErrorCode::UnsupportedVersion)
               : result;
}

Result writeReceiptTranscript(
    cbor::Writer& writer,
    const LocalControllerPairingReceipt& body) {
    Result result = validateReceipt(body, false);
    if (!result) return result;
    result = writer.beginArray(9U);
    if (result) {
        result = writer.writeText(
            StringView(kReceiptDomain, sizeof(kReceiptDomain) - 1U));
    }
    if (result) {
        result = writer.writeUnsigned(
            kLocalControllerPairingContractVersion);
    }
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writer.writeBytes(body.pairingNonce);
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writer.writeBytes(body.secretDigest);
    return result;
}

Result hmacSink(void* context, ByteView bytes) {
    return static_cast<HmacSha256*>(context)->update(bytes);
}

} // namespace

Result encodeLocalControllerPairingChallenge(
    const LocalControllerPairingChallenge& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateChallenge(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(6U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kLocalControllerPairingContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.pairingNonce);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.setupTranscriptHash);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.expiresInMillis);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeLocalControllerPairingChallenge(
    ByteView encoded,
    LocalControllerPairingChallenge& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 6U);
    LocalControllerPairingChallenge decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.pairingNonce);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.setupTranscriptHash);
    if (result) result = readKey(reader, 4U);
    if (result) result = readU32(reader, decoded.permissions);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.expiresInMillis);
    if (result) result = finishDecode(reader);
    if (result) result = validateChallenge(decoded);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeLocalControllerPairingRequest(
    const LocalControllerPairingRequest& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateRequest(body);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(8U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kLocalControllerPairingContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.pairingNonce);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.setupTranscriptHash);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.controllerSecret);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeLocalControllerPairingRequest(
    ByteView encoded,
    LocalControllerPairingRequest& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 8U);
    LocalControllerPairingRequest decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.pairingNonce);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.setupTranscriptHash);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readBytes(decoded.controllerId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.permissions);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.controllerSecret);
    if (result) result = finishDecode(reader);
    if (result) result = validateRequest(decoded);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeLocalControllerPairingReceipt(
    const LocalControllerPairingReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    Result result = validateReceipt(body, true);
    if (!result) return result;
    cbor::Writer writer(output);
    result = writer.beginMap(9U);
    if (result) result = writeKey(writer, 0U);
    if (result) {
        result = writer.writeUnsigned(
            kLocalControllerPairingContractVersion);
    }
    if (result) result = writeKey(writer, 1U);
    if (result) result = writer.writeBytes(body.deviceInstanceId);
    if (result) result = writeKey(writer, 2U);
    if (result) result = writer.writeBytes(body.pairingNonce);
    if (result) result = writeKey(writer, 3U);
    if (result) result = writer.writeBytes(body.requestDigest);
    if (result) result = writeKey(writer, 4U);
    if (result) result = writer.writeBytes(body.controllerId);
    if (result) result = writeKey(writer, 5U);
    if (result) result = writer.writeUnsigned(body.credentialVersion);
    if (result) result = writeKey(writer, 6U);
    if (result) result = writer.writeUnsigned(body.permissions);
    if (result) result = writeKey(writer, 7U);
    if (result) result = writer.writeBytes(body.secretDigest);
    if (result) result = writeKey(writer, 8U);
    if (result) result = writer.writeBytes(body.proof);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result decodeLocalControllerPairingReceipt(
    ByteView encoded,
    LocalControllerPairingReceipt& body) {
    cbor::Reader reader(encoded, contractLimits());
    Result result = beginMapDecode(encoded, reader, 9U);
    LocalControllerPairingReceipt decoded;
    if (result) result = readVersion(reader);
    if (result) result = readKey(reader, 1U);
    if (result) result = reader.readBytes(decoded.deviceInstanceId);
    if (result) result = readKey(reader, 2U);
    if (result) result = reader.readBytes(decoded.pairingNonce);
    if (result) result = readKey(reader, 3U);
    if (result) result = reader.readBytes(decoded.requestDigest);
    if (result) result = readKey(reader, 4U);
    if (result) result = reader.readBytes(decoded.controllerId);
    if (result) result = readKey(reader, 5U);
    if (result) result = readU32(reader, decoded.credentialVersion);
    if (result) result = readKey(reader, 6U);
    if (result) result = readU32(reader, decoded.permissions);
    if (result) result = readKey(reader, 7U);
    if (result) result = reader.readBytes(decoded.secretDigest);
    if (result) result = readKey(reader, 8U);
    if (result) result = reader.readBytes(decoded.proof);
    if (result) result = finishDecode(reader);
    if (result) result = validateReceipt(decoded, true);
    if (!result) return result;
    body = decoded;
    return Result::success();
}

Result encodeLocalControllerPairingReceiptTranscript(
    const LocalControllerPairingReceipt& body,
    MutableByteSpan output,
    ByteView& encoded) {
    encoded = ByteView();
    cbor::Writer writer(output);
    Result result = writeReceiptTranscript(writer, body);
    if (!result) return result;
    encoded = writer.view();
    return Result::success();
}

Result computeLocalControllerPairingReceiptProof(
    ByteView controllerSecret,
    const LocalControllerPairingReceipt& body,
    MutableByteSpan proofOutput) {
    if (!nonZeroExact(
            controllerSecret,
            kControllerCredentialSecretSize) ||
        proofOutput.data == nullptr ||
        proofOutput.size < kLocalControllerPairingProofSize) {
        return Result::failure(ErrorCode::InvalidArgument);
    }
    HmacSha256 hmac;
    Result result = hmac.begin(controllerSecret);
    cbor::Writer writer(&hmacSink, &hmac);
    if (result) result = writeReceiptTranscript(writer, body);
    if (result) {
        result = hmac.finish(MutableByteSpan(
            proofOutput.data,
            kLocalControllerPairingProofSize));
    }
    if (!result) hmac.clear();
    return result;
}

Result verifyLocalControllerPairingReceiptProof(
    ByteView controllerSecret,
    const LocalControllerPairingReceipt& body) {
    Result result = validateReceipt(body, true);
    uint8_t expected[kLocalControllerPairingProofSize] = {};
    if (result) {
        result = computeLocalControllerPairingReceiptProof(
            controllerSecret,
            body,
            MutableByteSpan(expected, sizeof(expected)));
    }
    if (result && !constantTimeEqual(
            ByteView(expected, sizeof(expected)), body.proof)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }
    secureZero(MutableByteSpan(expected, sizeof(expected)));
    return result;
}

} // namespace blinker
