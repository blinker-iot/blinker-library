#include "GatewayAccessDeliveryProcessor.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../core/Sha256.h"
#include "../security/GatewayAccessEnvelopeSecurity.h"

namespace blinker {

namespace gateway_access_delivery_detail {

bool sameText(StringView first, StringView second) {
    return first.size == second.size && first.data != nullptr &&
           second.data != nullptr &&
           memcmp(first.data, second.data, first.size) == 0;
}

bool matchesOptional(StringView expected, StringView actual) {
    return expected.empty() || sameText(expected, actual);
}

} // namespace gateway_access_delivery_detail

Result GatewayAccessDeliveryProcessor::process(
    const GatewayAccessDeliveryContext& context,
    ByteView exactEnvelope,
    MutableByteSpan ackOutput,
    size_t& ackSize) {
    ackSize = 0U;
    if (ackOutput.data == nullptr ||
        ackOutput.size <
            gateway::kGatewayAccessDeliveryAckMaximumEncodedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (exactEnvelope.data == nullptr || exactEnvelope.empty() ||
        exactEnvelope.size >
            gateway::kGatewayAccessEnvelopeMaximumEncodedSize ||
        context.deviceKeyVersion == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    gateway::GatewayAccessEnvelopeView envelope;
    Result result = gateway::decodeGatewayAccessEnvelope(
        exactEnvelope, envelope);
    if (result &&
        (!gateway_access_delivery_detail::matchesOptional(
             context.environmentId, envelope.environmentId) ||
         !gateway_access_delivery_detail::matchesOptional(
             context.tenantId, envelope.tenantId) ||
         !gateway_access_delivery_detail::matchesOptional(
             context.edgeHubLogicalDeviceId,
             envelope.edgeHubLogicalDeviceId) ||
         context.deviceKeyVersion != envelope.deviceKeyVersion)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    uint64_t now = 0U;
    if (result && (!clock_.unixTime(now) ||
                   now >= envelope.expiresAtUnixSeconds)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    DeviceKey key;
    if (result) result = deviceKey_.load(key);
    uint8_t aad[gateway::kGatewayAccessAadMaximumEncodedSize] = {};
    uint8_t plaintext[gateway::kGatewayAccessMaterialSize] = {};
    if (result) {
        result = openGatewayAccessEnvelope(
            key, envelope, crypto_, MutableByteSpan(aad, sizeof(aad)),
            MutableByteSpan(plaintext, sizeof(plaintext)));
    }

    GatewayAccessRecord record;
    if (result) {
        result = sha256(
            exactEnvelope,
            MutableByteSpan(record.envelopeDigest,
                            sizeof(record.envelopeDigest)));
    }
    if (result) {
        result = sha256(
            ByteView(plaintext, sizeof(plaintext)),
            MutableByteSpan(record.accessMaterialDigest,
                            sizeof(record.accessMaterialDigest)));
    }
    if (result) {
        memcpy(record.operationId, envelope.operationId.data,
               sizeof(record.operationId));
        record.childLogicalDeviceIdSize = static_cast<uint8_t>(
            envelope.childLogicalDeviceId.size);
        memcpy(record.childLogicalDeviceId,
               envelope.childLogicalDeviceId.data,
               envelope.childLogicalDeviceId.size);
        memcpy(record.childDeviceInstanceId,
               envelope.childDeviceInstanceId.data,
               sizeof(record.childDeviceInstanceId));
        record.accessEpoch = envelope.accessEpoch;
        memcpy(record.controllerId, envelope.controllerId.data,
               sizeof(record.controllerId));
        record.credentialVersion = envelope.credentialVersion;
        record.permissions = envelope.permissions;
        record.expiresAtUnixSeconds = envelope.expiresAtUnixSeconds;
        memcpy(record.secret, plaintext, sizeof(record.secret));
        record.activePresence.version = envelope.presenceKeyVersion;
        memcpy(
            record.activePresence.key,
            plaintext + gateway::kGatewaySecretSize,
            sizeof(record.activePresence.key));
    }

    GatewayAccessCommitOutcome outcome;
    if (result) result = access_.commit(record, outcome);
    if (result) {
        gateway::GatewayAccessDeliveryAckView ack;
        ack.operationId = ByteView(record.operationId,
                                   sizeof(record.operationId));
        ack.accessMaterialDigest = ByteView(
            record.accessMaterialDigest,
            sizeof(record.accessMaterialDigest));
        ack.storageRevision = outcome.storageRevision;
        result = gateway::encodeGatewayAccessDeliveryAck(
            ack, ackOutput, ackSize);
    }

    clearDeviceKey(key);
    clearGatewayAccessRecord(record);
    secureZero(MutableByteSpan(aad, sizeof(aad)));
    secureZero(MutableByteSpan(plaintext, sizeof(plaintext)));
    if (!result) {
        secureZero(ackOutput);
        ackSize = 0U;
    }
    return result;
}

} // namespace blinker
