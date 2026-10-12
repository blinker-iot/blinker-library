import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { bleNativeTimingGate } from './ble_native_timing_gate.mjs';

function fixture(nativePromise, now = () => 42) {
  const cap = { nativePromise };
  const context = vm.createContext({ Capacitor: cap, performance: { now }, input: null });
  const run = input => { context.input = input; return vm.runInContext(`(${bleNativeTimingGate.toString()})(input)`, context); };
  return { cap, run };
}

test('delegates exact arguments, this and promise; observations are payload-free', async () => {
  const input = { deviceId: 'private-id', value: 'private-payload' };
  const result = { value: 'private-response' };
  const promise = Promise.resolve(result);
  let receiver, argumentsSeen;
  const original = function (...args) { receiver = this; argumentsSeen = args; return promise; };
  const { cap, run } = fixture(original);
  run({ mode: 'start' });
  assert.equal(cap.nativePromise('BluetoothLe', 'write', input), promise);
  assert.equal(receiver, cap);
  assert.equal(argumentsSeen[2], input);
  assert.equal(await promise, result);
  const snapshot = run({ mode: 'stop' });
  assert.equal(snapshot.events.length, 2);
  assert.equal(snapshot.events[1].state, 'resolved');
  assert(!JSON.stringify(snapshot).includes('private-'));
  assert.equal(cap.nativePromise, original);
  assert.throws(() => run({ mode: 'read' }));
});

test('does not instrument unrelated plugins or change synchronous failures', () => {
  const error = Error('private-native-error');
  const { cap, run } = fixture(() => { throw error; });
  run({ mode: 'start' });
  assert.throws(() => cap.nativePromise('Other', 'write'), value => value === error);
  assert.equal(run({ mode: 'read' }).events.length, 0);
  assert.throws(() => cap.nativePromise('BluetoothLe', 'read'), value => value === error);
  assert.equal(run({ mode: 'stop' }).events[1].state, 'rejected');
});

test('records rejected promise without exposing reason; stops late completions', async () => {
  let reject;
  const promise = new Promise((_, failure) => { reject = failure; });
  const { cap, run } = fixture(() => promise);
  run({ mode: 'start' });
  assert.equal(cap.nativePromise('BluetoothLe', 'connect'), promise);
  const snapshot = run({ mode: 'stop' });
  reject(Error('private-rejection'));
  await assert.rejects(promise);
  assert.equal(snapshot.events.length, 1);
  run({ mode: 'start' });
  cap.nativePromise('BluetoothLe', 'connect');
  await Promise.resolve();
  assert.equal(run({ mode: 'stop' }).events[1].state, 'rejected');
});

test('bounds output, rejects duplicate installation and detects replacement', async () => {
  const { cap, run } = fixture(() => Promise.resolve());
  assert.throws(() => run({ mode: 'start', limit: 1 }));
  run({ mode: 'start', limit: 16 });
  assert.throws(() => run({ mode: 'start' }));
  for (let n = 0; n < 20; ++n) await cap.nativePromise('BluetoothLe', 'write');
  const snapshot = run({ mode: 'read' });
  assert.equal(snapshot.events.length, 16);
  assert.equal(snapshot.dropped, 24);
  const wrapped = cap.nativePromise;
  cap.nativePromise = () => {};
  assert.throws(() => run({ mode: 'stop' }));
  cap.nativePromise = wrapped;
  run({ mode: 'stop' });
});

test('observes both real Android coordinator and standard scanner without retaining discovery data', async () => {
  const input = { id: 'private-scan-id', services: ['private-service'], deviceId: 'private-device' };
  const result = { rawAdvertisement: 'private-advertisement' };
  const promise = Promise.resolve(result);
  const { cap, run } = fixture(() => promise);
  run({ mode: 'start' });
  for (const [plugin, method] of [['BlinkerBleScan', 'start'], ['BlinkerBleScan', 'stop'],
    ['BluetoothLe', 'requestLEScan'], ['BluetoothLe', 'stopLEScan']]) {
    assert.equal(cap.nativePromise(plugin, method, input), promise);
    await promise;
  }
  cap.nativePromise('BlinkerBleScan', 'unrelated', input);
  const output = run({ mode: 'stop' });
  assert.equal(output.events.length, 8);
  assert.deepEqual(Array.from(output.events, e => e.plugin), [
    'BlinkerBleScan', 'BlinkerBleScan', 'BlinkerBleScan', 'BlinkerBleScan',
    'BluetoothLe', 'BluetoothLe', 'BluetoothLe', 'BluetoothLe',
  ]);
  assert(!JSON.stringify(output).includes('private-'));
});

test('measures native settlement rather than inventing hidden Android connect subphases', async () => {
  let time = 100, settle;
  const promise = new Promise(resolve => { settle = resolve; });
  const { cap, run } = fixture(() => promise, () => time);
  run({ mode: 'start' });
  assert.equal(cap.nativePromise('BluetoothLe', 'connect'), promise);
  time = 769;
  settle({ privateMtu: 'private-result' });
  await promise;
  const events = run({ mode: 'stop' }).events;
  assert.equal(events.length, 2);
  assert.equal(events[0].durationMs, undefined);
  assert.equal(events[1].durationMs, 669);
  assert.equal(events[1].millis, 669);
  assert.equal(events[1].method, 'connect');
  assert(!JSON.stringify(events).includes('private-'));
  assert(!JSON.stringify(events).includes('service-discovery'));
});

test('keeps overlapping native calls separate and records rejected write elapsed time without its error', async () => {
  let time = 0, finishWrite, finishMtu;
  const write = new Promise((_, reject) => { finishWrite = reject; });
  const mtu = new Promise(resolve => { finishMtu = resolve; });
  const { cap, run } = fixture((_plugin, method) => method === 'write' ? write : mtu, () => time);
  run({ mode: 'start' });
  assert.equal(cap.nativePromise('BluetoothLe', 'write'), write);
  time = 5;
  assert.equal(cap.nativePromise('BluetoothLe', 'getMtu'), mtu);
  time = 12;
  finishMtu(); await mtu;
  time = 20;
  finishWrite(Error('private-failure'));
  await assert.rejects(write);
  const events = run({ mode: 'stop' }).events;
  const settled = events.filter(event => event.state !== 'start');
  assert.deepEqual(Array.from(settled, event => [event.method, event.call, event.state, event.durationMs]),
    [['getMtu', 2, 'resolved', 7], ['write', 1, 'rejected', 20]]);
  assert(!JSON.stringify(events).includes('private-'));
});
