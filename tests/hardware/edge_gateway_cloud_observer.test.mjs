import { test } from 'node:test';
import assert from 'node:assert/strict';
import { cloudProjection, readinessErrorCode, startCloudObserver } from './edge_gateway_cloud_observer.mjs';
test('Cloud diagnostics include only readiness facts, never store values or secrets', () => {
  const result = cloudProjection({ cloudReachable: true, manifestAccepted: true, stateFresh: false, stateRevision: 3,
    accessToken: 'sensitive', values: { key: 'sensitive' }, manifest: { secret: 'sensitive' } });
  assert.deepEqual(result, { reachable: true, manifest: true, fresh: false, revision: 3 });
  assert(!JSON.stringify(result).includes('sensitive'));
});
test('unknown and malformed projection is not reported as ready', () => {
  assert.deepEqual(cloudProjection({ cloudReachable: 'secret', stateFresh: 'yes', stateRevision: 'secret' }),
    { reachable: null, manifest: false, fresh: false, revision: null });
});
test('readiness diagnostics whitelist error codes, not arbitrary exception text', () => {
  assert.equal(readinessErrorCode({ code: 'DEVICE_V2_READY_TIMEOUT' }), 'DEVICE_V2_READY_TIMEOUT');
  assert.equal(readinessErrorCode({ code: 18 }), 18);
  for (const value of [undefined, { code: 'Bearer secret' }, { code: -1 }, { code: 256 }, { message: 'secret' }]) {
    assert.equal(readinessErrorCode(value), 'UNCLASSIFIED');
  }
});

async function recoveryFixture(code = 'DEVICE_V2_READY_TIMEOUT') {
  let listener, retire;
  const marks = [], counts = { reads: 0, acquired: 0, released: 0, stops: 0 };
  let snapshot = { cloudReachable: true, manifestAccepted: true, stateFresh: true, stateRevision: 1 };
  const abort = new AbortController();
  class Client {
    state = 'ready';
    store = { subscribe: fn => { listener = fn; return () => { listener = undefined; }; } };
    snapshot() { return snapshot; }
    subscribeState() { return () => undefined; }
    async start() {}
    async stop() { ++counts.stops; }
    async acquireConnectionDemand() {
      ++counts.acquired;
      return { accepted: Promise.resolve(), closed: new Promise(r => { retire = r; }), release: () => { ++counts.released; } };
    }
    async waitUntilReady() {
      if (++counts.reads === 2) throw { code, message: 'sensitive' };
      snapshot = { ...snapshot, cloudReachable: true }; listener?.('child');
    }
  }
  const sdk = { DeviceV2AccountClient: Client, isDeviceV2TargetReady: s => s.cloudReachable && s.stateFresh };
  const observer = await startCloudObserver(sdk, {}, 'child', abort.signal, (phase, data) => marks.push({ phase, ...data }));
  snapshot = { ...snapshot, cloudReachable: false }; listener('child'); await observer.settled();
  return { observer, counts, marks, abort, retire };
}

test('a second read is explicit, same-owner and bounded; first timeout remains failure', async () => {
  const f = await recoveryFixture();
  assert.equal(f.counts.reads, 2); assert.equal(f.counts.released, 0);
  assert.equal(f.observer.snapshot().fatal, 'GATE_CLOUD_READ_FAILED');
  await f.observer.recoverAfterTimeout();
  assert.equal(f.counts.reads, 3); assert.equal(f.counts.acquired, 1);
  assert.equal(f.observer.snapshot().fatal, 'GATE_CLOUD_READ_FAILED');
  await assert.rejects(f.observer.recoverAfterTimeout(), /GATE_RECOVERY_PRECONDITION/);
  assert(!JSON.stringify(f.marks).includes('sensitive'));
  await f.observer.close(); assert.equal(f.counts.released, 1); assert.equal(f.counts.stops, 1);
});

test('non-timeout failures never authorize an explicit recovery read', async () => {
  const f = await recoveryFixture('DEVICE_V2_READY_RETIRED');
  await assert.rejects(f.observer.recoverAfterTimeout(), /GATE_RECOVERY_PRECONDITION/);
  assert.equal(f.counts.reads, 2); await f.observer.close();
});

test('owner retirement, cancellation and close prevent diagnostic continuation', async () => {
  for (const boundary of ['retire', 'abort', 'close']) {
    const f = await recoveryFixture();
    if (boundary === 'retire') { f.retire('session-ended'); await Promise.resolve(); }
    else if (boundary === 'abort') f.abort.abort();
    else await f.observer.close();
    await assert.rejects(f.observer.recoverAfterTimeout(), /GATE_RECOVERY_PRECONDITION/);
    assert.equal(f.counts.reads, 2);
    if (boundary !== 'close') await f.observer.close();
  }
});
