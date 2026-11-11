#include <BlinkerV2/core/SecureMemory.h>
#include <BlinkerV2/protocol/gateway/Contracts.h>
#include <BlinkerV2/security/GatewayAccessEnvelopeSecurity.h>
#include <BlinkerV2/security/GatewayCredentialRenewalEnvelopeSecurity.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

typedef std::map<std::string, std::string> Properties;

void fail(const std::string& message) {
    std::cerr << "edge gateway golden: " << message << std::endl;
    std::exit(1);
}

void check(bool condition, const std::string& message) {
    if (!condition) fail(message);
}

Properties load(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) fail("cannot open fixture");
    Properties output;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line[line.size() - 1U] == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t split = line.find('=');
        if (split == std::string::npos || split == 0U ||
            output.find(line.substr(0U, split)) != output.end()) {
            fail("invalid fixture line");
        }
        output[line.substr(0U, split)] = line.substr(split + 1U);
    }
    return output;
}

const std::string& required(const Properties& values, const char* key) {
    Properties::const_iterator found = values.find(key);
    if (found == values.end()) fail(std::string("missing property: ") + key);
    return found->second;
}

uint64_t number(const Properties& values, const char* key) {
    const std::string& text = required(values, key);
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    check(end != text.c_str() && *end == '\0', "invalid number");
    return static_cast<uint64_t>(parsed);
}

uint8_t nibble(char value) {
    if (value >= '0' && value <= '9') return static_cast<uint8_t>(value - '0');
    if (value >= 'a' && value <= 'f') {
        return static_cast<uint8_t>(value - 'a' + 10);
    }
    fail("invalid hex");
    return 0U;
}

std::vector<uint8_t> hex(const std::string& value) {
    check((value.size() & 1U) == 0U, "odd hex length");
    std::vector<uint8_t> output(value.size() / 2U);
    for (size_t index = 0U; index < output.size(); ++index) {
        output[index] = static_cast<uint8_t>(
            (nibble(value[index * 2U]) << 4U) |
            nibble(value[index * 2U + 1U]));
    }
    return output;
}

blinker::ByteView view(const std::vector<uint8_t>& value) {
    return value.empty() ? blinker::ByteView()
                         : blinker::ByteView(&value[0], value.size());
}

blinker::StringView text(const std::string& value) {
    return blinker::StringView(value.data(), value.size());
}

