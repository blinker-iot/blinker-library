import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { scanLookupGate } from './ble_scan_lookup_gate.mjs';

function fixture({ staleSelection = false, selected = false, throttle = false } = {}) {
  let owner, event, finish, lookups = 0, timer, attempts = 0;
  const native = {
    async start({ id }) {
      if (owner) throw Error('BLE_SCAN_BUSY');
      if (++attempts >= 5 && throttle) throw Error('BLE_SCAN_THROTTLED'); owner = id;
    },
    async stop({ id }) { if (owner === id) owner = undefined; },
  };
  const create = ({ name }) => {
    event?.({ state: 'started' });
    if (owner) return Promise.reject(Error('BLE_SCAN_BUSY'));
    if (++attempts >= 5 && throttle) return Promise.reject(Error('BLE_SCAN_THROTTLED'));
    owner = 'prov'; ++lookups;
    return new Promise((_resolve, reject) => {
      finish = error => { owner = undefined; finish = undefined; if (staleSelection) selected = true; reject(Error(error)); };
      if (lookups === 3) timer = setTimeout(() => finish?.(`No BLE provisioning device named ${name} was found.`), 0);
    });
  };
  const prov = {
    async getDeviceInfo() { if (!selected) throw Error('No ESP device is selected.'); return {}; },
    async addListener(_name, handler) { event = handler; return { async remove() { event = undefined; } }; },
    createDevice: create, createDeviceFromQr: ({ payload }) => create(JSON.parse(payload)),
    async stopBleScan() { finish?.('BLE_SCAN_CANCELLED'); },
    async clearState() { finish?.('BLE_SCAN_CANCELLED'); },
  };
  const ui = { active: true, data: { auth: {} }, connections: new Map(), ble: { accountLinks: new Set(), accountSessions: new Set() } };
  const action = vm.runInNewContext(`(${scanLookupGate.toString()})`, {
    ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) }, document: { querySelector() {} },
    Capacitor: { Plugins: { BlinkerBleScan: native, WiFiProv: prov } }, crypto, performance, setTimeout, clearTimeout,
  });
  return { action, clean: () => !owner && !event && !finish, close: () => clearTimeout(timer) };
}

test('name/QR cancellation, overlap, exact handoff and deadline without GATT', async () => {
  const f = fixture(); const result = await f.action(); assert(result.passed); assert(f.clean()); f.close();
});
test('late selection after cancellation fails the gate', async () => {
  const f = fixture({ staleSelection: true }); const result = await f.action();
  assert(!result.passed); assert.equal(result.error, 'GATE_LATE_SELECTION'); assert(f.clean()); f.close();
});
test('an existing provisioning selection must not be cleared by a diagnostic', async () => {
  const f = fixture({ selected: true }); await assert.rejects(f.action(), /GATE_PROV_SELECTED/); assert(f.clean());
});
test('shared scan quota rejection is not misreported as an exclusivity failure', async () => {
  const f = fixture({ throttle: true }); const result = await f.action();
  assert(!result.passed); assert.equal(result.error, 'BLE_SCAN_THROTTLED'); assert(f.clean()); f.close();
});
