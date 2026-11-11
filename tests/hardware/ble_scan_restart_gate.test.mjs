import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { scanRestartPhase } from './ble_scan_restart_gate.mjs';

function fixture(failClose = false) {
  let event, owner, attempts = 0;
  const port = {
    async addListener(_name, listener) { event = listener; return { async remove() { event = undefined; } }; },
    async start({ id }) {
      if (++attempts > 5) { event({ id, error: 'BLE_SCAN_THROTTLED', retryAfterMs: 20000 }); throw Error('BLE_SCAN_THROTTLED'); }
      owner = id; event({ id, result: {} });
    },
    async stop({ id }) { if (failClose) throw Error('SECRET_NATIVE_BODY'); if (owner === id) owner = undefined; },
  };
  const ui = { active: true, data: { auth: {} }, connections: new Map(), ble: { accountLinks: new Set(), accountSessions: new Set() } };
  const action = vm.runInNewContext(`(${scanRestartPhase.toString()})`, {
    ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) }, document: { querySelector() {} },
    Capacitor: { Plugins: { BlinkerBleScan: port } }, crypto, setTimeout, clearTimeout,
  });
  return { action, clean: () => !event && !owner, cool: () => { attempts = 0; } };
}
test('quota and explicit later recovery are separate phases with no hidden retries', async () => {
  const f = fixture(); const before = await f.action({ phase: 'prepare' });
  assert(before.passed); assert.equal(before.rows.length, 6); assert(f.clean());
  const after = await f.action({ phase: 'after-restart' }); assert(after.passed); assert.equal(after.rows.length, 1);
  f.cool(); assert((await f.action({ phase: 'recover' })).passed); assert(f.clean());
});
test('early restart refund is a failed gate, not a successful scan recovery', async () => {
  const f = fixture(); assert(!(await f.action({ phase: 'after-restart' })).passed); assert(f.clean());
});
test('physical close failure stops the gate and is never certified as released', async () => {
  const f = fixture(true), result = await f.action({ phase: 'prepare' });
  assert(!result.passed && !result.closed); assert.equal(result.rows.length, 1); assert(!JSON.stringify(result).includes('SECRET'));
});
