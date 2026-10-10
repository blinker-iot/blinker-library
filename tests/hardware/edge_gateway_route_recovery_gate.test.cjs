'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { installRouteRecoveryGate } = require('./edge_gateway_route_recovery_gate.cjs');

// Unit-test interception/cleanup independently from the sibling Service build.
// These are codec stubs, NOT CBOR/BBP2 fixtures. The live runner loads the actual
// Broker codecs; their wire contracts have separate golden/lifecycle gates.
const protocol = {
  decodeGatewayDirectYield: bytes => ({ childLogicalDeviceId: 'child', operation: bytes[1],
    controlId: 9, challengeId: 7, leaseMillis: bytes[2] ? 30000 : 0 }),
  decodeGatewayExecutionDemand: bytes => ({ childLogicalDeviceId: 'child', operation: bytes[1],
    challengeId: 7, leaseMillis: bytes[2] ? 30000 : 0, incarnation: Buffer.alloc(16, 1) }),
  decodeGatewayRouteLease: bytes => ({ childLogicalDeviceId: 'child', acknowledgment: bytes[2] === 1,
    generation: Buffer.alloc(8, 1), challenge: Buffer.alloc(8, bytes[3] || 1) }),
  decodeGatewayRouteClosure: bytes => ({ childLogicalDeviceId: 'child', acknowledgment: bytes[1] === 1, generation: Buffer.alloc(8, 1) }),
  decodeGatewayRouteAdmission: () => ({ childLogicalDeviceId: 'child', retiredGeneration: Buffer.alloc(8) }),
  decodeChildRouteEnvelope: bytes => ({ childLogicalDeviceId: 'child',
    correlationId: bytes.subarray(1, 17), innerBbp2Frame: bytes.subarray(17) })
};
const parseBbp2Frame = bytes => ({ kind: bytes[0], sequence: bytes[1] });

function fixture(t, fault, durationMillis) {
  const claims = { sessionId: 'mqtt', managementReady: true, routeSubscribeTopic: 'down' };
  const broker = { publish(packet, callback) { sent.push(Buffer.from(packet.payload)); callback?.(null); } };
  const client = { broker, closed: false }, sent = [], accepted = [];
  let current = claims, ready = { claims: { sessionId: 'edge:mqtt:old',
    stateValues: { buttonPresses: { value: 10 } }, stateRevision: 0 } };
  const first = ready;
  first.isOpen = () => ready === first;
  const envelope = kind => Buffer.concat([Buffer.from([0x88]), Buffer.alloc(16, 1), Buffer.from([kind, 1])]);
  const admission = Buffer.from([0x8a, 2]);
  const routes = {
    async acceptEdgeGatewayRouteUp(aedes, source, identity, payload) { accepted.push(Buffer.from(payload)); return null; },
    revokeEdgeGatewayRouteForChild(tenant, hub, target) {
      assert.equal(tenant, 'tenant'); assert.equal(hub, 'hub');
      assert.equal(target.childLogicalDeviceId, 'child');
      assert.deepEqual(target.operationId, Buffer.alloc(16, 1));
      assert.equal(target.topologyVersion, 1);
      ready = null; return true;
    },
    async admitEdgeGatewayRoute(aedes, source, identity) {
      assert.equal(identity, claims);
      await new Promise(resolve => aedes.publish({ topic: 'down', payload: admission }, resolve));
      ready = { claims: { sessionId: 'edge:mqtt:new', stateValues: {}, stateRevision: 1 } };
      return true;
    }
  };
  const originalPublish = broker.publish, originalAccept = routes.acceptEdgeGatewayRouteUp;
  const input = { fault, durationMillis, hub: 'hub', child: 'child', tenant: 'tenant', shard: 1,
    recoveryTarget: { operationId: '01'.repeat(16), topologyVersion: 1 } };
  const gateProtocol = Object.fromEntries(Object.entries(protocol).map(([name, decode]) => [name, bytes => {
    const decoded = decode(bytes);
    if (bytes.at(-1) === 0xee) decoded.childLogicalDeviceId = 'sibling';
    return decoded;
  }]));
  const gate = installRouteRecoveryGate(input, {
    registry: { getV2EdgeHubRouteConnection: () => ({ client, claims }), getV2Connection: () => current },
    channels: { getReadyV2VirtualDeviceChannel: () => ready }, routes, protocol: gateProtocol, frame: parseBbp2Frame,
    hash: bytes => require('crypto').createHash('sha256').update(bytes).digest('hex')
  });
  t.after(() => gate.close());
  return { gate, input, sent, accepted, broker, client, claims, routes, envelope, admission, originalPublish, originalAccept,
    advanceCounter: value => { ready.claims.stateValues.buttonPresses = { value }; },
    expire: () => { ready = null; },
    replace: () => { current = { ...claims }; },
    down: payload => new Promise(resolve => broker.publish({ topic: 'down', payload }, resolve)),
    up: (payload, source = client) => routes.acceptEdgeGatewayRouteUp(broker, source, claims, payload, 1) };
}

