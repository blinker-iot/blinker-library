import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { assertOfflinePageResult, observeOfflinePage } from './edge_gateway_offline_page_gate.mjs';

const passed = () => ({ nativeOffline: true, catalogRestored: true, physicalReleased: true, connects: 1, closes: 1,
  readyMs: 4000, expiredMs: 12000, physicalReleasedMs: 12100, quietMs: 6050, counter: 1, error: '', links: 0, sessions: 0, scopes: 0,
  opening: false, closing: false, events: [{ phase: 'sdk-close-done', ms: 12000 }] });

test('requires real ready, SDK and physical release, complete quiet, and no logical residue', () => {
  assertOfflinePageResult(passed(), 1);
  for (const changes of [{ readyMs: null }, { catalogRestored: false }, { nativeOffline: false },
    { physicalReleased: false }, { physicalReleasedMs: null }, { connects: 0 }, { connects: 2 }, { closes: 0 }, { events: [] },
    { quietMs: 5999 }, { counter: 2 }, { links: 1 }, { sessions: 1 }, { scopes: 1 },
    { closing: true }, { opening: true }, { error: 'GATE_UNEXPECTED' }]) {
    assert.throws(() => assertOfflinePageResult({ ...passed(), ...changes }, 1));
  }
});

test('missing cold-start directory fails before native calls or any synthetic inventory', async () => {
  const ble = { accountLinks: new Set(), accountSessions: new Set() };
  const ui = { active: true, data: { auth: {}, getDevice: () => undefined }, ble,
    network: { offline: true }, deviceV2: { state: { value: 'idle' } }, connections: new Map() };
  const action = vm.runInNewContext(`(${observeOfflinePage.toString()})`, {
    document: { querySelector() {} }, ng: { getComponent: () => ({ deviceService: { deviceUi: ui } }) },
  });
  await assert.rejects(action({ child: 'ble_test', instance: '01'.repeat(16), hubIsolated: true }), /GATE_CATALOG_NOT_RESTORED/);
});
