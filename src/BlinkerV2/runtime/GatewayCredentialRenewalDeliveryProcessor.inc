#include "GatewayCredentialRenewalDeliveryProcessor.h"

#include <string.h>

#include "../core/SecureMemory.h"
#include "../security/GatewayCredentialRenewalEnvelopeSecurity.h"

namespace blinker {
namespace gateway_renewal_delivery_detail {

bool sameText(StringView first, StringView second) {
    return first.data != nullptr && second.data != nullptr &&
           first.size == second.size &&
           memcmp(first.data, second.data, first.size) == 0;
}

bool matchesOptional(StringView expected, StringView actual) {
    return expected.empty() || sameText(expected, actual);
}

bool sameBytes(const uint8_t* first, ByteView second, size_t size) {
    return first != nullptr && second.data != nullptr &&
           second.size == size &&
           constantTimeEqual(ByteView(first, size), second);
}

bool activeIdentityMatches(
    const GatewayAccessRecord& active,
    const gateway::GatewayCredentialRenewalEnvelopeView& envelope) {
    return sameBytes(active.operationId, envelope.topologyOperationId,
                     sizeof(active.operationId)) &&
           active.childLogicalDeviceIdSize ==
               envelope.childLogicalDeviceId.size &&
           memcmp(active.childLogicalDeviceId,
                  envelope.childLogicalDeviceId.data,
                  active.childLogicalDeviceIdSize) == 0 &&
           sameBytes(active.childDeviceInstanceId,
                     envelope.childDeviceInstanceId,
                     sizeof(active.childDeviceInstanceId)) &&
           active.accessEpoch == envelope.accessEpoch &&
           sameBytes(active.controllerId, envelope.controllerId,
                     sizeof(active.controllerId)) &&
           active.permissions == envelope.permissions &&
           active.activePresence.version == envelope.presenceKeyVersion &&
           active.pendingPresence.version == 0U;
}

} // namespace gateway_renewal_delivery_detail

Result GatewayCredentialRenewalDeliveryProcessor::process(
    const GatewayAccessDeliveryContext& context,
    ByteView exactEnvelope,
    MutableByteSpan ackOutput,
    size_t& ackSize) {
    namespace detail = gateway_renewal_delivery_detail;
    ackSize = 0U;
    if (ackOutput.data == nullptr ||
        ackOutput.size <
            gateway::kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize) {
        return Result::failure(ErrorCode::BufferTooSmall);
    }
    if (exactEnvelope.data == nullptr || exactEnvelope.empty() ||
        exactEnvelope.size >
            gateway::kGatewayCredentialRenewalEnvelopeMaximumEncodedSize ||
        context.deviceKeyVersion == 0U) {
        return Result::failure(ErrorCode::InvalidArgument);
    }

    gateway::GatewayCredentialRenewalEnvelopeView envelope;
    Result result = gateway::decodeGatewayCredentialRenewalEnvelope(
        exactEnvelope, envelope);
    if (result &&
        (!detail::matchesOptional(
             context.environmentId, envelope.environmentId) ||
         !detail::matchesOptional(context.tenantId, envelope.tenantId) ||
         !detail::matchesOptional(
             context.edgeHubLogicalDeviceId,
             envelope.edgeHubLogicalDeviceId) ||
         context.deviceKeyVersion != envelope.deviceKeyVersion)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    uint64_t now = 0U;
    if (result && (!clock_.unixTime(now) || now == 0U ||
                   now >= envelope.expiresAtUnixSeconds)) {
        result = Result::failure(ErrorCode::AuthenticationRequired);
    }

    GatewayAccessRecord active;
    if (result) result = active_.load(active);
    GatewayCredentialRenewalRecord existing;
    bool hasExisting = false;
    if (result) {
        const Result existingResult = pending_.load(existing);
        hasExisting = existingResult.ok();
        if (!existingResult &&
            existingResult.code() != ErrorCode::NotFound) {
            result = existingResult;
        }
    }
    if (result &&
        (!detail::activeIdentityMatches(active, envelope) ||
         (!hasExisting &&
          active.credentialVersion !=
              envelope.expectedCredentialVersion) ||
         (hasExisting &&
          active.credentialVersion != envelope.expectedCredentialVersion &&
          active.credentialVersion != envelope.credentialVersion))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }

    DeviceKey key;
    if (result) result = deviceKey_.load(key);
    uint8_t aad[
        gateway::kGatewayCredentialRenewalAadMaximumEncodedSize] = {};
    uint8_t secret[gateway::kGatewaySecretSize] = {};
    if (result) {
        result = openGatewayCredentialRenewalEnvelope(
            key, envelope,
            ByteView(active.activePresence.key,
                     sizeof(active.activePresence.key)),
            crypto_, MutableByteSpan(aad, sizeof(aad)),
            MutableByteSpan(secret, sizeof(secret)));
    }
    if (result &&
        active.credentialVersion == envelope.credentialVersion &&
        (active.expiresAtUnixSeconds != envelope.expiresAtUnixSeconds ||
         !constantTimeEqual(
             ByteView(active.secret, sizeof(active.secret)),
             ByteView(secret, sizeof(secret))) ||
         !constantTimeEqual(
             ByteView(active.accessMaterialDigest,
                      sizeof(active.accessMaterialDigest)),
             envelope.accessMaterialDigest))) {
        result = Result::failure(ErrorCode::SequenceConflict);
    }

    GatewayCredentialRenewalRecord candidate;
    if (result) {
        memcpy(candidate.renewalOperationId,
               envelope.renewalOperationId.data,
               sizeof(candidate.renewalOperationId));
        memcpy(candidate.topologyOperationId,
               envelope.topologyOperationId.data,
               sizeof(candidate.topologyOperationId));
        memcpy(candidate.childDeviceInstanceId,
               envelope.childDeviceInstanceId.data,
               sizeof(candidate.childDeviceInstanceId));
        candidate.accessEpoch = envelope.accessEpoch;
        memcpy(candidate.controllerId, envelope.controllerId.data,
               sizeof(candidate.controllerId));
        candidate.expectedCredentialVersion =
            envelope.expectedCredentialVersion;
        candidate.credentialVersion = envelope.credentialVersion;
        candidate.presenceKeyVersion = envelope.presenceKeyVersion;
        candidate.expiresAtUnixSeconds = envelope.expiresAtUnixSeconds;
        memcpy(candidate.secret, secret, sizeof(candidate.secret));
        memcpy(candidate.accessMaterialDigest,
               envelope.accessMaterialDigest.data,
               sizeof(candidate.accessMaterialDigest));
    }

    GatewayCredentialRenewalCommitOutcome outcome;
    if (result) result = pending_.commit(candidate, outcome);
    if (result) {
        gateway::GatewayCredentialRenewalDeliveryAckView ack;
        ack.renewalOperationId = ByteView(
            candidate.renewalOperationId,
            sizeof(candidate.renewalOperationId));
        ack.topologyOperationId = ByteView(
            candidate.topologyOperationId,
            sizeof(candidate.topologyOperationId));
        ack.accessMaterialDigest = ByteView(
            candidate.accessMaterialDigest,
            sizeof(candidate.accessMaterialDigest));
        ack.pendingStorageRevision = outcome.pendingStorageRevision;
        result = gateway::encodeGatewayCredentialRenewalDeliveryAck(
            ack, ackOutput, ackSize);
    }

    clearDeviceKey(key);
    clearGatewayAccessRecord(active);
    clearGatewayCredentialRenewalRecord(existing);
    clearGatewayCredentialRenewalRecord(candidate);
    secureZero(MutableByteSpan(aad, sizeof(aad)));
    secureZero(MutableByteSpan(secret, sizeof(secret)));
    if (!result) {
        secureZero(ackOutput);
        ackSize = 0U;
    }
    return result;
}

} // namespace blinker