test('execution controls remain distinct from child BBP and lease evidence', async t => {
  const f = fixture(t, 'observe');
  const challenge = Buffer.from([0x8b, 1, 0]), decision = Buffer.from([0x8b, 2, 1]);
  await f.up(challenge); await f.down(decision);
  assert.deepEqual(f.accepted, [challenge]); assert.deepEqual(f.sent, [decision]);
  const events = f.gate.snapshot().events.filter(e => e.execution);
  assert.equal(events.length, 3);
  assert(events.every(e => e.kind === undefined && !e.lease));
  assert.equal(events.at(-1).leaseMillis, 30000);
  assert.equal(f.gate.snapshot().dropped, false);
});

test('execution release loss drops only zero decisions, not positive, Probe or reachability', async t => {
  const f = fixture(t, 'execution-release-loss');
  const positive = Buffer.from([0x8b, 2, 1]), release = Buffer.from([0x8b, 2, 0]);
  const probe = Buffer.from([0x8b, 0, 0]), reachability = Buffer.from([0x8a, 1, 0]);
  for (const bytes of [positive, release, release, probe, reachability]) await f.down(bytes);
  assert.deepEqual(f.sent, [positive, probe, reachability]);
  assert.equal(f.gate.snapshot().events.filter(e => e.phase === 'drop-down').length, 2);
});

test('runtime State gate drops one Patch, one State request and only the second Action ACK', async t => {
  const f = fixture(t, 'runtime-state');
  const first = f.envelope(0x12), second = Buffer.from(first); second[16] = 2;
  await f.down(first); await f.down(first); // Exact retry is not another Action.
  await f.up(f.envelope(0x14));
  const patch = f.envelope(0x11);
  await f.up(patch); await f.up(patch);
  await f.down(second);
  const secondAck = Buffer.from(second); secondAck[17] = 0x14;
  await f.up(secondAck); await f.up(secondAck);
  const state = f.envelope(0x10);
  await f.down(state); await f.down(state);
  const snapshot = f.gate.snapshot();
  assert(snapshot.patchDropped && snapshot.stateDropped && snapshot.commandAckDropped);
  assert.equal(f.accepted.length, 3);
  assert.equal(f.sent.length, 4);
  f.advanceCounter(12); // The initial claims object is mutable; compare to its captured value.
  assert.equal((await f.gate.waitReady()).waitResult, 'ready');
  assert.equal(f.gate.snapshot().session, snapshot.initialSession);
});

for (const [fault, kind] of [['admission', null], ['manifest', 2], ['hello', 1]]) {
  test(`${fault}: drop once, preserve bytes and restore both hooks`, async t => {
    const f = fixture(t, fault), payload = kind === null ? f.admission : f.envelope(kind);
    const send = fault === 'hello' ? f.up : f.down;
    await send(payload); await send(payload);
    const forwarded = fault === 'hello' ? f.accepted : f.sent;
    assert.equal(forwarded.length, 1); assert.deepEqual(forwarded[0], payload);
    const trace = f.gate.snapshot(); assert.equal(trace.dropped, true);
    const events = trace.events.filter(x => x.digest);
    assert.equal(events[0].digest, events[1].digest);
    f.gate.close(); assert.equal(f.broker.publish, f.originalPublish); assert.equal(f.routes.acceptEdgeGatewayRouteUp, f.originalAccept);
  });
}

