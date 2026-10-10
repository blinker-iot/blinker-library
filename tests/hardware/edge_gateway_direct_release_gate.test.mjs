import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { checkDirectReleaseInApp } from './edge_gateway_direct_release_gate.mjs';

function fixture(mode = 'late-connect', fault = '') {
  const input = { mode, hubIsolated: true, child: 'ble_test', instance: '01'.repeat(16) };
  const calls = [], cap = {};
  let physical = false, failed = false, closing;
  const native = async (plugin, method, options) => {
    calls.push({ plugin, method, options });
    if (method === 'connect') { physical = true; return; }
    if (method === 'disconnect') {
      physical = false;
      if (options.timeout === 1 && fault !== 'fast-close') throw Error('Disconnection timeout.');
      return;
    }
    if (method === 'getConnectedDevices') return { devices: physical ? [{ deviceId: 'PRIVATE_ADDRESS' }] : [] };
  };
  cap.nativePromise = native;
  const ble = {
    accountLinks: new Set(), accountSessions: new Set(), accountContext: () => ({}), assertAccount() {},
    async credentialDeviceInstanceId() { return new Uint8Array(16).fill(1); },
    async hasActiveCredential() { return true; },
    async ensureReady(id, timeout, context, admission) {
      if (failed) throw Error('BLE_DIRECT_NATIVE_RELEASE_FAILED');
      if (fault === 'open') throw Error('PRIVATE_OPEN_ERROR');
      this.opening = true; this.accountLinks.add('link');
      try {
        await cap.nativePromise('BluetoothLe', 'connect', { deviceId: 'PRIVATE_ADDRESS' });
        admission.assertAcquire();
        this.accountSessions.add('session');
        this.active = { session: { store: { snapshot: () => ({ manifestAccepted: true, stateFresh: true }) } } };
      } catch (error) { await this.disconnect(id); throw error; }
      finally { this.opening = false; }
    },
    async disconnect() {
      if (closing) return closing;
      if (!this.accountLinks.size) return;
      this.active = undefined;
      closing = cap.nativePromise('BluetoothLe', 'disconnect', { deviceId: 'PRIVATE_ADDRESS' }).then(() => {
        this.accountLinks.clear(); this.accountSessions.clear(); closing = undefined; this.closing = undefined;
      }).catch(() => { failed = true; throw Error('BLE_DIRECT_NATIVE_RELEASE_FAILED'); });
      this.closing = closing;
      return closing;
    },
  };
  const ui = { active: true, data: { auth: {} }, ble, connections: new Map() };
  const sandbox = { Capacitor: cap, ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) },
    document: { querySelector() {} }, AbortController, performance, setTimeout, clearTimeout };
  const action = vm.runInNewContext(`(${checkDirectReleaseInApp.toString()})`, sandbox);
  return { input, ui, ble, calls, async run() {
    try { return await action(input); }
    finally { assert.equal(cap.nativePromise, native); assert.equal(sandbox.__edgeDirectRelease, undefined); }
  } };
}

test('late real-success delivery is cancelled and drained, with no Ready or leaked identities', async () => {
  const f = fixture(), result = await f.run();
  assert.equal(result.error, ''); assert.equal(result.ready, false);
  assert.equal(result.physicalReleased, true); assert.equal(result.nativeConnects, 1);
  assert.equal(result.links + result.sessions, 0); assert.equal(result.closing, false);
  const events = result.events.map(e => e.phase);
  assert(events.indexOf('sdk-connect-done') < events.indexOf('cancel-before-connect-delivery'));
  assert(events.indexOf('sdk-close-done') < events.indexOf('opening-settled'));
  assert(!JSON.stringify(result).includes('PRIVATE'));
});
test('timeout adjusts one real SDK call, retains quarantine and blocks a successor', async () => {
  const f = fixture('close-timeout'), result = await f.run();
  assert.equal(result.error, ''); assert(result.ready && result.sdkTimeout && result.blocked && result.physicalReleased);
  assert.equal(result.nativeConnects, 1); assert.equal(result.nativeCloses, 1);
  assert.equal(result.closing, true); assert.equal(result.links, 1);
  assert.equal(f.calls.filter(c => c.options.timeout === 1).length, 1);
  assert(!JSON.stringify(result).includes('PRIVATE'));
});
test('a fast successful SDK close is NOT falsely counted as a timeout gate', async () => {
  const result = await fixture('close-timeout', 'fast-close').run();
  assert.equal(result.error, 'GATE_FAULT_NOT_OBSERVED'); assert.equal(result.blocked, false);
});
test('unexpected errors are redacted and all bridge hooks restored', async () => {
  const result = await fixture('late-connect', 'open').run();
  assert(result.error); assert(!JSON.stringify(result).includes('PRIVATE'));
  assert.equal(result.nativeConnects, 0);
});
test('requires isolated exact identity and no other active callers', async () => {
  const f = fixture(); f.input.hubIsolated = false;
  await assert.rejects(f.run(), /GATE_INPUT/); f.input.hubIsolated = true;
  f.ui.connections.set('page', { scopes: new Set([{}]) });
  await assert.rejects(f.run(), /GATE_BUSY/); assert.equal(f.calls.length, 0);
});
test('rechecks idle after asynchronous credential preflight', async () => {
  const f = fixture();
  f.ble.hasActiveCredential = async () => { f.ui.connections.set('late-page', { scopes: new Set([{}]) }); return true; };
  await assert.rejects(f.run(), /GATE_BUSY/); assert.equal(f.calls.length, 0);
});