void expect(
    blinker::ByteView actual,
    const Properties& values,
    const char* key) {
    const std::vector<uint8_t> expected = hex(required(values, key));
    check(actual.size == expected.size() &&
          (actual.empty() || std::memcmp(
              actual.data, &expected[0], actual.size) == 0),
          std::string("mismatch: ") + key);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) fail("usage: edge_gateway_golden <fixture>");
    const Properties values = load(argv[1]);
    const std::vector<uint8_t> operation = hex(required(values, "operation_id"));
    const std::vector<uint8_t> instance = hex(required(values, "child_device_instance_id"));
    const std::vector<uint8_t> controller = hex(required(values, "controller_id"));
    const std::vector<uint8_t> nonce = hex(required(values, "nonce"));
    const std::vector<uint8_t> ciphertext = hex(required(values, "ciphertext"));
    const std::vector<uint8_t> tag = hex(required(values, "tag"));

    blinker::gateway::GatewayAccessEnvelopeView envelope;
    envelope.operationId = view(operation);
    envelope.environmentId = text(required(values, "environment_id"));
    envelope.tenantId = text(required(values, "tenant_id"));
    envelope.edgeHubLogicalDeviceId = text(required(
        values, "edge_hub_logical_device_id"));
    envelope.deviceKeyVersion = static_cast<uint32_t>(number(
        values, "device_key_version"));
    envelope.childLogicalDeviceId = text(required(
        values, "child_logical_device_id"));
    envelope.childDeviceInstanceId = view(instance);
    envelope.accessEpoch = static_cast<uint32_t>(number(values, "access_epoch"));
    envelope.controllerId = view(controller);
    envelope.credentialVersion = static_cast<uint32_t>(number(
        values, "credential_version"));
    envelope.permissions = static_cast<uint32_t>(number(values, "permissions"));
    envelope.presenceKeyVersion = static_cast<uint32_t>(number(
        values, "presence_key_version"));
    envelope.expiresAtUnixSeconds = number(values, "expires_at_unix_seconds");
    envelope.nonce = view(nonce);
    envelope.ciphertext = view(ciphertext);
    envelope.tag = view(tag);

    uint8_t encoded[blinker::gateway::kGatewayAccessEnvelopeMaximumEncodedSize] = {};
    size_t written = 0U;
    check(blinker::gateway::encodeGatewayAccessEnvelope(
        envelope, blinker::MutableByteSpan(encoded, sizeof(encoded)), written).ok(),
        "envelope encode failed");
    expect(blinker::ByteView(encoded, written), values, "gateway_access_envelope_hex");
    blinker::gateway::GatewayAccessEnvelopeView decoded;
    check(blinker::gateway::decodeGatewayAccessEnvelope(
        blinker::ByteView(encoded, written), decoded).ok(), "envelope decode failed");
    check(decoded.accessEpoch == envelope.accessEpoch &&
          decoded.credentialVersion == envelope.credentialVersion &&
          decoded.expiresAtUnixSeconds == envelope.expiresAtUnixSeconds,
          "envelope round trip mismatch");

    uint8_t aad[blinker::gateway::kGatewayAccessAadMaximumEncodedSize] = {};
    size_t aadSize = 0U;
    check(blinker::gateway::encodeGatewayAccessAad(
        envelope, blinker::MutableByteSpan(aad, sizeof(aad)), aadSize).ok(),
        "AAD encode failed");
    expect(blinker::ByteView(aad, aadSize), values, "gateway_access_aad_hex");

    const std::vector<uint8_t> renewalOperation = hex(required(
        values, "renewal_operation_id"));
    const std::vector<uint8_t> renewalNonce = hex(required(
        values, "renewal_nonce"));
    const std::vector<uint8_t> renewalCiphertext = hex(required(
        values, "renewal_ciphertext"));
    const std::vector<uint8_t> renewalTag = hex(required(
        values, "renewal_tag"));
    const std::vector<uint8_t> renewalMaterialDigest = hex(required(
        values, "renewal_access_material_digest"));
    blinker::gateway::GatewayCredentialRenewalEnvelopeView renewal;
    renewal.renewalOperationId = view(renewalOperation);
    renewal.topologyOperationId = envelope.operationId;
    renewal.environmentId = envelope.environmentId;
    renewal.tenantId = envelope.tenantId;
    renewal.edgeHubLogicalDeviceId = envelope.edgeHubLogicalDeviceId;
    renewal.deviceKeyVersion = envelope.deviceKeyVersion;
    renewal.childLogicalDeviceId = envelope.childLogicalDeviceId;
    renewal.childDeviceInstanceId = envelope.childDeviceInstanceId;
    renewal.accessEpoch = envelope.accessEpoch;
    renewal.controllerId = envelope.controllerId;
    renewal.expectedCredentialVersion = static_cast<uint32_t>(number(
        values, "renewal_expected_credential_version"));
    renewal.credentialVersion = static_cast<uint32_t>(number(
        values, "renewal_credential_version"));
    renewal.permissions = envelope.permissions;
    renewal.presenceKeyVersion = static_cast<uint32_t>(number(
        values, "renewal_presence_key_version"));
    renewal.accessMaterialDigest = view(renewalMaterialDigest);
    renewal.expiresAtUnixSeconds = number(
        values, "renewal_expires_at_unix_seconds");
    renewal.nonce = view(renewalNonce);
    renewal.ciphertext = view(renewalCiphertext);
    renewal.tag = view(renewalTag);

    uint8_t renewalBytes[
        blinker::gateway::
            kGatewayCredentialRenewalEnvelopeMaximumEncodedSize] = {};
    size_t renewalSize = 0U;
    check(blinker::gateway::encodeGatewayCredentialRenewalEnvelope(
        renewal, blinker::MutableByteSpan(
            renewalBytes, sizeof(renewalBytes)), renewalSize).ok(),
        "renewal envelope encode failed");
    expect(blinker::ByteView(renewalBytes, renewalSize), values,
           "gateway_credential_renewal_envelope_hex");
    blinker::gateway::GatewayCredentialRenewalEnvelopeView decodedRenewal;
    check(blinker::gateway::decodeGatewayCredentialRenewalEnvelope(
        blinker::ByteView(renewalBytes, renewalSize), decodedRenewal).ok(),
        "renewal envelope decode failed");
    check(decodedRenewal.expectedCredentialVersion ==
              renewal.expectedCredentialVersion &&
          decodedRenewal.credentialVersion == renewal.credentialVersion,
          "renewal envelope round trip mismatch");

    uint8_t renewalAad[
        blinker::gateway::
            kGatewayCredentialRenewalAadMaximumEncodedSize] = {};
    size_t renewalAadSize = 0U;
    check(blinker::gateway::encodeGatewayCredentialRenewalAad(
        renewal, blinker::MutableByteSpan(
            renewalAad, sizeof(renewalAad)), renewalAadSize).ok(),
        "renewal AAD encode failed");
    expect(blinker::ByteView(renewalAad, renewalAadSize), values,
           "gateway_credential_renewal_aad_hex");

    blinker::gateway::GatewayCredentialRenewalDeliveryAckView renewalAck;
    renewalAck.renewalOperationId = renewal.renewalOperationId;
    renewalAck.topologyOperationId = renewal.topologyOperationId;
    renewalAck.accessMaterialDigest = renewal.accessMaterialDigest;
    renewalAck.pendingStorageRevision = static_cast<uint32_t>(number(
        values, "renewal_pending_storage_revision"));
    uint8_t renewalAckBytes[
        blinker::gateway::
            kGatewayCredentialRenewalDeliveryAckMaximumEncodedSize] = {};
    size_t renewalAckSize = 0U;
    check(blinker::gateway::encodeGatewayCredentialRenewalDeliveryAck(
        renewalAck, blinker::MutableByteSpan(
            renewalAckBytes, sizeof(renewalAckBytes)), renewalAckSize).ok(),
        "renewal delivery ACK encode failed");
    expect(blinker::ByteView(renewalAckBytes, renewalAckSize), values,
           "gateway_credential_renewal_delivery_ack_hex");
    blinker::gateway::GatewayCredentialRenewalDeliveryAckView
        decodedRenewalAck;
    check(blinker::gateway::decodeGatewayCredentialRenewalDeliveryAck(
        blinker::ByteView(renewalAckBytes, renewalAckSize),
        decodedRenewalAck).ok(),
        "renewal delivery ACK decode failed");

    const std::vector<uint8_t> empty;
    blinker::gateway::GatewayCredentialRenewalCommandView renewalPrepare;
    renewalPrepare.renewalOperationId = renewal.renewalOperationId;
    renewalPrepare.topologyOperationId = renewal.topologyOperationId;
    renewalPrepare.topologyVersion = static_cast<uint32_t>(number(
        values, "topology_version"));
    renewalPrepare.childDeviceInstanceId = renewal.childDeviceInstanceId;
    renewalPrepare.accessEpoch = renewal.accessEpoch;
    renewalPrepare.controllerId = renewal.controllerId;
    renewalPrepare.expectedCredentialVersion =
        renewal.expectedCredentialVersion;
    renewalPrepare.credentialVersion = renewal.credentialVersion;
    renewalPrepare.pendingStorageRevision =
        renewalAck.pendingStorageRevision;
    renewalPrepare.activeStorageRevision = 0U;
    renewalPrepare.accessMaterialDigest = renewal.accessMaterialDigest;
    renewalPrepare.phase =
        blinker::gateway::GatewayCredentialRenewalPhase::Prepare;
    renewalPrepare.grant = view(empty);
    renewalPrepare.receiptDigest = view(empty);
    renewalPrepare.expiresAtUnixSeconds = renewal.expiresAtUnixSeconds;
    uint8_t renewalCommandBytes[
        blinker::gateway::
            kGatewayCredentialRenewalCommandMaximumEncodedSize] = {};
    size_t renewalCommandSize = 0U;
    check(blinker::gateway::encodeGatewayCredentialRenewalCommand(
        renewalPrepare, blinker::MutableByteSpan(
            renewalCommandBytes, sizeof(renewalCommandBytes)),
        renewalCommandSize).ok(), "renewal Prepare encode failed");
    expect(blinker::ByteView(renewalCommandBytes, renewalCommandSize), values,
           "gateway_credential_renewal_prepare_command_hex");
    blinker::gateway::GatewayCredentialRenewalCommandView
        decodedRenewalCommand;
    check(blinker::gateway::decodeGatewayCredentialRenewalCommand(
        blinker::ByteView(renewalCommandBytes, renewalCommandSize),
        decodedRenewalCommand).ok(), "renewal Prepare decode failed");

    const std::vector<uint8_t> renewalReceipt = hex(required(
        values, "renewal_receipt"));
    const std::vector<uint8_t> renewalReceiptDigest = hex(required(
        values, "renewal_receipt_digest"));
    blinker::gateway::GatewayCredentialRenewalResultView renewalRotated;
    renewalRotated.renewalOperationId = renewal.renewalOperationId;
    renewalRotated.topologyOperationId = renewal.topologyOperationId;
    renewalRotated.topologyVersion = renewalPrepare.topologyVersion;
    renewalRotated.phase =
        blinker::gateway::GatewayCredentialRenewalPhase::Apply;
    renewalRotated.status =
        blinker::gateway::GatewayCredentialRenewalStatus::Rotated;
    renewalRotated.pendingStorageRevision =
        renewalAck.pendingStorageRevision;
    renewalRotated.activeStorageRevision = static_cast<uint32_t>(number(
        values, "renewal_active_storage_revision"));
    renewalRotated.credentialVersion = renewal.credentialVersion;
    renewalRotated.accessMaterialDigest = renewal.accessMaterialDigest;
    renewalRotated.payload = view(renewalReceipt);
    renewalRotated.receiptDigest = view(renewalReceiptDigest);
    uint8_t renewalResultBytes[
        blinker::gateway::
            kGatewayCredentialRenewalResultMaximumEncodedSize] = {};
    size_t renewalResultSize = 0U;
    check(blinker::gateway::encodeGatewayCredentialRenewalResult(
        renewalRotated, blinker::MutableByteSpan(
            renewalResultBytes, sizeof(renewalResultBytes)),
        renewalResultSize).ok(), "renewal Rotated result encode failed");
    expect(blinker::ByteView(renewalResultBytes, renewalResultSize), values,
           "gateway_credential_renewal_rotated_result_hex");
    blinker::gateway::GatewayCredentialRenewalResultView
        decodedRenewalResult;
    check(blinker::gateway::decodeGatewayCredentialRenewalResult(
        blinker::ByteView(renewalResultBytes, renewalResultSize),
        decodedRenewalResult).ok(),
        "renewal Rotated result decode failed");
    check(decodedRenewalResult.status ==
              blinker::gateway::GatewayCredentialRenewalStatus::Rotated,
          "renewal result round trip mismatch");

    blinker::gateway::GatewayCredentialRenewalEnvelopeView invalidRenewal =
        renewal;
    ++invalidRenewal.credentialVersion;
    check(!blinker::gateway::encodeGatewayCredentialRenewalEnvelope(
        invalidRenewal, blinker::MutableByteSpan(
            renewalBytes, sizeof(renewalBytes)), renewalSize).ok(),
        "credential version jump accepted");

    const std::vector<uint8_t> wrapDeviceKey = hex(required(
        values, "wrap_device_key"));
    check(wrapDeviceKey.size() == blinker::kDeviceKeySize,
          "wrap DeviceKey size mismatch");
    blinker::DeviceKey deviceKey;
    std::memcpy(deviceKey.bytes, &wrapDeviceKey[0], sizeof(deviceKey.bytes));
    blinker::GatewayAccessWrapContext wrapContext;
    wrapContext.environmentId = text(required(values, "wrap_environment_id"));
    wrapContext.tenantId = text(required(values, "wrap_tenant_id"));
    wrapContext.edgeHubLogicalDeviceId = text(required(
        values, "wrap_edge_hub_logical_device_id"));
    wrapContext.deviceKeyVersion = static_cast<uint32_t>(number(
        values, "wrap_device_key_version"));
    uint8_t wrapKey[blinker::kAes256KeySize] = {};
    check(blinker::deriveGatewayAccessWrapKey(
              deviceKey, wrapContext,
              blinker::MutableByteSpan(wrapKey, sizeof(wrapKey))).ok(),
          "wrap key derivation failed");
    expect(blinker::ByteView(wrapKey, sizeof(wrapKey)), values, "wrap_key");
    check(blinker::deriveGatewayCredentialRenewalWrapKey(
              deviceKey, wrapContext,
              blinker::MutableByteSpan(wrapKey, sizeof(wrapKey))).ok(),
          "renewal wrap key derivation failed");
    expect(blinker::ByteView(wrapKey, sizeof(wrapKey)), values,
           "renewal_wrap_key");
    blinker::clearDeviceKey(deviceKey);
    blinker::secureZero(blinker::MutableByteSpan(wrapKey, sizeof(wrapKey)));

    const std::vector<uint8_t> accessMaterialDigest = hex(required(
        values, "access_material_digest"));
    blinker::gateway::GatewayAccessDeliveryAckView deliveryAck;
    deliveryAck.operationId = envelope.operationId;
    deliveryAck.accessMaterialDigest = view(accessMaterialDigest);
    deliveryAck.storageRevision = static_cast<uint32_t>(number(values, "storage_revision"));
    uint8_t ack[blinker::gateway::kGatewayAccessDeliveryAckMaximumEncodedSize] = {};
    size_t ackSize = 0U;
    check(blinker::gateway::encodeGatewayAccessDeliveryAck(
        deliveryAck, blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).ok(),
        "delivery ACK encode failed");
    expect(blinker::ByteView(ack, ackSize), values, "gateway_access_delivery_ack_hex");
    blinker::gateway::GatewayAccessDeliveryAckView decodedAck;
    check(blinker::gateway::decodeGatewayAccessDeliveryAck(
        blinker::ByteView(ack, ackSize), decodedAck).ok(),
        "delivery ACK decode failed");
    check(decodedAck.storageRevision == deliveryAck.storageRevision,
        "delivery ACK round trip mismatch");
    const std::vector<uint8_t> zeroDigest(
        blinker::gateway::kAccessMaterialDigestSize, 0U);
    deliveryAck.accessMaterialDigest = view(zeroDigest);
    check(blinker::gateway::encodeGatewayAccessDeliveryAck(
        deliveryAck, blinker::MutableByteSpan(ack, sizeof(ack)), ackSize).ok(),
        "zero digest delivery ACK rejected");

    const std::vector<uint8_t> proofNonce = hex(required(
        values, "proof_nonce"));
    blinker::gateway::GatewayProofCommandView proofCommand;
    proofCommand.operationId = envelope.operationId;
    proofCommand.topologyVersion = static_cast<uint32_t>(number(
        values, "topology_version"));
    proofCommand.childLogicalDeviceId = envelope.childLogicalDeviceId;
    proofCommand.childDeviceInstanceId = envelope.childDeviceInstanceId;
    proofCommand.accessEpoch = envelope.accessEpoch;
    proofCommand.gatewayCredentialVersion = envelope.credentialVersion;
    proofCommand.presenceKeyVersion = envelope.presenceKeyVersion;
    proofCommand.accessMaterialDigest = view(accessMaterialDigest);
    proofCommand.accessStorageRevision = deliveryAck.storageRevision;
    proofCommand.proofNonce = view(proofNonce);
    proofCommand.expiresAtUnixSeconds = envelope.expiresAtUnixSeconds;
    uint8_t proofBytes[
        blinker::gateway::kGatewayProofCommandMaximumEncodedSize] = {};
    size_t proofSize = 0U;
    check(blinker::gateway::encodeGatewayProofCommand(
        proofCommand, blinker::MutableByteSpan(
            proofBytes, sizeof(proofBytes)), proofSize).ok(),
        "proof command encode failed");
    expect(blinker::ByteView(proofBytes, proofSize), values,
           "gateway_proof_command_hex");
    blinker::gateway::GatewayProofCommandView decodedProof;
    check(blinker::gateway::decodeGatewayProofCommand(
        blinker::ByteView(proofBytes, proofSize), decodedProof).ok(),
        "proof command decode failed");
    check(decodedProof.accessStorageRevision ==
              proofCommand.accessStorageRevision,
          "proof command round trip mismatch");

    blinker::gateway::GatewayProofResultView proofResult;
    proofResult.operationId = envelope.operationId;
    proofResult.topologyVersion = proofCommand.topologyVersion;
    proofResult.proofNonce = proofCommand.proofNonce;
    proofResult.status = blinker::gateway::GatewayProofStatus::Secure;
    proofResult.presenceKeyVersion = envelope.presenceKeyVersion;
    uint8_t resultBytes[
        blinker::gateway::kGatewayProofResultMaximumEncodedSize] = {};
    size_t resultSize = 0U;
    check(blinker::gateway::encodeGatewayProofResult(
        proofResult, blinker::MutableByteSpan(
            resultBytes, sizeof(resultBytes)), resultSize).ok(),
        "proof result encode failed");
    expect(blinker::ByteView(resultBytes, resultSize), values,
           "gateway_proof_result_hex");
    blinker::gateway::GatewayProofResultView decodedResult;
    check(blinker::gateway::decodeGatewayProofResult(
        blinker::ByteView(resultBytes, resultSize), decodedResult).ok(),
        "proof result decode failed");
    check(decodedResult.status ==
              blinker::gateway::GatewayProofStatus::Secure,
          "proof result round trip mismatch");

    blinker::gateway::GatewayRouteAdmissionView admission;
    admission.operationId = envelope.operationId;
    admission.topologyVersion = proofCommand.topologyVersion;
    admission.childLogicalDeviceId = envelope.childLogicalDeviceId;
    admission.childDeviceInstanceId = envelope.childDeviceInstanceId;
    admission.accessEpoch = envelope.accessEpoch;
    admission.gatewayCredentialVersion = envelope.credentialVersion;
    admission.expiresAtUnixSeconds = envelope.expiresAtUnixSeconds;
    uint8_t admissionBytes[
        blinker::gateway::kGatewayRouteAdmissionMaximumEncodedSize] = {};
    size_t admissionSize = 0U;
    check(blinker::gateway::encodeGatewayRouteAdmission(
        admission, blinker::MutableByteSpan(
            admissionBytes, sizeof(admissionBytes)), admissionSize).ok(),
        "route admission encode failed");
    expect(blinker::ByteView(admissionBytes, admissionSize), values,
           "gateway_route_admission_hex");
    blinker::gateway::GatewayRouteAdmissionView decodedAdmission;
    check(blinker::gateway::decodeGatewayRouteAdmission(
        blinker::ByteView(admissionBytes, admissionSize),
        decodedAdmission).ok(), "route admission decode failed");
    check(decodedAdmission.topologyVersion == admission.topologyVersion &&
          decodedAdmission.expiresAtUnixSeconds ==
              admission.expiresAtUnixSeconds,
          "route admission round trip mismatch");
    admission.retiredGeneration = 0x8000000000000001ULL;
    check(blinker::gateway::encodeGatewayRouteAdmission(admission,
        blinker::MutableByteSpan(admissionBytes, sizeof(admissionBytes)), admissionSize).ok(),
        "recovery admission encode failed");
    check(blinker::gateway::decodeGatewayRouteAdmission(blinker::ByteView(admissionBytes, admissionSize),
        decodedAdmission).ok() && decodedAdmission.retiredGeneration == admission.retiredGeneration,
        "full uint64 recovery fence was not preserved");
    admission.version = 1U;
    check(!blinker::gateway::encodeGatewayRouteAdmission(admission,
        blinker::MutableByteSpan(admissionBytes, sizeof(admissionBytes)), admissionSize).ok(),
        "legacy admission must not bypass recovery fence");

    const std::vector<uint8_t> correlation = hex(required(values, "correlation_id"));
    const std::vector<uint8_t> inner = hex(required(values, "inner_bbp2_frame"));
    blinker::gateway::ChildRouteEnvelopeView route;
    route.topologyVersion = static_cast<uint32_t>(number(values, "topology_version"));
    route.childLogicalDeviceId = envelope.childLogicalDeviceId;
    route.childDeviceInstanceId = envelope.childDeviceInstanceId;
    route.accessEpoch = envelope.accessEpoch;
    route.gatewayCredentialVersion = envelope.credentialVersion;
    route.correlationId = view(correlation);
    route.innerBbp2Frame = view(inner);
    uint8_t routed[256] = {};
    size_t routedSize = 0U;
    check(blinker::gateway::encodeChildRouteEnvelope(
        route, blinker::MutableByteSpan(routed, sizeof(routed)), routedSize).ok(),
        "route encode failed");
    expect(blinker::ByteView(routed, routedSize), values, "child_route_envelope_hex");
    blinker::gateway::ChildRouteEnvelopeView decodedRoute;
    check(blinker::gateway::decodeChildRouteEnvelope(
        blinker::ByteView(routed, routedSize), decodedRoute).ok(),
        "route decode failed");
    check(decodedRoute.topologyVersion == route.topologyVersion &&
          decodedRoute.innerBbp2Frame.size == inner.size(),
          "route round trip mismatch");

    using blinker::gateway::TopologyState;
    check(static_cast<uint8_t>(TopologyState::Created) == number(values, "topology_created") &&
          static_cast<uint8_t>(TopologyState::Detached) == number(values, "topology_detached"),
          "topology codes drifted");
    check(blinker::gateway::validTopologyTransition(
              TopologyState::Created, TopologyState::PendingAccessDelivery) &&
          blinker::gateway::validTopologyTransition(
              TopologyState::PendingGatewayProof, TopologyState::Active) &&
          !blinker::gateway::validTopologyTransition(
              TopologyState::Active, TopologyState::Created),
          "topology transitions drifted");

    // Same independently specified bytes as the Service contract test.
    std::vector<uint8_t> closureGolden = hex(
        "890102000769626c655f6368696c64500102030405060708090a0b0c0d0e0f100304480000000000000009");
    blinker::gateway::GatewayRouteClosureView closure;
    check(blinker::gateway::decodeGatewayRouteClosure(view(closureGolden), closure).ok(), "closure decode");
    check(!closure.acknowledgment && closure.generation == 9U && closure.topologyVersion == 7U, "closure fields");
    uint8_t closureBytes[128];
    size_t closureSize = 0U;
    check(blinker::gateway::encodeGatewayRouteClosure(
        closure, blinker::MutableByteSpan(closureBytes, sizeof(closureBytes)), closureSize).ok(), "closure encode");
    check(closureSize == closureGolden.size() && std::memcmp(closureBytes, closureGolden.data(), closureSize) == 0, "closure golden");
    closure.acknowledgment = true;
    closureGolden[3] = 1U;
    check(blinker::gateway::encodeGatewayRouteClosure(
        closure, blinker::MutableByteSpan(closureBytes, sizeof(closureBytes)), closureSize).ok(), "receipt encode");
    check(std::memcmp(closureBytes, closureGolden.data(), closureSize) == 0, "receipt golden");
    for (size_t length = 0U; length < closureGolden.size(); ++length) {
        check(!blinker::gateway::decodeGatewayRouteClosure(blinker::ByteView(closureGolden.data(), length), closure), "truncated closure");
    }
    closureGolden.push_back(0U);
    check(!blinker::gateway::decodeGatewayRouteClosure(view(closureGolden), closure), "closure trailing byte");
    closureGolden.pop_back();
    for (size_t offset = 0U; offset < 4U; ++offset) {
        const uint8_t saved = closureGolden[offset]; closureGolden[offset] = 3U;
        check(!blinker::gateway::decodeGatewayRouteClosure(view(closureGolden), closure), "closure discriminator");
        closureGolden[offset] = saved;
    }
    check(blinker::gateway::decodeGatewayRouteClosure(view(closureGolden), closure).ok(), "receipt decode");
    closure.generation = 0U;
    check(!blinker::gateway::encodeGatewayRouteClosure(
        closure, blinker::MutableByteSpan(closureBytes, sizeof(closureBytes)), closureSize), "zero generation");

    std::cout << "C++ Edge Gateway golden passed: access, renewal, proof, admission, closure, route, states"
              << std::endl;
    return 0;
}