test('other client and other topic are never intercepted', async t => {
  const f = fixture(t, 'hello'), payload = f.envelope(1);
  await f.up(payload, {});
  await new Promise(resolve => f.broker.publish({ topic: 'elsewhere', payload }, resolve));
  assert.equal(f.accepted.length, 1); assert.equal(f.sent.length, 1);
  assert.equal(f.gate.snapshot().dropped, false);
});

for (const fault of ['admission', 'hello', 'manifest', 'bootstrap-terminal', 'unknown-action',
  'runtime-state', 'lease-loss-all', 'execution-release-loss']) {
  test(`${fault}: shared-Hub sibling controls and business pass unchanged without consuming the fault`, async t => {
    const f = fixture(t, fault), events = f.gate.snapshot().events.length;
    const sibling = bytes => Buffer.concat([bytes, Buffer.from([0xee])]);
    const controls = [Buffer.from([0x8c, 2, 1]), Buffer.from([0x8b, 2, 0]), Buffer.from([0x8a, 1, 1]), Buffer.from([0x89, 1])];
    const business = [1, 2, 0x10, 0x11, 0x12, 0x14].map(f.envelope);
    const down = [f.admission, ...controls, ...business].map(sibling);
    const up = [...controls, ...business].map(sibling);
    for (const bytes of down) await f.down(bytes);
    for (const bytes of up) await f.up(bytes);
    assert.deepEqual(f.sent, down); assert.deepEqual(f.accepted, up);
    assert.equal(f.gate.snapshot().events.length, events);
    assert.equal(f.gate.snapshot().dropped, false);
    assert.equal(f.gate.snapshot().recovered, false);
  });
}

test('recovery rejects missing or stale exact topology without a Hub-wide fallback', async t => {
  const f = fixture(t, 'observe');
  const target = f.input.recoveryTarget;
  delete f.input.recoveryTarget;
  await assert.rejects(f.gate.recover(), /GATE_RECOVERY_TARGET/);
  f.input.recoveryTarget = target;
  f.routes.revokeEdgeGatewayRouteForChild = () => false;
  f.routes.removeEdgeGatewayRoute = () => assert.fail('must never retire shared Hub');
  await assert.rejects(f.gate.recover(), /GATE_RECOVERY_TARGET_CHANGED/);
  assert.equal(f.gate.snapshot().recovered, false);
  assert.equal(f.gate.snapshot().ready, true);
});

test('Direct control is not parsed as a BBP envelope or an execution release', async t => {
  const f = fixture(t, 'execution-release-loss');
  const decision = Buffer.from([0x8c, 2, 0]), receipt = Buffer.from([0x8c, 3, 1]);
  await f.down(decision); await f.up(receipt);
  assert.deepEqual(f.sent, [decision]); assert.deepEqual(f.accepted, [receipt]);
  assert.equal(f.gate.snapshot().dropped, false);
  const events = f.gate.snapshot().events.filter(event => event.direct);
  assert.equal(events.length, 2); assert(events.every(event => !event.execution && event.kind === undefined));
});

test('child recovery does not retire a sibling channel or MQTT owner', async t => {
  const f = fixture(t, 'observe');
  const sibling = { open: true }, retire = f.routes.revokeEdgeGatewayRouteForChild;
  f.routes.removeEdgeGatewayRoute = () => { sibling.open = false; assert.fail('shared Hub retired'); };
  f.routes.revokeEdgeGatewayRouteForChild = (...args) => { assert.equal(sibling.open, true); return retire(...args); };
  await f.gate.recover();
  assert.equal(sibling.open, true);
  assert.equal(f.gate.snapshot().sameMqtt, true);
});

test('lease challenge and proof are not confused with admission, BBP or closure faults', async t => {
  const f = fixture(t, 'admission');
  const challenge = Buffer.from([0x8a, 1, 0]), proof = Buffer.from([0x8a, 1, 1]);
  await f.down(challenge); await f.up(proof);
  assert.deepEqual(f.sent, [challenge]);
  assert.deepEqual(f.accepted, [proof]);
  assert.equal(f.gate.snapshot().dropped, false);
  await f.down(f.admission);
  assert.equal(f.gate.snapshot().dropped, true);
  assert.equal(f.gate.snapshot().events.filter(x => x.lease).length, 3);
});

