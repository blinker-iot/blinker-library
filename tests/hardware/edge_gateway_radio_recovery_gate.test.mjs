import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { clockOffset, elapsedBounds, observeRadioRecovery } from './edge_gateway_radio_recovery_gate.mjs';

test('clock calibration intersects request/reply intervals, not receive-time point estimates', () => {
  assert.deepEqual(clockOffset([{ start: 100, end: 110, local: 20 }, { start: 105, end: 107, local: 24 }]), { lower: 81, upper: 83 });
  assert.throws(() => clockOffset([{ start: 0, end: 1, local: 0 }, { start: 3, end: 4, local: 0 }]), /CLOCK_INCONSISTENT/);
});
test('elapsed interval retains uncertainty, including an observation before late SDK idle sampling', () => {
  assert.deepEqual(elapsedBounds(10, { lower: 3, upper: 5 }, 16, { lower: 1, upper: 2 }), { minMs: -5, maxMs: -2 });
});

function fixture(nativeFailure = false) {
  const profile = { wireVersion: 3, mode: 2, capabilities: 81, modeLocator: new Uint8Array(8).fill(1) };
  const credential = { state: 'active', logicalDeviceId: 'ble_test', deviceInstanceId: new Uint8Array(16).fill(1),
    controllerSecret: new Uint8Array(16).fill(3), presenceKeys: [{ key: new Uint8Array(16).fill(4), accessEpoch: 1, version: 1 }] };
  let matched = 0;
  const protocol = { decodeBleModeProfile: bytes => ({ ...profile, modeLocator: bytes.slice(5) }),
    async matchesBlePresenceLocator(key, context, locator) {
      assert.equal(key[0], 4); assert.equal(context.presenceKeyVersion, 1); ++matched;
      if (nativeFailure === 'matcher') throw Error('PRIVATE_ERROR_MUST_NOT_ESCAPE');
      return locator[0] === 2; // A fresh nonce, not the priming profile's 1.
    } };
  let emit, selected, scan, path = '/home/device';
  const ble = { accountLinks: new Set(), accountSessions: new Set(), active: undefined,
    accountContext: () => ({}), assertAccount() {}, credentialDeviceInstanceId: async () => new Uint8Array(16).fill(1),
    credentialStore: () => ({ load: async () => credential }),
    discoverDirect: async () => ({ profile }),
    async ensureReady(id) { await this.discoverDirect(); this.active = { logicalDeviceId: id, session: { state: 'ready' } }; },
    async disconnect() { this.active = undefined; },
  };
  const original = ble.discoverDirect;
  const page = { snapshot: { stateFresh: true, endpoints: [{ key: 'buttonPresses', value: 1 }] }, canControl: true, direct: false };
  const ui = { active: true, data: { auth: {} }, ble, connections: new Map() };
  const native = {
    async addListener(_name, listener) { emit = listener; return { async remove() { emit = undefined; } }; },
    async start({ id }) {
      if (nativeFailure === true) throw Error('BLE_SCAN_THROTTLED');
      scan = id; emit({ id, result: { serviceData: {} } });
      if (nativeFailure === 'held') {
        emit({ id, result: { serviceData: { '5f6d0001-3f5b-4e4f-9f4d-626c696e6b32': '0102035100' + '02'.repeat(8) } } });
        await new Promise(resolve => setTimeout(resolve, 0));
      }
    },
    async stop() { scan = undefined; },
  };
  const root = { deviceService: { deviceUi: ui }, navCtrl: { async navigateRoot(next) {
    path = next; selected = next.startsWith('/device/');
    if (scan && !selected) emit({ id: scan, result: { serviceData: { '5f6d0001-3f5b-4e4f-9f4d-626c696e6b32': '0102035100' + '02'.repeat(8) } } });
  } } };
  const action = vm.runInNewContext(`(${observeRadioRecovery.toString()})`, {
    ng: { getComponent: element => element === 'app-root' ? root : page }, document: { querySelector: selector => selector },
    Capacitor: { Plugins: { BlinkerBleScan: native } }, crypto, performance, AbortController, setTimeout, clearTimeout,
  });
  return { action: () => action({ hubIsolated: true, child: 'ble_test', instance: '01'.repeat(16) }, protocol), ui,
    matched: () => matched,
    cleared: () => !credential.controllerSecret.some(Boolean) && !credential.presenceKeys[0].key.some(Boolean),
    clean: () => !scan && !emit && !ble.active && ble.discoverDirect === original && path === '/home/device' };
}
test('authorized baseline, native observation, page release and authenticated rotating nonce; no Action', async () => {
  const f = fixture(), result = await f.action(); assert(result.closed); assert.equal(result.counter, 1); assert(f.clean());
  assert.equal(f.matched(), 1); assert(f.cleared());
  assert(result.seenAt >= result.releaseAt); assert(!JSON.stringify(result).includes('01020351'));
});
test('native quota failure cleans up without retry or changing the prior logical binding', async () => {
  const f = fixture(true), result = await f.action(); assert(!result.closed); assert.equal(result.error, 'BLE_SCAN_THROTTLED'); assert(f.clean());
  assert(f.cleared());
});
test('active business scopes prevent isolation and cannot be cleared to make the probe pass', async () => {
  const f = fixture(); f.ui.connections.set('other', { scopes: new Set([1]) });
  await assert.rejects(f.action(), /GATE_BUSY/); assert.equal(f.ui.connections.get('other').scopes.size, 1);
});
test('an authenticated advertisement while Cloud is held invalidates the physical baseline', async () => {
  const f = fixture('held'), result = await f.action();
  assert.equal(result.error, 'GATE_ADVERTISING_WHILE_HELD'); assert(!result.closed); assert(f.clean()); assert(f.cleared());
});
test('asynchronous matcher failure is sanitized and finishes before credentials are cleared', async () => {
  const f = fixture('matcher'), result = await f.action();
  assert.equal(result.error, 'GATE_MATCH_FAILED'); assert(!JSON.stringify(result).includes('PRIVATE'));
  assert(f.clean()); assert(f.cleared());
});
