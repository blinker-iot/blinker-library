import test from 'node:test';
import assert from 'node:assert/strict';
import { measureColdStart, appColdAction } from './edge_gateway_cold_start_gate.mjs';

function fixture(overrides = {}) {
  const calls = []; let now = 100, mark;
  return { calls, io: {
    now: () => now,
    prepare: async () => { calls.push('prepare'); },
    openSerial: async observer => { mark = observer; },
    trigger: async () => { calls.push('trigger'); now += 3; mark('serial-sent'); },
    waitReady: async () => { await Promise.resolve(); now += 1000;
      return { sameMqtt: true, ready: true, waitResult: 'ready', initialSession: 'old', session: 'new' }; },
    control: async () => { calls.push('control'); now += 50; return { before: 1, after: 2 }; },
    close: async () => { calls.push('close'); },
    ...overrides,
  } };
}

test('one host clock observes trigger, readiness then exactly one Action', async () => {
  const f = fixture(); const result = await measureColdStart(f.io);
  assert.deepEqual(f.calls, ['prepare', 'trigger', 'control', 'close']);
  assert.deepEqual(result.marks.map(x => x.phase), ['trigger', 'serial-sent', 'broker-observed', 'app-control-start', 'app-control-result']);
  assert.equal(result.marks[0].ms, 0); assert.equal(result.marks.at(-1).ms, 1053);
});

for (const [name, change] of Object.entries({ timeout: { waitResult: 'deadline' },
  offline: { ready: false }, mqtt: { sameMqtt: false }, stale: { session: 'old' } })) {
  test(`${name}: never submit an Action, always close observers`, async () => {
    const f = fixture({ waitReady: async () => ({ sameMqtt: true, ready: true,
      waitResult: 'ready', initialSession: 'old', session: 'new', ...change }) });
    await assert.rejects(measureColdStart(f.io), /GATE_COLD_NOT_READY/);
    assert(!f.calls.includes('control')); assert.equal(f.calls.at(-1), 'close');
  });
}

test('an unknown Action result is not retried or reported as success', async () => {
  let count = 0;
  const f = fixture({ control: async () => { count++; throw Error('outcome_unknown'); } });
  await assert.rejects(measureColdStart(f.io), /outcome_unknown/);
  assert.equal(count, 1); assert.equal(f.calls.at(-1), 'close');
});

test('partial setup failure is cleaned up', async () => {
  const f = fixture({ prepare: async () => { throw Error('partial setup'); } });
  await assert.rejects(measureColdStart(f.io), /partial setup/);
  assert.deepEqual(f.calls, ['close']);
});

for (const code of [8, 'DEVICE_V2_COMMAND_OUTCOME_UNKNOWN']) {
test(`ingress gate preserves uncertainty ${code} without exception payloads`, async t => {
  let calls = 0;
  const page = { available: true, edgeHubLogicalDeviceId: 'hub', gateway: {
    ble: { accountLinks: new Map(), accountSessions: new Map() }
  } };
  const app = { deviceService: { deviceUi: { deviceV2: {
    command: async () => { calls++; throw Object.assign(Error('private payload'), { code }); }
  } } } };
  const oldNg = globalThis.ng, oldDocument = globalThis.document;
  t.after(() => { globalThis.ng = oldNg; globalThis.document = oldDocument; });
  globalThis.document = { querySelector: selector => selector };
  globalThis.ng = { getComponent: selector => selector === 'app-root' ? app : page };
  const result = await appColdAction({ hub: 'hub', child: 'child', action: 'ingress-command' });
  assert.deepEqual(result, { acknowledged: false, code });
  assert.equal(calls, 1);
  assert(!JSON.stringify(result).includes('private'));
});
}
