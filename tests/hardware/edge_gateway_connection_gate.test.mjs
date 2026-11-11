import assert from 'node:assert/strict';
import test from 'node:test';
import vm from 'node:vm';
import { connectInApp } from './edge_gateway_connection_gate.mjs';

// Test the serialized observer, not BLE/authentication itself. No real account,
// device, network, credentials or SDK is used by this fixture.
function fixture(failure = '', failAt = 'authenticate') {
  const config = { hub: 'hub_test', child: 'ble_test', instance: '21'.repeat(16), count: 2 };
  const secret = 'PRIVATE_BODY_MUST_NOT_APPEAR';
  const frame = Uint8Array.of(0x42, 0x4b, 2, 5, 2, 10, 0, 2, 0, secret.length,
    ...new TextEncoder().encode(secret));
  let nextOperation = 0;
  const used = [];
  const ble = {
    accountLinks: new Set(), accountSessions: new Set(),
    async credentialDeviceInstanceId() { return new Uint8Array(16).fill(0x21); },
    client(link) {
      const session = {
        channel: { async send() {} },
        async receive() {},
        async synchronize() { if (failure && failAt === 'synchronize') throw new Error(failure); },
        async close() { ble.accountSessions.delete(session); await link.disconnect(); },
      };
      const client = {
        async authenticate() {
          await link.sendRecord(frame);
          await link.receiveRecord();
          if (failure && failAt === 'authenticate') throw new Error(failure);
          return session;
        },
        async connectWithCredential() { await link.connect(); return this.authenticate(); },
      };
      used.push({ target: client, originals: { ...client } });
      used.push({ target: session, originals: { ...session } });
      return client;
    },
    async connectUsing(link) {
      ble.accountLinks.add(link);
      await link.waitForMode();
      const session = await this.client(link).connectWithCredential();
      await session.synchronize();
      ble.accountSessions.add(session);
      return { session };
    },
  };
  const gateway = {
    ble, accountContext: () => 'account_test', assertAccount() {},
    accountPermitJoinCheckpoints: () => ({ async list() { return []; } }),
    permitJoin: { async readRelay() {}, async sendRelay() {} },
    permitJoinLink() {
      const link = {
        operationId: new Uint8Array(16).fill(++nextOperation),
        async waitForMode() { return { profile: { modeLocator: new Uint8Array(8) } }; },
        async connect() {},
        async disconnect() { this.closing = true; ble.accountLinks.delete(this); },
        async sendRecord() {},
        async receiveRecord() { return frame; },
      };
      used.push({ target: link, originals: { ...link } });
      return link;
    },
  };
  const originalClient = ble.client, originalRelay = { ...gateway.permitJoin };
  const page = { available: true, edgeHubLogicalDeviceId: config.hub, gateway };
  const action = vm.runInNewContext(`(${connectInApp.toString()})`, {
    ng: { getComponent: () => page }, document: { querySelector() {} }, setTimeout,
  });
  return { config, secret, async run() {
    const results = await action(config);
    assert.equal(ble.client, originalClient);
    assert.deepEqual(gateway.permitJoin, originalRelay);
    for (const { target, originals } of used) {
      for (const [key, value] of Object.entries(originals)) {
        if (typeof value === 'function') assert.equal(target[key], value, `restore ${key}`);
      }
    }
    return results;
  } };
}

test('normal observations separate authentication and initial synchronization', async () => {
  const f = fixture(), results = await f.run();
  assert.equal(results.length, 2);
  for (const result of results) {
    const phases = result.events.map(x => x.phase);
    assert(phases.indexOf('authenticate-ready') < phases.indexOf('connect-synchronized'));
    assert.equal(result.error, '');
    assert.equal(result.recordTrace, undefined);
    assert.equal(result.links + result.sessions + result.checkpoints, 0);
  }
  assert.notEqual(results[0].operationId, results[1].operationId);
});

test('handshake parse failure retains only public headers and stops the batch', async () => {
  const f = fixture('CBOR unsigned value is out of range'), results = await f.run();
  assert.equal(results.length, 1);
  const [result] = results;
  assert.equal(result.error, 'GATE_CBOR_UNSIGNED_RANGE');
  assert(result.events.some(x => x.phase === 'authenticate-failed'));
  assert(!result.events.some(x => x.phase === 'authenticate-ready'));
  assert.equal(result.recordTrace.length, 2);
  assert.equal(result.recordTrace[1].sequence, 2);
  assert.equal(result.recordTrace[1].bodySize, f.secret.length);
  assert(!JSON.stringify(result).includes(f.secret));
  assert.equal(result.links + result.sessions + result.checkpoints, 0);
});

test('initial sync failure is not mislabeled as Method 2 failure; arbitrary messages stay private', async () => {
  const f = fixture('PRIVATE_HTTP_CREDENTIAL', 'synchronize'), [result] = await f.run();
  assert.equal(result.error, 'UNEXPECTED_FAILURE');
  assert(result.events.some(x => x.phase === 'authenticate-ready'));
  assert(!result.events.some(x => x.phase === 'authenticate-failed' || x.phase === 'connect-synchronized'));
  assert(!JSON.stringify(result).includes('PRIVATE_HTTP_CREDENTIAL'));
  assert.equal(result.links + result.sessions + result.checkpoints, 0);
});
