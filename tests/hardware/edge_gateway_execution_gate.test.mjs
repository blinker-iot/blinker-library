import test from 'node:test';
import assert from 'node:assert/strict';
import { appExecutionAction, verifyExecutionEvidence } from './edge_gateway_execution_gate.mjs';

const evidence = () => ({ sameMqtt: true, ready: true, session: 'same', initialSession: 'same', demandOwners: 0,
  events: [1, 2, 3].map(challengeId => ({ phase: 'down', execution: true, operation: 2,
    incarnationDigest: 'one', challengeId, leaseMillis: challengeId === 3 ? 0 : 29900 })) });

test('execution evidence requires distinct challenges, bounded lease and final release', () => {
  verifyExecutionEvidence(evidence());
  for (const mutate of [s => s.demandOwners++, s => s.sameMqtt = false, s => s.ready = false,
    s => s.session = 'replacement', s => s.events[1].challengeId = 1,
    s => s.events[1].leaseMillis = 30001, s => s.events.pop(),
    s => s.events.push({ phase: 'down', kind: 0x12 })]) {
    const state = evidence(); mutate(state); assert.throws(() => verifyExecutionEvidence(state));
  }
});

test('App uses exact production Session owners and releases retained handles on cleanup', async t => {
  let stops = 0;
  const owners = [], cloud = { async ensureReady() {}, async stop() { stops++; }, client: { session: {
    acquireConnectionDemand(target) {
      assert.equal(target, 'child');
      const owner = { accepted: Promise.resolve(), closed: Promise.resolve('session-ended'),
        released: false, release() { this.released = true; } };
      owners.push(owner); return owner;
    },
  } } };
  const page = { available: true, edgeHubLogicalDeviceId: 'hub' };
  const previous = { ng: globalThis.ng, document: globalThis.document };
  globalThis.document = { querySelector: value => value };
  globalThis.ng = { getComponent: element => element === 'app-edge-gateway-enrollment'
    ? page : { deviceService: { deviceUi: { deviceV2: cloud } } } };
  t.after(() => { globalThis.ng = previous.ng; globalThis.document = previous.document; delete globalThis.__edgeDemandOwners; });
  const input = { hub: 'hub', child: 'child', owners: 2 };
  assert.equal((await appExecutionAction({ ...input, action: 'acquire' })).owners, 2);
  assert.equal((await appExecutionAction({ ...input, action: 'release-one' })).owners, 1);
  assert(owners[1].released && !owners[0].released);
  await appExecutionAction({ ...input, action: 'close' });
  assert(owners.every(o => o.released)); assert.equal(globalThis.__edgeDemandOwners, undefined);
  await appExecutionAction({ ...input, action: 'acquire', owners: 1 });
  assert.equal((await appExecutionAction({ ...input, action: 'end-session' })).reason, 'session-ended');
  assert.equal(stops, 2); assert.equal(owners.at(-1).released, false);
});
