'use strict';
// Source owner aggregate -> shared Service CBOR -> real C++ decoder/lease.
// No MQTT, clocks synchronized across machines, accounts, BLE or hardware.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const { createRequire } = require('node:module');
const repository = path.resolve(__dirname, '../..');
const service = process.env.BLINKER_SERVICE_ROOT || path.resolve(repository, '../aily-blinker-services');
const serviceRequire = createRequire(path.join(service, 'package.json'));
const { encodeGatewayExecutionDemand } = serviceRequire('./packages/device-v2-protocol/src');
const { connectionDemandRemainingMillis } = serviceRequire('./services/broker/dist/protocol/v2/connection-demand');
const executable = process.env.BLINKER_EXECUTION_LEASE_TEST || path.join(repository, 'build/current-host',
  ...(process.platform === 'win32' ? ['Debug', 'blinker_gateway_execution_lease_tests.exe'] : ['blinker_gateway_execution_lease_tests']));
const identity = {
  operation: 2, topologyVersion: 7, childLogicalDeviceId: 'ble_child',
  childDeviceInstanceId: Uint8Array.from({ length: 16 }, (_, i) => i + 1),
  accessEpoch: 3, gatewayCredentialVersion: 4,
  incarnation: Uint8Array.from({ length: 16 }, (_, i) => 0x20 + i), challengeId: 1,
};
function receive(snapshot, sourceNow, issued, received, changes = {}) {
  const bytes = encodeGatewayExecutionDemand({ ...identity,
    leaseMillis: connectionDemandRemainingMillis(snapshot, sourceNow), ...changes });
  const result = spawnSync(executable, ['--wire', Buffer.from(bytes).toString('hex'), String(issued), String(received)],
    { encoding: 'utf8', timeout: 5000, maxBuffer: 65536 });
  assert.ifError(result.error);
  assert.equal(result.status, 0, result.stderr || result.stdout);
  return JSON.parse(result.stdout);
}
test('Broker remaining duration reaches C++ without a receive-time extension', () => {
  // Source and Hub use deliberately unrelated monotonic origins.
  assert.deepEqual(receive({ owners: 2, deadline: 530000 }, 504000, 1000, 4000), { accepted: true, wanted: true });
});
test('zero owners, expired source and sub-millisecond remainder produce release', () => {
  for (const snapshot of [{ owners: 0, deadline: 530000 }, { owners: 1, deadline: 504000 },
    { owners: 1, deadline: 504000.75 }]) {
    assert.deepEqual(receive(snapshot, 504000, 1000, 4000), { accepted: true, wanted: false });
  }
});
test('queued positive decision cannot pass local challenge or shortened demand deadline', () => {
  assert.deepEqual(receive({ owners: 1, deadline: 530000 }, 504000, 1000, 6000), { accepted: false, wanted: false });
  assert.deepEqual(receive({ owners: 1, deadline: 505000 }, 504000, 1000, 2000), { accepted: false, wanted: false });
});
test('old incarnation and unissued challenge cannot grant or release', () => {
  for (const owners of [0, 1]) {
    for (const changes of [{ incarnation: new Uint8Array(16).fill(0x40) }, { challengeId: 2 }]) {
      assert.deepEqual(receive({ owners, deadline: 530000 }, 504000, 1000, 1100, changes),
        { accepted: false, wanted: false });
    }
  }
});
test('Hub millis wrap does not depend on Broker uptime origin', () => {
  assert.deepEqual(receive({ owners: 1, deadline: 530000 }, 500000, 0xffffffff - 999, 10), { accepted: true, wanted: true });
});
