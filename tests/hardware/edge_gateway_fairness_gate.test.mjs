import test from 'node:test';
import assert from 'node:assert/strict';
import { assertPairedEvidence } from './edge_gateway_fairness_gate.mjs';
import { startCloudObserver } from './edge_gateway_cloud_observer.mjs';

const phone = () => ({ error: '', nativeOffline: true, catalogRestored: true, physicalReleased: true,
  connects: 1, closes: 1, readyMs: 8000, expiredMs: 17000, physicalReleasedMs: 17500, quietMs: 6100,
  counter: 1, links: 0, sessions: 0, scopes: 0, closing: false, opening: false,
  events: [{ phase: 'sdk-close-done', ms: 17000 }] });
const events = () => [{ phase: 'cloud-owner-accepted', ms: 50 },
  { phase: 'hub', ms: 60, wanted: 0, secure: 0 }, // Positive decision is not yet applied.
  { phase: 'hub', ms: 100, wanted: 1, secure: 1 }, { phase: 'phone-start', ms: 200 },
  { phase: 'cloud-fresh', ms: 1000, fresh: false }, { phase: 'hub', ms: 1100, wanted: 1, childState: 0 },
  { phase: 'cloud-fresh', ms: 19000, fresh: true }];

test('paired evidence requires independent retained demand, actual yield and fresh recovery', () => {
  assert.deepEqual(assertPairedEvidence(events(), phone(), 1), {
    cloudUnavailableMs: 18000, cloudOutagesMillis: [18000], cloudUsefulWindowsMillis: [], phoneReadyMs: 8000 });
  for (const remove of ['cloud-owner-accepted', 'phone-start', 'cloud-fresh', 'hub']) {
    assert.throws(() => assertPairedEvidence(events().filter(e => e.phase !== remove), phone(), 1));
  }
  for (const extra of [{ phase: 'cloud-owner-retired', ms: 2000 }, { phase: 'hub', ms: 2000, wanted: 0 }]) {
    assert.throws(() => assertPairedEvidence([...events(), extra], phone(), 1));
  }
  assert.throws(() => assertPairedEvidence(events(), { ...phone(), connects: 2 }, 1));
});

test('measures repeated outage/useful windows, excluding intentional account cleanup', () => {
  const timeline = [...events(), { phase: 'cloud-fresh', fresh: false, ms: 21000 },
    { phase: 'cloud-fresh', fresh: true, ms: 29500 }, { phase: 'cloud-account', state: 'stopped', ms: 30000 },
    { phase: 'cloud-fresh', fresh: false, ms: 30001 }];
  const summary = assertPairedEvidence(timeline, phone(), 1);
  assert.deepEqual(summary.cloudOutagesMillis, [18000, 8500]);
  assert.deepEqual(summary.cloudUsefulWindowsMillis, [2000]);
});

test('Cloud observer only acquires one production owner, drains it on close and never sends Action', async () => {
  const calls = [], records = []; let instance, settled;
  class Client {
    constructor(credentials, channel) {
      instance = this; this.credentials = credentials; this.channel = channel; this.state = 'idle';
      this.store = { subscribe: listener => { this.listener = listener; return () => calls.push('detach'); } };
    }
    subscribeState() { return () => undefined; }
    async start() { await this.channel(await this.credentials()); this.state = 'ready'; }
    async acquireConnectionDemand(id, signal) { calls.push('acquire'); return {
      accepted: Promise.resolve(), closed: new Promise(r => { settled = r; }), release: () => { calls.push('release'); settled('cancelled'); },
    }; }
    snapshot() { return { fresh: this.fresh, values: { buttonPresses: { value: 1 } } }; }
    async waitUntilReady() { calls.push('read'); this.fresh = true; this.listener('ble_test'); }
    async stop() { this.state = 'stopped'; calls.push('stop'); }
  }
  const observer = await startCloudObserver({ DeviceV2AccountClient: Client,
    openMqttDeviceV2Channel: async () => { calls.push('channel'); }, isDeviceV2TargetReady: s => !!s.fresh },
  { mqtt: { password: 'PRIVATE' } }, 'ble_test', new AbortController().signal, (phase, data) => records.push({ phase, ...data }));
  assert(observer.snapshot().fresh);
  for (let i = 0; i < 2; ++i) {
    instance.fresh = false; instance.listener('ble_test'); await observer.settled();
    assert(observer.snapshot().fresh);
  }
  await observer.close(); await Promise.resolve();
  assert.deepEqual(calls, ['channel', 'acquire', 'read', 'read', 'read', 'release', 'stop', 'detach']);
  assert.equal(records.filter(r => r.phase === 'cloud-resynchronized').length, 2);
  assert(!JSON.stringify(records).includes('PRIVATE')); assert(!observer.snapshot().ownerActive);
});

test('failed initial sync still closes the newly accepted Cloud owner', async () => {
  const calls = [];
  class Client {
    store = { subscribe: () => () => undefined };
    subscribeState() { return () => undefined; }
    async start() {}
    async acquireConnectionDemand() { return { accepted: Promise.resolve(), closed: new Promise(() => {}), release: () => calls.push('release') }; }
    async waitUntilReady() { throw Error('read-failed'); }
    async stop() { calls.push('stop'); }
  }
  await assert.rejects(startCloudObserver({ DeviceV2AccountClient: Client }, {}, 'ble_test', new AbortController().signal, () => {}));
  assert.deepEqual(calls, ['release', 'stop']);
});
