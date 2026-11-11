import { test } from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { cloudWitnessAppAction, runCloudWitnessAppGate } from './cloud_witness_app_gate.mjs';

function fixture({ pendingAcceptance = false, actionFailure = false } = {}) {
  let accept, retire, listener;
  const timers = new Map(), calls = { read: 0, command: 0, released: 0, unwatched: 0 };
  const snapshot = { manifestAccepted: true, stateFresh: true, stateRevision: 1,
    values: { power: { value: false }, buttonPresses: { value: 0 }, secret: { value: 'not-for-output' } } };
  const interest = { accepted: pendingAcceptance ? new Promise(r => { accept = r; }) : Promise.resolve(),
    closed: new Promise(r => { retire = r; }), release: () => { ++calls.released; } };
  const cloud = { client: { session: {} }, state: { value: 'ready' },
    store: { subscribe: fn => { listener = fn; return () => { ++calls.unwatched; }; } },
    acquireStateInterest: async () => interest, waitUntilReady: async () => { ++calls.read; },
    snapshot: () => snapshot,
    command: async (_id, key, value) => {
      ++calls.command;
      if (key === 'button' && actionFailure) throw Error('Uncertain execution');
      if (key === 'power') snapshot.values.power.value = value;
      else snapshot.values.buttonPresses.value++;
      snapshot.stateRevision++; listener?.('device_test', snapshot);
    } };
  const root = { deviceService: { deviceUi: { deviceV2: cloud } }, dataService: { auth: { uuid: 'account' } },
    userService: { getAllInfo: async () => true } };
  const context = vm.createContext({ ng: { getComponent: () => root }, document: { querySelector: () => ({}) }, AbortController,
    setTimeout: (fn, ms) => { const token = {}; timers.set(token, { fn, ms }); return token; },
    clearTimeout: token => timers.delete(token) });
  const action = vm.runInContext(`(${cloudWitnessAppAction.toString()})`, context);
  return { run: (mode, id = 'device_test') => action({ mode, logicalDeviceId: id }), cloud, calls, timers, context,
    accept: () => accept(), retire: () => retire('session-ended') };
}

test('cloud-capable BLE-origin identity reaches the same gate; malformed IDs fail before CDP', async t => {
  const offline = Error('no CDP in unit test');
  const fetch = t.mock.method(globalThis, 'fetch', async () => { throw offline; });
  await assert.rejects(runCloudWitnessAppGate('open', 'ble__Z_ZA8SUGgyIiqkfAf3ZBw'), e => e === offline);
  for (const id of ['ble_', 'ble_short', '8C:BF:EA:1B:96:14', 'ble_' + 'A'.repeat(23)]) {
    await assert.rejects(runCloudWitnessAppGate('open', id), /Invalid witness request/);
  }
  assert.equal(fetch.mock.callCount(), 1);
});

test('BLE-origin logical ID uses the original account observation and command path', async () => {
  const f = fixture(), id = 'ble__Z_ZA8SUGgyIiqkfAf3ZBw';
  await f.run('open', id); await f.run('sample', id);
  const result = await f.run('action', id);
  assert(result.sameAccountSession); assert.equal(result.values.buttonPresses, 1);
  await f.run('close', id); assert.equal(f.calls.released, 1);
});

test('observe acceptance before readiness; one existing account and whitelisted State', async () => {
  const f = fixture({ pendingAcceptance: true }), opening = f.run('open');
  await new Promise(r => setImmediate(r)); assert.equal(f.calls.read, 0);
  f.accept(); const opened = await opening; assert(opened.sameAccountSession);
  const sample = await f.run('sample'); assert.equal(sample.values.power, true);
  assert(!JSON.stringify(sample).includes('not-for-output'));
  await f.run('close'); await f.run('close');
  assert.equal(f.calls.released, 1); assert.equal(f.calls.unwatched, 1); assert.equal(f.timers.size, 0);
});

test('an uncertain Action is latched and never retried', async () => {
  const f = fixture({ actionFailure: true }); await f.run('open');
  await assert.rejects(f.run('action')); await assert.rejects(f.run('action'));
  assert.equal(f.calls.command, 1); await f.run('close');
});

test('only an explicitly closed witness can be replaced by a different device', async () => {
  const f = fixture(); await f.run('open');
  await assert.rejects(f.run('open', 'another'));
  await assert.rejects(f.run('close', 'another'));
  await f.run('close');
  await assert.rejects(f.run('sample', 'another'));
  await f.run('open', 'another'); await f.run('close', 'another');
  assert.equal(f.calls.released, 2); assert.equal(f.calls.unwatched, 2);
  assert.equal(f.timers.size, 0);
});

test('account replacement cannot send through a successor session', async () => {
  const f = fixture(); await f.run('open'); f.cloud.client.session = {};
  await assert.rejects(f.run('sample')); await assert.rejects(f.run('action'));
  assert.equal(f.calls.command, 0); await f.run('close');
});

test('deadline or lost observation releases only this witness', async () => {
  for (const reason of ['deadline', 'observation']) {
    const f = fixture(); await f.run('open');
    if (reason === 'deadline') [...f.timers.values()].find(t => t.ms === 120000).fn();
    else { f.retire(); await Promise.resolve(); }
    const result = await f.run('inspect'); assert(result.aborted); assert.equal(result.cloudState, 'ready');
    assert.equal(f.timers.size, 0); assert.equal(f.calls.released, 1);
    await assert.rejects(f.run('sample')); assert.equal(f.calls.command, 0);
  }
});

test('periodic Property writes are bounded; close prevents late timer work', async () => {
  const f = fixture(); await f.run('open');
  for (let i = 0; i < 24; i++) await f.run('sample');
  await assert.rejects(f.run('sample')); assert.equal(f.calls.command, 24); await f.run('close');
  const g = fixture(); await g.run('open'); await g.run('start'); await new Promise(r => setImmediate(r));
  const tick = [...g.timers.values()].find(t => t.ms === 1000).fn;
  await g.run('close'); await tick(); assert.equal(g.calls.command, 1); assert.equal(g.timers.size, 0);
});