for (const fault of ['lease-request', 'lease-proof']) {
  test(`${fault}: drop exactly one proof exchange without affecting business or other controls`, async t => {
    const f = fixture(t, fault);
    const bytes = Buffer.from([0x8a, 1, fault === 'lease-proof' ? 1 : 0]);
    const send = fault === 'lease-proof' ? f.up : f.down;
    await send(bytes); await send(bytes);
    const forwarded = fault === 'lease-proof' ? f.accepted : f.sent;
    assert.deepEqual(forwarded, [bytes]);
    await f.down(f.envelope(0x12)); await f.up(f.envelope(0x14));
    assert.equal(f.gate.snapshot().events.filter(x => x.phase.startsWith('drop-')).length, 1);
  });
}

test('normal lease wait requires three distinct proof challenges on the same ready session', async t => {
  t.mock.timers.enable({ apis: ['setInterval', 'setTimeout'] });
  const f = fixture(t, 'observe');
  let finished = false;
  const waiting = f.gate.waitLeases().then(value => { finished = true; return value; });
  await f.up(Buffer.from([0x8a, 1, 1, 1])); await f.up(Buffer.from([0x8a, 1, 1, 1]));
  t.mock.timers.tick(50); await Promise.resolve();
  assert.equal(finished, false);
  await f.up(Buffer.from([0x8a, 1, 1, 2])); await f.up(Buffer.from([0x8a, 1, 1, 3]));
  t.mock.timers.tick(50);
  assert.equal((await waiting).waitResult, 'ready');
});

test('loss-all drops closure, proof and Hello; replay cannot restore route and explicit recovery lifts injection', async t => {
  t.mock.timers.enable({ apis: ['setInterval', 'setTimeout'] });
  const f = fixture(t, 'lease-loss-all');
  await f.up(Buffer.from([0x8a, 1, 1]));
  await f.up(Buffer.from([0x89, 0])); await f.down(Buffer.from([0x89, 1]));
  await f.up(f.envelope(1)); await f.up(f.envelope(0x11));
  assert.equal(f.accepted.length, 0); assert.equal(f.sent.length, 0);
  const offline = f.gate.waitOffline(); f.expire(); t.mock.timers.tick(50);
  assert.equal((await offline).waitResult, 'offline');
  await f.gate.replayLease(); assert.equal(f.gate.snapshot().ready, false);
  await f.gate.recover();
  await f.gate.replayLease();
  await assert.rejects(f.gate.replayLease(), /GATE_LEASE_REPLAY_PRECONDITION/);
  await f.up(f.envelope(1));
  assert.equal(f.accepted.length, 3); // Two explicit replays plus resumed Hello.
  assert.equal(f.gate.snapshot().ready, true);
});

test('closure receipt loss is isolated from bootstrap and preserves exact notices', async t => {
  const f = fixture(t, 'closure-receipt');
  const notice = Buffer.from([0x89, 0]), receipt = Buffer.from([0x89, 1]);
  await f.up(notice);
  await f.down(receipt); await f.down(receipt);
  await f.down(f.envelope(2));
  assert.deepEqual(f.accepted[0], notice);
  assert.deepEqual(f.sent[0], receipt);
  assert.equal(f.sent.length, 2);
  assert.equal(f.gate.snapshot().events.filter(x => x.phase === 'drop-down' && x.closure).length, 1);
  await f.gate.recover();
  await f.gate.replayClosure();
  assert.equal(f.gate.snapshot().events.at(-1).phase, 'old-closure-rejected');
  f.gate.close();
  await assert.rejects(f.gate.replayClosure(), /GATE_CLOSURE_REPLAY_PRECONDITION/);
});

test('MQTT identity replacement fences explicit recovery', async t => {
  const f = fixture(t, 'observe'); f.replace();
  await assert.rejects(f.gate.recover(), /GATE_RECOVERY_PRECONDITION/);
  assert.equal(f.gate.snapshot().sameMqtt, false);
});

