import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { measureDirectInApp } from './edge_gateway_direct_timing_gate.mjs';

function fixture(failure = '') {
  const config = { child: 'ble_test', instance: '01'.repeat(16), count: 2, mode: 'isolated-baseline', hubIsolated: true };
  const originals = [];
  const ble = {
    accountLinks: new Set(), accountSessions: new Set(), accountContext: () => ({}), assertAccount() {},
    async credentialDeviceInstanceId() { return new Uint8Array(16).fill(1); }, async hasActiveCredential() { return true; },
    async discoverDirect() { return { profile: { wireVersion: 3, capabilities: 0x51, modeLocator: 'PRIVATE' } }; },
    createLink() { return { async connect() {}, async disconnect() {} }; },
    client(link) {
      const session = { logicalDeviceId: config.child, state: 'ready',
        store: { snapshot: () => ({ manifestAccepted: true, stateFresh: true }) },
        async synchronize() { if (failure === 'sync') throw Error('PRIVATE_SYNC_ERROR'); } };
      const client = { async authenticate() { if (failure === 'auth') throw Error('BLE_DIRECT_DEVICE_PROOF_INVALID'); return session; } };
      originals.push([client, client.authenticate, session, session.synchronize]); return client;
    },
    async ensureReady(id) {
      await this.discoverDirect();
      const link = this.createLink(); this.link = link;
      this.accountLinks.add(link); const client = this.client(link); await link.connect();
      const session = await client.authenticate(); this.accountSessions.add(session);
      this.active = { logicalDeviceId: id, session }; await session.synchronize();
    },
    async disconnect() { await this.link?.disconnect(); this.accountLinks.clear(); this.accountSessions.clear(); this.active = undefined; },
  };
  const ui = { active: true, data: { auth: {} }, connections: new Map(), ble };
  let time = 0;
  const sandbox = { ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) }, document: { querySelector() {} },
    performance: { now: () => ++time }, AbortController, setTimeout, clearTimeout };
  const action = vm.runInNewContext(`(${measureDirectInApp.toString()})`, sandbox);
  const oldClient = ble.client, oldDiscover = ble.discoverDirect;
  return { config, ui, ble, async run() {
    try { return await action(config); }
    finally {
      assert.equal(ble.client, oldClient); assert.equal(ble.discoverDirect, oldDiscover);
      assert.equal(sandbox.__edgeDirectTiming, undefined);
      for (const [client, auth, session, sync] of originals) { assert.equal(client.authenticate, auth); assert.equal(session.synchronize, sync); }
    }
  } };
}

test('separates actual phase boundaries, observes no values/secrets and closes every iteration', async () => {
  const f = fixture(), result = await f.run(); assert.equal(result.results.length, 2);
  assert.equal(result.nativeAdmissionBuild, false);
  for (const row of result.results) {
    assert.equal(row.error, ''); assert.equal(row.links + row.sessions, 0);
    const names = row.events.map(e => e.phase);
    assert(names.indexOf('discovery-done') < names.indexOf('native-gatt-done'));
    assert(names.indexOf('method2-done') < names.indexOf('initial-state-done'));
    assert(names.indexOf('ready') < names.indexOf('closed'));
  }
  assert(!JSON.stringify(result).includes('PRIVATE'));
});
test('requires explicit isolated-baseline context and no active callers', async () => {
  const f = fixture(); f.config.hubIsolated = false; await assert.rejects(f.run(), /GATE_INPUT/);
  f.config.hubIsolated = true; f.ui.connections.set('other', { scopes: new Set([{}]) });
  await assert.rejects(f.run(), /GATE_BUSY/);
});
test('a page caller arriving during credential preflight cannot be bypassed', async () => {
  const f = fixture();
  f.ble.hasActiveCredential = async () => {
    f.ui.connections.set('page', { scopes: new Set([{}]) }); return true;
  };
  await assert.rejects(f.run(), /GATE_BUSY/);
  assert.equal(f.ble.accountLinks.size, 0);
});
for (const failure of ['auth', 'sync']) test(`${failure} fails the batch once, redacts data and restores hooks`, async () => {
  const result = await fixture(failure).run(); assert.equal(result.results.length, 1);
  const row = result.results[0]; assert.equal(row.links + row.sessions, 0); assert(row.error);
  assert(!JSON.stringify(result).includes('PRIVATE'));
  assert.equal(row.events.some(e => e.phase === 'method2-done'), failure === 'sync');
});

test('failed cleanup still restores observer hooks, stops the batch and hides SDK error bodies', async () => {
  const f = fixture(); let closes = 0;
  f.ble.createLink = () => ({ async connect() {}, async disconnect() { ++closes; throw Error('PRIVATE_CLOSE_ERROR'); } });
  await assert.rejects(f.run(), { message: 'GATE_CLOSE_FAILED' });
  assert.equal(closes, 2); // One iteration, then final cleanup; no second acquisition.
});

test('native scan throttling is not relabelled as a device timeout or an unknown failure', async () => {
  const f = fixture(); f.ble.ensureReady = async () => { throw Error('BLE_SCAN_THROTTLED'); };
  const result = await f.run();
  assert.equal(result.results.length, 1); assert.equal(result.results[0].error, 'BLE_SCAN_THROTTLED');
});

test('a late parallel native close stays in its own iteration and is drained before the next discovery', async () => {
  const f = fixture();
  let finish, pending, discoveries = 0;
  const originalDiscover = f.ble.discoverDirect;
  f.ble.createLink = () => ({ async connect() {}, async disconnect() { if (pending) await pending; } });
  f.ble.discoverDirect = async function() {
    if (++discoveries === 1) pending = new Promise(r => { finish = r; });
    return originalDiscover();
  };
  f.ble.disconnect = async function() {
    if (this.link) { void this.link.disconnect(); this.link = undefined; }
    this.accountLinks.clear(); this.accountSessions.clear(); this.active = undefined;
  };
  // Invoke the serialized production observer directly; fixture.run also checks
  // the earlier factory baseline, intentionally changed by this test.
  const action = vm.runInNewContext(`(${measureDirectInApp.toString()})`, {
    ng: { getComponent: () => ({ deviceService: { deviceUi: f.ui } }) }, document: { querySelector() {} },
    performance, AbortController, setTimeout, clearTimeout,
  });
  const work = action(f.config); await new Promise(r => setTimeout(r, 5));
  assert.equal(discoveries, 1); finish(); pending = undefined;
  const result = await work; assert.equal(discoveries, 2);
  for (const row of result.results) {
    assert(row.events.every(e => Number.isFinite(e.ms)));
    assert.equal(row.events.at(-1).phase, 'closed');
  }
});
