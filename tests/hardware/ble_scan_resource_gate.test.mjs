import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { scanResourceGate } from './ble_scan_resource_gate.mjs';

function fixture(failed = false) {
  let owner, event;
  const native = {
    async start({ id }) { if (failed) throw Error('PRIVATE_NATIVE_BODY'); if (owner) throw Error('BLE_SCAN_BUSY'); owner = id; },
    async stop({ id }) { if (owner === id) owner = undefined; },
  };
  const prov = {
    async addListener(_name, listener) { event = listener; return { async remove() { event = undefined; } }; },
    async startBleScan() { if (owner) event({ state: 'failed', message: 'BLE_SCAN_BUSY' }); else owner = 'wifi'; },
    async stopBleScan() { if (owner === 'wifi') { owner = undefined; event?.({ state: 'failed', message: 'BLE_SCAN_CANCELLED' }); } },
  };
  const ui = { active: true, data: { auth: {} }, connections: new Map(), ble: { accountLinks: new Set(), accountSessions: new Set() } };
  const action = vm.runInNewContext(`(${scanResourceGate.toString()})`, {
    ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) }, document: { querySelector() {} },
    Capacitor: { Plugins: { BlinkerBleScan: native, WiFiProv: prov } }, crypto, setTimeout, clearTimeout,
  });
  return { ui, action, clean: () => !owner && !event };
}
test('scan-only mutual exclusion, stale stop and cleanup without device credentials or Action', async () => {
  const f = fixture(), result = await f.action(); assert(result.released); assert(f.clean());
  assert(result.directBlocksProvisioning && result.provisioningBlocksDirect && result.staleStopIsolated);
});
test('native failure is sanitized and observer cleanup still runs', async () => {
  const f = fixture(true), result = await f.action(); assert.equal(result.error, 'GATE_UNEXPECTED');
  assert(!JSON.stringify(result).includes('PRIVATE')); assert(f.clean());
});
test('a live page caller prevents the diagnostic from acquiring the scanner', async () => {
  const f = fixture(); f.ui.connections.set('page', { scopes: new Set([1]) });
  await assert.rejects(f.action(), /GATE_BUSY/); assert(f.clean());
});
