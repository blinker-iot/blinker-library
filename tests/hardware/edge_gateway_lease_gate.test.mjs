import test from 'node:test';
import assert from 'node:assert/strict';
import { verifyLeaseEvidence } from './edge_gateway_lease_gate.mjs';

const healthy = () => ({ sameMqtt: true, ready: true, waitResult: 'ready', session: 'a', initialSession: 'a',
  events: [1, 2, 3].map(challengeDigest => ({ phase: 'accepted', lease: true, challengeDigest })) });
test('lease evidence needs three distinct challenges, same MQTT and no business Action', () => {
  verifyLeaseEvidence('observe', healthy());
  for (const changed of [{ sameMqtt: false }, { session: 'b' }, { events: Array(3).fill({ phase: 'accepted', lease: true, challengeDigest: 'same' }) },
    { events: [...healthy().events, { phase: 'down', kind: 0x12 }] }]) {
    assert.throws(() => verifyLeaseEvidence('observe', { ...healthy(), ...changed }));
  }
});
test('single-drop gates require a bounded exact retransmission', () => {
  for (const [fault, phase] of [['lease-request', 'down'], ['lease-proof', 'up']]) {
    const events = [...healthy().events, { phase: `drop-${phase}`, lease: true, digest: 'x', ms: 100 },
      { phase, lease: true, digest: 'x', ms: 5100 }];
    verifyLeaseEvidence(fault, { ...healthy(), events });
    events.at(-1).digest = 'changed';
    assert.throws(() => verifyLeaseEvidence(fault, { ...healthy(), events }));
  }
});
test('loss-all requires genuine closure/Hello/proof loss and a retired projection', () => {
  const state = { sameMqtt: true, ready: false, waitResult: 'offline', events: [
    { phase: 'drop-up', lease: true }, { phase: 'drop-up', closure: true }, { phase: 'drop-up', kind: 1 },
  ] };
  verifyLeaseEvidence('lease-loss-all', state);
  for (const changed of [{ ready: true }, { events: state.events.slice(1) }, { events: [...state.events, { phase: 'accepted' }] }]) {
    assert.throws(() => verifyLeaseEvidence('lease-loss-all', { ...state, ...changed }));
  }
});
