import { test } from 'node:test';
import assert from 'node:assert/strict';
import { permitBusyAppAction } from './edge_gateway_permit_busy_gate.mjs';

function fixture(t) {
  const names = ['ng', 'document', 'Capacitor', 'fetch', '__blinkerMultiChildGate'];
  const previous = names.map(name => [name, Object.getOwnPropertyDescriptor(globalThis, name)]);
  const input = { children: ['ble_A', 'ble_B'], hub: 'device_00000000-0000-0000-0000-000000000001',
    operation: 'AAAAAAAAAAAAAAAAAAAAAA' };
  const calls = [], session = {}, service = { client: { session },
    snapshot: () => ({ cloudReachable: true, stateFresh: true, manifestAccepted: true, values: { buttonPresses: { value: 0 } } }) };
  const gate = { ids: input.children, closed: false, session, actions: new Set(), controllers: input.children.map(() => new AbortController()) };
  globalThis.ng = { getComponent: () => ({ deviceService: { deviceUi: { deviceV2: service } } }) };
  globalThis.document = { querySelector: () => ({}) };
  globalThis.Capacitor = { Plugins: { SecureStorage: { get: async () => ({ accessToken: 'in-memory-only' }) } } };
  globalThis.__blinkerMultiChildGate = gate;
  let state = 'busy';
  globalThis.fetch = async (url, options) => {
    calls.push({ url, method: options.method });
    return { ok: true, json: async () => ({ data: { operationId: input.operation, edgeHubLogicalDeviceId: input.hub, state } }) };
  };
  t.after(() => previous.forEach(([name, descriptor]) => descriptor
    ? Object.defineProperty(globalThis, name, descriptor) : delete globalThis[name]));
  return { input, gate, service, calls, setState: value => { state = value; }, run: () => permitBusyAppAction(input) };
}

test('occupied pool returns Busy and closes only its own window without any Action or relay', async t => {
  const f = fixture(t), result = await f.run();
  assert.equal(result.terminal, 'busy'); assert.equal(result.closeConfirmed, true);
  assert(result.after.children.every(child => child.ready && child.active));
  assert.deepEqual(f.calls.map(call => call.method), ['POST', 'POST']);
  assert(f.calls[1].url.endsWith(`/${f.input.operation}/close`));
  await assert.rejects(f.run(), /GATE_PERMIT_ONCE/);
  assert.equal(f.calls.length, 2);
});

test('unknown Open response still closes its exact operation; failure is not reported as Busy', async t => {
  const f = fixture(t), original = globalThis.fetch;
  globalThis.fetch = async (...args) => {
    if (!args[0].endsWith('/close')) throw Error('lost response');
    return original(...args);
  };
  await assert.rejects(f.run(), /lost response/);
  assert.equal(f.calls.length, 1); assert(f.calls[0].url.endsWith('/close'));
  await assert.rejects(f.run(), /GATE_PERMIT_ONCE/);
});

test('unexpected admitted window is closed, and non-ready or replaced owners never open one', async t => {
  const f = fixture(t), original = globalThis.fetch;
  globalThis.fetch = async (...args) => { f.setState(args[0].endsWith('/close') ? 'closed' : 'ready'); return original(...args); };
  await assert.rejects(f.run(), /GATE_EXPECTED_BUSY/);
  assert.equal(f.calls.length, 2);
  f.gate.actions.clear(); f.service.client.session = {};
  await assert.rejects(f.run(), /GATE_OWNERS/);
  f.service.client.session = f.gate.session; f.gate.controllers[1].abort();
  await assert.rejects(f.run(), /GATE_CHILDREN_NOT_READY/);
  assert.equal(f.calls.length, 2);
});