test('context recovery executes once and preserves MQTT', async t => {
  const f = fixture(t, 'admission'); await f.gate.recover();
  assert.equal(f.gate.snapshot().sameMqtt, true); assert.equal(f.gate.snapshot().dropped, true);
  assert.notEqual(f.gate.snapshot().session, f.gate.snapshot().initialSession);
  await assert.rejects(f.gate.recover(), /GATE_RECOVERY_PRECONDITION/);
});

test('exhaust only one exact bootstrap request, allow the next generation', async t => {
  const f = fixture(t, 'bootstrap-exhaustion'), first = f.envelope(2);
  for (let i = 0; i < 4; i++) await f.down(first);
  assert.equal(f.sent.length, 0);
  const next = Buffer.from(first); next[1]++;
  await f.down(next); assert.deepEqual(f.sent, [next]);
});

test('unknown Action gate requires real matching ACK before retirement and exact old replay', async t => {
  const f = fixture(t, 'unknown-action'), command = f.envelope(0x12), ack = f.envelope(0x14);
  await assert.rejects(f.gate.replayOld(), /GATE_REPLAY_PRECONDITION/);
  await f.up(ack); assert.equal(f.gate.snapshot().recovered, false);
  await f.down(command); await f.up(ack);
  assert.equal(f.gate.snapshot().recovered, true); assert.equal(f.gate.snapshot().sameMqtt, true);
  await f.gate.replayOld(); assert.deepEqual(f.sent.at(-1), command);
  assert.deepEqual(f.accepted.at(-1), ack);
  f.gate.close(); await assert.rejects(f.gate.replayOld(), /GATE_REPLAY_PRECONDITION/);
});

test('terminal bootstrap gate drops both generations but never authority admission or Commands', async t => {
  const f = fixture(t, 'bootstrap-terminal'), first = f.envelope(2);
  const second = Buffer.from(first); second[1]++;
  for (const request of [first, second]) for (let i = 0; i < 4; i++) await f.down(request);
  assert.equal(f.sent.length, 0);
  await f.down(f.admission); await f.down(f.envelope(0x12));
  assert.equal(f.sent.length, 2);
  const drops = f.gate.snapshot().events.filter(value => value.phase === 'drop-down');
  assert.equal(drops.length, 8);
  assert.equal(new Set(drops.map(value => value.generation)).size, 2);
});

test('gate expires, releases hooks and does not extend itself on traffic', t => {
  t.mock.timers.enable({ apis: ['setTimeout', 'setInterval'] });
  const f = fixture(t, 'observe'); t.mock.timers.tick(120000);
  assert.equal(f.gate.snapshot().closed, true);
  assert.equal(f.broker.publish, f.originalPublish);
  assert.equal(f.routes.acceptEdgeGatewayRouteUp, f.originalAccept);
});

test('page observation allows a bounded three-minute run, not an unlimited hook', t => {
  t.mock.timers.enable({ apis: ['setTimeout', 'setInterval'] });
  const f = fixture(t, 'observe', 180000);
  t.mock.timers.tick(179999); assert.equal(f.gate.snapshot().closed, false);
  t.mock.timers.tick(1); assert.equal(f.gate.snapshot().closed, true);
  for (const durationMillis of [0, 999, 180001, Infinity, '180000']) {
    assert.throws(() => installRouteRecoveryGate({ durationMillis }, {}), /GATE_DURATION/);
  }
});

test('waiter requires a new ready session, with an independent bounded deadline', async t => {
  t.mock.timers.enable({ apis: ['setTimeout', 'setInterval'] });
  const f = fixture(t, 'observe');
  const timeout = f.gate.waitReady(); t.mock.timers.tick(45000);
  assert.equal((await timeout).waitResult, 'deadline');
  const success = f.gate.waitReady(); await f.gate.recover(); t.mock.timers.tick(50);
  assert.equal((await success).waitResult, 'ready');
});

test('closing a gate interrupts its wait without changing MQTT', async t => {
  t.mock.timers.enable({ apis: ['setTimeout', 'setInterval'] });
  const f = fixture(t, 'observe'), wait = f.gate.waitReady();
  f.gate.close(); t.mock.timers.tick(50);
  assert.equal((await wait).waitResult, 'interrupted');
});
