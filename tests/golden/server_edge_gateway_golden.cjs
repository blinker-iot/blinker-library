'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const protocol = require('../../../aily-blinker-services/packages/device-v2-protocol/src');

function properties(path) {
  const output = new Map();
  for (const line of fs.readFileSync(path, 'utf8').split(/\r?\n/)) {
    if (!line || line.startsWith('#')) continue;
    const split = line.indexOf('=');
    if (split <= 0 || output.has(line.slice(0, split))) throw new Error('invalid fixture');
    output.set(line.slice(0, split), line.slice(split + 1));
  }
  return output;
}

const values = properties(process.argv[2]);
const value = key => {
  if (!values.has(key)) throw new Error(`missing ${key}`);
  return values.get(key);
};
const bytes = key => Uint8Array.from(Buffer.from(value(key), 'hex'));
const number = key => Number(value(key));
const envelope = {
  version: 2,
  operationId: bytes('operation_id'),
  environmentId: value('environment_id'),
  tenantId: value('tenant_id'),
  edgeHubLogicalDeviceId: value('edge_hub_logical_device_id'),
  deviceKeyVersion: number('device_key_version'),
  childLogicalDeviceId: value('child_logical_device_id'),
  childDeviceInstanceId: bytes('child_device_instance_id'),
  accessEpoch: number('access_epoch'),
  controllerId: bytes('controller_id'),
  credentialVersion: number('credential_version'),
  permissions: 3,
  presenceKeyVersion: number('presence_key_version'),
  expiresAtUnixSeconds: number('expires_at_unix_seconds'),
  nonce: bytes('nonce'),
  ciphertext: bytes('ciphertext'),
  tag: bytes('tag'),
};
assert.equal(Buffer.from(protocol.encodeGatewayAccessEnvelope(envelope)).toString('hex'),
  value('gateway_access_envelope_hex'));
assert.equal(Buffer.from(protocol.encodeGatewayAccessAad(envelope)).toString('hex'),
  value('gateway_access_aad_hex'));
assert.equal(protocol.decodeGatewayAccessEnvelope(
  protocol.encodeGatewayAccessEnvelope(envelope)).credentialVersion, 5);
const deliveryAck = {
  version: 2,
  operationId: envelope.operationId,
  accessMaterialDigest: bytes('access_material_digest'),
  storageRevision: number('storage_revision'),
};
assert.equal(Buffer.from(protocol.encodeGatewayAccessDeliveryAck(deliveryAck)).toString('hex'),
  value('gateway_access_delivery_ack_hex'));
assert.equal(protocol.decodeGatewayAccessDeliveryAck(
  protocol.encodeGatewayAccessDeliveryAck(deliveryAck)).storageRevision,
number('storage_revision'));

const proofCommand = {
  version: 1,
  operationId: envelope.operationId,
  topologyVersion: number('topology_version'),
  childLogicalDeviceId: envelope.childLogicalDeviceId,
  childDeviceInstanceId: envelope.childDeviceInstanceId,
  accessEpoch: envelope.accessEpoch,
  gatewayCredentialVersion: envelope.credentialVersion,
  presenceKeyVersion: envelope.presenceKeyVersion,
  accessMaterialDigest: deliveryAck.accessMaterialDigest,
  accessStorageRevision: deliveryAck.storageRevision,
  proofNonce: bytes('proof_nonce'),
  expiresAtUnixSeconds: envelope.expiresAtUnixSeconds,
};
assert.equal(Buffer.from(protocol.encodeGatewayProofCommand(proofCommand)).toString('hex'),
  value('gateway_proof_command_hex'));
const proofResult = {
  version: 1,
  operationId: envelope.operationId,
  topologyVersion: proofCommand.topologyVersion,
  proofNonce: proofCommand.proofNonce,
  status: protocol.EdgeGatewayProofStatus.Secure,
  presenceKeyVersion: proofCommand.presenceKeyVersion,
};
assert.equal(Buffer.from(protocol.encodeGatewayProofResult(proofResult)).toString('hex'),
  value('gateway_proof_result_hex'));

const routeAdmission = {
  version: 2,
  retiredGeneration: Buffer.alloc(8),
  operationId: envelope.operationId,
  topologyVersion: proofCommand.topologyVersion,
  childLogicalDeviceId: envelope.childLogicalDeviceId,
  childDeviceInstanceId: envelope.childDeviceInstanceId,
  accessEpoch: envelope.accessEpoch,
  gatewayCredentialVersion: envelope.credentialVersion,
  expiresAtUnixSeconds: envelope.expiresAtUnixSeconds,
};
assert.equal(Buffer.from(protocol.encodeGatewayRouteAdmission(routeAdmission)).toString('hex'),
  value('gateway_route_admission_hex'));
assert.equal(protocol.decodeGatewayRouteAdmission(
  protocol.encodeGatewayRouteAdmission(routeAdmission)).topologyVersion, 7);

const route = {
  version: 1,
  topologyVersion: number('topology_version'),
  childLogicalDeviceId: envelope.childLogicalDeviceId,
  childDeviceInstanceId: envelope.childDeviceInstanceId,
  accessEpoch: envelope.accessEpoch,
  gatewayCredentialVersion: envelope.credentialVersion,
  correlationId: bytes('correlation_id'),
  innerBbp2Frame: bytes('inner_bbp2_frame'),
};
assert.equal(Buffer.from(protocol.encodeChildRouteEnvelope(route)).toString('hex'),
  value('child_route_envelope_hex'));
assert.equal(protocol.decodeChildRouteEnvelope(
  protocol.encodeChildRouteEnvelope(route)).topologyVersion, 7);
assert.equal(protocol.EdgeGatewayTopologyState.Created, number('topology_created'));
assert.equal(protocol.EdgeGatewayTopologyState.Detached, number('topology_detached'));
assert.equal(protocol.validEdgeGatewayTopologyTransition(1, 2), true);
assert.equal(protocol.validEdgeGatewayTopologyTransition(5, 1), false);
console.log('Server Edge Gateway golden passed: envelope, AAD, ACK, proof, admission, route, states');
