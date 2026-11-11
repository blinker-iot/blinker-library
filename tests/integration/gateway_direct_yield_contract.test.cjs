'use strict';
// Real Broker coordinator + Service codec + C++ receiver. The host executable
// reconstructs a deterministic fresh Hub challenge per scenario, not real BLE.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const workspace = path.resolve(__dirname, '../../..');
const service = process.env.BLINKER_SERVICE_ROOT || path.join(workspace, 'aily-blinker-services');
const library = path.join(workspace, 'blinker-library');
const protocol = require(path.join(service, 'packages/device-v2-protocol/src'));
const { EdgeGatewayDirectYield } = require(path.join(service, 'services/broker/dist/protocol/v2/edge-gateway-direct-yield'));
const binary = process.env.BLINKER_DIRECT_YIELD_HOST || path.join(library, 'build/current-host/Debug/blinker_gateway_direct_yield_tests.exe');
const identity = { topologyVersion: 7, childLogicalDeviceId: 'ble_child', accessEpoch: 3, gatewayCredentialVersion: 4,
  childDeviceInstanceId: Uint8Array.from({ length: 16 }, (_, i) => i + 1) };
function receiver(decision, issued, received, quiescent) {
  const bytes = protocol.encodeGatewayDirectYield({ ...identity, ...decision });
  const result = spawnSync(binary, ['--wire', Buffer.from(bytes).toString('hex'), String(issued), String(received), quiescent ? '1' : '0'],
    { encoding: 'utf8', timeout: 10000, windowsHide: true });
  assert.equal(result.status, 0, result.error?.message || result.stderr);
  return JSON.parse(result.stdout);
}
async function fixture(t) {
  let now = 0, current = true;
  const token = {}, reservation = { token, binding: 'exact_hub', deadline: 30000 }, messages = [];
  const gate = new EdgeGatewayDirectYield({ incarnation: Uint8Array.from({ length: 16 }, (_, i) => 0x20 + i),
    now: () => now, current: () => current,
    currentReservation: r => r.token === token, authorize: async () => true,
    send: async message => { messages.push(protocol.decodeGatewayDirectYield(protocol.encodeGatewayDirectYield({ ...identity, ...message }))); } });
  t.after(() => gate.close());
  assert.equal(await gate.start(reservation), true);
  const challenge = { ...messages[0], operation: 1, incarnation: Uint8Array.from({ length: 16 }, (_, i) => 0x20 + i), challengeId: 1 };
  now = 1000; assert.equal(await gate.accept(challenge), true);
  return { gate, token, messages, at: time => { now = time; }, retire: () => { current = false; } };
}
test('Service Decision -> real C++ receiver receipt -> Broker Ready with conservative independent clocks', async t => {
  const f = await fixture(t), decision = f.messages.at(-1);
  assert.equal(decision.leaseMillis, 29000);
  const result = receiver(decision, 10000, 11000, true);
  assert.equal(result.accepted, true); assert.ok(result.receipt);
  f.at(2000); const receipt = protocol.decodeGatewayDirectYield(Buffer.from(result.receipt, 'hex'));
  assert.equal(receipt.operation, 3); assert.equal(receipt.leaseMillis, 29000);
  assert.equal(await f.gate.accept(receipt), true); assert.equal(f.gate.ready(f.token), true);
  f.at(28999); assert.equal(f.gate.ready(f.token), true);
  f.at(29000); assert.equal(f.gate.ready(f.token), false); // Not receipt + 29 s.
});
test('C++ cannot issue a receipt during native/other-owner occupancy or after challenge timeout', async t => {
  const f = await fixture(t), decision = f.messages.at(-1);
  const busy = receiver(decision, 10000, 11000, false);
  assert.equal(busy.accepted, true); assert.equal(busy.receipt, ''); assert.equal(f.gate.ready(f.token), false);
  const late = receiver(decision, 10000, 15000, true);
  assert.equal(late.accepted, false); assert.equal(late.receipt, '');
});
test('valid C++ Yielded cannot authorize a replaced MQTT identity or canceled reservation', async t => {
  const f = await fixture(t), result = receiver(f.messages.at(-1), 1000, 2000, true);
  const receipt = protocol.decodeGatewayDirectYield(Buffer.from(result.receipt, 'hex'));
  f.retire(); assert.equal(await f.gate.accept(receipt), false); assert.equal(f.gate.ready(f.token), false);
  f.gate.cancel(); assert.equal(await f.gate.accept(receipt), false);
});
