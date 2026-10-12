import test from 'node:test';
import assert from 'node:assert/strict';
import vm from 'node:vm';
import { firstConnectAppGate } from './first_connect_app_gate.mjs';

function fixture() {
  const calls = [];
  const nativeCalls = [];
  const session = { channel: { send() {} }, store: { snapshot: () => ({}) },
    synchronize() {}, synchronizeManifest() {}, acceptManifest() {}, synchronizeState() {} };
  const client = { authenticate: async () => session };
  const link = { connect: async () => { calls.push('gatt'); } };
  const ble = { accountLinks: new Map(), accountSessions: new Map(), opening: false,
    hasActiveCredential: async () => true, discoverDirect: async () => {},
    client: () => client, ensureReady: async () => {} };
  const ui = { ble, connections: new Map(), deviceV2: { state: { value: 'retrying' }, client: { session: {} } },
    supportsDirectBle: () => true, isCloudCapable: () => true,
    transport: () => ({ value: 'ble' }), connectCloud: async () => {} };
  const power = { key: 'power', value: false }, count = { key: 'buttonPresses', value: 0 };
  const p = { logicalDeviceId: 'logical-id', canControl: false,
    snapshot: { stateRevision: 1, stateFresh: true, manifestAccepted: true, endpoints: [power, count, { key: 'button' }] },
    value: field => field.value,
    setBoolean: async (_field, event) => { await ui.sendCommand('logical-id', 'power', event.detail.checked); },
    execute: async () => { await ui.sendCommand('logical-id', 'button', null); } };
  ui.sendCommand = async (_id, key, value) => {
    if (key === 'power') power.value = value;
    else count.value++;
    p.snapshot.stateRevision++;
  };
  const location = { pathname: '/home/device' };
  const root = { deviceService: { deviceUi: ui }, dataService: { auth: { uuid: 'owner' } },
    userService: { getAllInfo: async () => { calls.push('refresh'); } },
    viewService: { router: { navigateByUrl: async path => {
      location.pathname = path;
      if (path === '/home/device') { p.canControl = false; return; }
      p.canControl = false;
      const current = ble.client(link);
      void (async () => {
        await link.connect({ device: { deviceId: 'native-address' } });
        const connectedSession = await current.authenticate();
        await connectedSession.synchronize();
        if (location.pathname !== '/home/device') p.canControl = true;
      })().catch(() => { p.error = true; });
    } } } };
  const original = { factory: ble.client, connect: link.connect, authenticate: client.authenticate };
  const cap = { nativePromise: async (...args) => { nativeCalls.push(args); } };
  const context = vm.createContext({ location, performance, clearTimeout, DOMException,
    setTimeout: (callback, ms) => setTimeout(callback, Math.min(ms, 5)),
    document: { querySelector: selector => selector },
    ng: { getComponent: selector => selector === 'app-root' ? root : p },
    Capacitor: cap });
  const run = options => {
    context.input = { mode: 'open', logicalDeviceId: 'logical-id', ownerId: 'owner',
      expectedTransport: 'ble', measureBlePhases: true, ...options };
    return vm.runInContext(`(${firstConnectAppGate.toString()})(input)`, context);
  };
  return { run, calls, nativeCalls, original, ble, link, client, session, cap, p, ui, context };
}

test('pure BLE measurement is explicit and never bypasses real credential or BLE capability', async () => {
  const f = fixture();
  f.ui.isCloudCapable = () => false;
  await assert.rejects(f.run({}), /capabilities/);
  assert.equal((await f.run({ requireCloudCapable: false })).ready, true);
  await f.run({ mode: 'close', requireCloudCapable: false });
  f.ble.hasActiveCredential = async () => false;
  await assert.rejects(f.run({ requireCloudCapable: false }), /credential/);
  f.ble.hasActiveCredential = async () => true;
  f.ui.supportsDirectBle = () => false;
  await assert.rejects(f.run({ requireCloudCapable: false }), /capabilities/);
  for (const options of [{ requireCloudCapable: 'false' },
    { requireCloudCapable: false, expectedTransport: 'cloud' },
    { requireCloudCapable: false, measureBlePhases: false }]) await assert.rejects(f.run(options));
});

test('default observer refreshes account and never changes connection policy', async () => {
  const f = fixture();
  const result = await f.run({});
  assert.equal(result.ready, true);
  assert.deepEqual(f.calls, ['refresh', 'gatt']);
  assert.equal(f.nativeCalls.length, 0);
  await f.run({ mode: 'close' });
  assert.equal(f.ble.client, f.original.factory);
  assert.equal(f.link.connect, f.original.connect);
  assert.equal(f.client.authenticate, f.original.authenticate);
});

test('cached BLE account still requires admission and never adds a priority hint', async () => {
  const f = fixture();
  const result = await f.run({ useCachedAccountForBle: true });
  assert.equal(result.ready, true);
  assert.deepEqual(f.calls, ['gatt']);
  assert.equal(f.nativeCalls.length, 0);
  const names = result.events.map(event => event.name + ':' + event.state);
  assert(names.indexOf('gatt:resolved') < names.indexOf('authentication:start'));
  await f.run({ mode: 'close' });
  f.ble.hasActiveCredential = async () => false;
  await assert.rejects(f.run({ useCachedAccountForBle: true }), /credential/);
});

test('rejects wrong owner and non-BLE shortcuts before navigation', async () => {
  const f = fixture();
  for (const options of [
    { ownerId: 'other-owner' },
    { useCachedAccountForBle: true, expectedTransport: 'cloud' },
    { useCachedAccountForBle: true, measureBlePhases: false },
    { cancelAtPhase: 'authentication' }, { mode: 'cancel', cancelAtPhase: 'unknown' },
    { mode: 'cancel', cancelAtPhase: 'gatt', expectedTransport: 'cloud' },
  ]) await assert.rejects(f.run(options));
  assert.equal(f.calls.length, 0);
  assert.equal(f.nativeCalls.length, 0);
});

test('rejects retired priority parameters before navigation or installing any observer', async () => {
  const f = fixture();
  for (const options of [
    { testConnectionPriority: 1 }, { testConnectionPriority: 0 }, { testConnectionPriority: undefined },
    { testPriorityRejection: true }, { testPriorityRejection: false }, { testPriorityRejection: undefined },
    { testPriorityRejection: 'yes', testConnectionPriority: 1 },
    { mode: 'cancel', cancelAtPhase: 'gatt', testConnectionPriority: 1 },
  ]) {
    await assert.rejects(f.run(options), /Legacy BLE priority experiment removed/);
    assert.equal(f.context.location.pathname, '/home/device');
    assert.equal(f.context.__blinkerFirstConnectGate, undefined);
    assert.equal(f.ble.client, f.original.factory);
    assert.equal(f.link.connect, f.original.connect);
    assert.equal(f.client.authenticate, f.original.authenticate);
  }
  // Presence detection must also reject inherited or getter parameters without
  // reading their values, not silently turn an old experiment into a baseline.
  let getterCalls = 0;
  for (const input of [Object.create({ testConnectionPriority: 1 }),
    Object.defineProperty({}, 'testPriorityRejection', { get() { getterCalls++; throw Error('private-parameter'); } })]) {
    Object.assign(input, { mode: 'open', logicalDeviceId: 'logical-id', ownerId: 'owner',
      expectedTransport: 'ble', measureBlePhases: true });
    f.context.input = input;
    await assert.rejects(vm.runInContext(`(${firstConnectAppGate.toString()})(input)`, f.context),
      /Legacy BLE priority experiment removed/);
  }
  assert.equal(getterCalls, 0);
  assert.equal(f.calls.length, 0);
  assert.equal(f.nativeCalls.length, 0);
});

test('phase cancellation is observed before exit and late authentication cannot leave wrappers installed', async () => {
  const f = fixture();
  let finish;
  const session = await f.original.authenticate();
  f.client.authenticate = () => new Promise(resolve => { finish = resolve; });
  const originalAuthenticate = f.client.authenticate;
  const cancelled = await f.run({ mode: 'cancel', cancelAtPhase: 'authentication' });
  assert.equal(cancelled.phaseReached, true);
  assert.equal(cancelled.route, '/home/device');
  assert.equal(cancelled.ready, false);
  assert(!cancelled.events.some(e => e.name === 'authentication' && e.state === 'resolved'));
  assert.equal(f.client.authenticate, originalAuthenticate);
  assert.equal(f.link.connect, f.original.connect);
  finish(session); await new Promise(resolve => setImmediate(resolve));
  assert.equal(f.client.authenticate, originalAuthenticate);
  assert.equal(f.nativeCalls.length, 0); // The observer never issues a connection hint.
});

test('a UI-swallowed failed property is not an ACK and must not dispatch Action', async () => {
  const f = fixture();
  f.p.setBoolean = async () => {}; // Page may swallow a failed/no-op command.
  await f.run({});
  const result = await f.run({ mode: 'commands' });
  assert.equal(result.propertyAcknowledged, false);
  assert.equal(result.commandsFailed, true);
  assert.equal(f.p.snapshot.endpoints[1].value, 0);
  await f.run({ mode: 'close' });
});

test('clean exit before the requested authentication phase is not a successful phase-cancel gate', async () => {
  const f = fixture();
  f.link.connect = async () => { throw Error('GATT unavailable'); };
  const originalConnect = f.link.connect;
  const result = await f.run({ mode: 'cancel', cancelAtPhase: 'authentication' });
  assert.equal(result.phaseReached, false);
  assert.equal(result.cancelFailed, true);
  assert.equal(result.route, '/home/device');
  assert.equal(result.links, 0);
  assert.equal(f.link.connect, originalConnect);
  assert.equal(f.client.authenticate, f.original.authenticate);
  assert.equal(f.ble.client, f.original.factory);
});

test('a UI-swallowed failed Action is unknown, never retried or counted as ACK', async () => {
  const f = fixture();
  f.p.execute = async () => {};
  await f.run({});
  const result = await f.run({ mode: 'commands' });
  assert.equal(result.propertyAcknowledged, true);
  assert.equal(result.actionAcknowledged, false);
  assert.equal(result.commandsFailed, true);
  await assert.rejects(f.run({ mode: 'commands' }), /once-only/);
  assert.equal(f.p.snapshot.endpoints[1].value, 0);
  await f.run({ mode: 'close' });
});

test('wrong page identity and stale/unaccepted state cannot authorize test commands', async () => {
  const f = fixture();
  await f.run({});
  f.p.logicalDeviceId = 'other-device';
  await assert.rejects(f.run({ mode: 'commands' }), /Ready/);
  f.p.logicalDeviceId = 'logical-id';
  f.p.snapshot.stateFresh = false;
  await assert.rejects(f.run({ mode: 'commands' }), /Ready/);
  f.p.snapshot.stateFresh = true; f.p.snapshot.manifestAccepted = false;
  await assert.rejects(f.run({ mode: 'commands' }), /Ready/);
  assert.equal(f.p.snapshot.endpoints[1].value, 0);
  await f.run({ mode: 'close' });
});

test('measurement needs no native priority API and preserves once-only controls', async () => {
  const f = fixture();
  delete f.cap.nativePromise;
  const result = await f.run({});
  assert.equal(result.ready, true);
  assert.equal((await f.run({ mode: 'commands' })).commandsFailed, false);
  await assert.rejects(f.run({ mode: 'commands' }), /once-only/);
  assert.equal(f.nativeCalls.length, 0);
  await f.run({ mode: 'close' });
});

test('late GATT completion after phase cancellation cannot add observations or leave wrappers', async () => {
  const f = fixture();
  let finish;
  f.link.connect = () => new Promise(resolve => { finish = resolve; });
  const originalConnect = f.link.connect;
  const result = await f.run({ mode: 'cancel', cancelAtPhase: 'gatt' });
  assert.equal(result.phaseReached, true);
  assert.equal(result.ready, false);
  finish(); await new Promise(resolve => setImmediate(resolve));
  assert.equal(f.nativeCalls.length, 0);
  assert.equal(f.link.connect, originalConnect);
  assert.equal(f.ble.client, f.original.factory);
  assert.equal(f.client.authenticate, f.original.authenticate);
});

function detailedFixture({ packetBytes = 244, fetch = false, cacheMatches = true } = {}) {
  const f = fixture();
  const privateManifest = { revision: 7, fingerprint: 'private-fingerprint', fields: ['private-field'] };
  const cache = { hello: { revision: 7, fingerprint: cacheMatches ? privateManifest.fingerprint : 'other-fingerprint' },
    load: () => privateManifest };
  const state = { manifestAccepted: false, stateFresh: false };
  f.session.manifestCache = cache;
  f.session.store.snapshot = () => state;
  const sendPromise = Promise.resolve(), receivePromise = Promise.resolve(Uint8Array.of(88, 99));
  f.link.sendRecord = () => sendPromise;
  f.link.receiveRecord = () => receivePromise;
  f.link.maxPacketSize = async () => packetBytes;
  f.link.preparePacketNegotiation = () => 'original-prepare-result';
  f.link.applyPacketSize = () => 'original-apply-result';
  const originals = Object.fromEntries(['sendRecord', 'receiveRecord', 'maxPacketSize',
    'preparePacketNegotiation', 'applyPacketSize'].map(name => [name, f.link[name]]));
  const originalLoad = cache.load, originalChannelSend = f.session.channel.send;
  const header = (kind, size) => {
    const record = new Uint8Array(size);
    record.set([0x42, 0x4b, 2, kind, 0, 10, 0, 1, 0, size - 10]);
    return record;
  };
  f.client.authenticate = async () => {
    const capacity = await f.link.maxPacketSize();
    for (const [kind, size] of [[1, 33], [4, 58], [4, 49]]) {
      // Observers must not replace promise identity or modify wire records.
      const record = header(kind, size);
      assert.equal(f.link.sendRecord(record), sendPromise);
      await sendPromise;
      assert.equal(f.link.receiveRecord(), receivePromise);
      await receivePromise;
      assert.equal(record[3], kind);
    }
    if (capacity > 20) {
      assert.equal(f.link.preparePacketNegotiation(capacity), 'original-prepare-result');
      await f.link.sendRecord(new Uint8Array(60).fill(88));
      await f.link.receiveRecord();
      assert.equal(f.link.applyPacketSize(capacity), 'original-apply-result');
    }
    return f.session;
  };
  f.session.synchronize = async () => {
    await f.session.synchronizeManifest();
    await f.session.synchronizeState();
  };
  f.session.synchronizeManifest = async () => {
    const candidate = cache.load();
    if (fetch || candidate.fingerprint !== cache.hello.fingerprint)
      await f.session.channel.send({ kind: 2, body: Uint8Array.of(22, 33) });
    await f.session.acceptManifest();
  };
  f.session.acceptManifest = async () => {
    await f.session.channel.send({ kind: 6, body: Uint8Array.of(44, 55) });
    state.manifestAccepted = true;
  };
  f.session.synchronizeState = async () => {
    await f.session.channel.send({ kind: 16, body: Uint8Array.of(66, 77) });
    state.stateFresh = true;
  };
  return { ...f, cache, originals, originalLoad, originalChannelSend };
}

test('splits real bound Direct bootstrap, accepted packet size, cache ACK and page readiness without private fields', async () => {
  const f = detailedFixture();
  const result = await f.run({});
  const records = result.events.filter(event => event.name === 'bootstrap-record');
  assert.deepEqual(Array.from(records, event => `${event.phase}:${event.state}`), [
    'hello:send', 'hello:received', 'method2-challenge:send', 'method2-challenge:received',
    'method2-authorize:send', 'method2-authorize:received', 'authenticated-hello:send', 'authenticated-hello:received',
  ]);
  assert.equal(result.events.find(event => event.name === 'packet-capacity').packetBytes, 244);
  assert.equal(result.events.find(event => event.name === 'packet-accepted').packetBytes, 244);
  const starts = result.events.filter(event => event.name === 'record-send' && event.state === 'start');
  assert.deepEqual(Array.from(starts, event => [event.phase, event.recordBytes, event.packetBytes,
    event.estimatedFragments, event.estimatedInterFragmentWaitMs]), [
    ['hello', 33, 20, 3, 60], ['method2-challenge', 58, 20, 4, 90],
    ['method2-authorize', 49, 20, 4, 90], ['authenticated-hello', 60, 20, 4, 90],
  ]);
  for (const name of ['record-send', 'record-receive']) {
    const settled = result.events.filter(event => event.name === name && event.state === 'resolved');
    assert.deepEqual(Array.from(settled, event => event.phase), Array.from(starts, event => event.phase));
    assert(settled.every(event => Number.isInteger(event.durationMs) && event.durationMs >= 0));
  }
  const cache = result.events.find(event => event.name === 'manifest-cache-result');
  assert.equal(cache.outcome, 'hit'); assert.equal(cache.accepted, true); assert.equal(cache.requests, 0);
  const names = result.events.map(event => `${event.name}:${event.state}`);
  assert(names.indexOf('state:resolved') < names.indexOf('page-ready:observed'));
  assert(result.events.some(event => event.name === 'phase-coverage' && event.bootstrapRecords
    && event.noise === 'not-used-on-bound-direct-connect'));
  assert(!JSON.stringify(result.events).includes('private-'));
  assert(!JSON.stringify(result.events).includes('native-address'));
  assert.equal(f.nativeCalls.length, 0);
  await f.run({ mode: 'close' });
  for (const [name, original] of Object.entries(f.originals)) assert.equal(f.link[name], original);
  assert.equal(f.cache.load, f.originalLoad);
  assert.equal(f.session.channel.send, f.originalChannelSend);
});

test('ATT20 and actual Manifest fetch are observed rather than fabricated negotiation/cache success', async () => {
  for (const options of [{ packetBytes: 20, fetch: true }, { packetBytes: 20, cacheMatches: false }]) {
    const f = detailedFixture(options);
    const result = await f.run({});
    assert.equal(result.ready, true);
    assert.equal(result.events.filter(event => event.name === 'bootstrap-record').length, 6);
    assert(!result.events.some(event => event.name === 'packet-accepted'));
    const cache = result.events.find(event => event.name === 'manifest-cache-result');
    assert.equal(cache.requests, 1); assert.equal(cache.accepted, true);
    assert.equal(cache.outcome, options.fetch ? 'fetch-after-cache' : 'fetch');
    await f.run({ mode: 'close' });
  }
});

test('bounds phase output, reports lost coverage and preserves late method completion after cleanup', async () => {
  const f = detailedFixture();
  await assert.rejects(f.run({ eventLimit: 1 }), /event limit/);
  const result = await f.run({ eventLimit: 16 });
  assert.equal(result.ready, true);
  assert.equal(result.events.length, 16); assert(result.eventsDropped > 0);
  await f.run({ mode: 'close' });
  const reentered = await f.run({ eventLimit: 2048 });
  assert.equal(reentered.eventsDropped, 0);
  await f.run({ mode: 'close' });
});

test('ordinary discovery, GATT, authentication and target-filtered observation return exact original promises', async () => {
  const f = fixture();
  const connect = Promise.resolve(), authenticated = Promise.resolve(f.session);
  const discover = Promise.resolve({ privateToken: 'private-token', deviceId: 'private-native-address' });
  const ready = Promise.resolve(), cloud = Promise.resolve('private-cloud-result');
  f.link.connect = () => connect;
  f.client.authenticate = () => authenticated;
  f.ble.discoverDirect = () => discover;
  f.ble.ensureReady = () => ready;
  f.ui.connectCloud = () => cloud;
  await f.run({});
  assert.equal(f.link.connect(), connect);
  assert.equal(f.client.authenticate(), authenticated);
  assert.equal(f.ble.discoverDirect(), discover);
  assert.equal(f.ble.ensureReady('logical-id'), ready);
  assert.equal(f.ui.connectCloud('logical-id'), cloud);
  const events = f.context.__blinkerFirstConnectGate.events;
  const before = events.length;
  assert.equal(f.ui.connectCloud('other-private-id'), cloud);
  assert.equal(events.length, before);
  await new Promise(resolve => setImmediate(resolve));
  assert(!JSON.stringify(events).includes('private-'));
  await f.run({ mode: 'close' });
});

test('fixed native scan categories and cancellation are recorded without raw error text or identifiers', async () => {
  const f = fixture();
  let error;
  const original = () => Promise.reject(error);
  f.ble.discoverDirect = original;
  await f.run({});
  for (const [message, category] of [
    ['BLE_SCAN_BACKGROUND', 'scan-background'], ['BLE_SCAN_BUSY', 'scan-busy'],
    ['BLE_SCAN_BUDGET_INVALID', 'scan-budget-invalid'], ['BLE_SCAN_UNAVAILABLE', 'scan-unavailable'],
    ['BLE_SCAN_BUDGET_STORAGE_FAILED', 'scan-budget-storage-failed'], ['BLE_SCAN_THROTTLED', 'scan-throttled'],
    ['BLE_SCAN_PERMISSION', 'scan-permission'], ['BLE_SCAN_FAILED', 'scan-failed'],
    ['BLE_SCAN_CLOSE_FAILED', 'scan-close-failed'], ['BLE_SCAN_DEADLINE', 'scan-deadline'],
    ['BLE_SCAN_REGISTRATION_FAILED', 'scan-registration-failed'], ['BLE_SCAN_UNSUPPORTED', 'scan-unsupported'],
    ['BLE_SCAN_RESOURCES', 'scan-resources'], ['BLE_SCAN_CANCELLED', 'cancelled'],
    ['BLE_DIRECT_SCAN_CANCELLED', 'cancelled'], ['BLE_DIRECT_CONNECT_CANCELLED', 'cancelled'],
    ['BLE_DIRECT_SCAN_TIMEOUT', 'scan-timeout'], ['BLE_SCAN_BUSY private-token/private-id', 'unknown'],
    ['private-token/private-id', 'unknown'],
  ]) {
    error = Error(message);
    await assert.rejects(f.ble.discoverDirect(), actual => actual === error);
    const event = f.context.__blinkerFirstConnectGate.events.at(-1);
    assert.equal(event.name, 'discovery'); assert.equal(event.state, 'rejected');
    assert.equal(event.failureCategory, category);
    assert(Number.isInteger(event.durationMs) && event.durationMs >= 0);
  }
  error = new DOMException('private-token/private-id', 'AbortError');
  await assert.rejects(f.ble.discoverDirect(), actual => actual === error);
  assert.equal(f.context.__blinkerFirstConnectGate.events.at(-1).failureCategory, 'cancelled');
  assert(!JSON.stringify(f.context.__blinkerFirstConnectGate.events).includes('private-'));
  await f.run({ mode: 'close' });
  assert.equal(f.ble.discoverDirect, original);
});

test('hostile rejection and result getters cannot alter synchronous throws or original return values', async () => {
  const f = fixture();
  let getterCalls = 0, current;
  const hostile = Object.create(null, {
    message: { get() { getterCalls++; throw Error('private-token'); } },
    name: { get() { getterCalls++; throw Error('private-id'); } },
  });
  const original = () => { if (current === hostile) throw hostile; return current; };
  f.ble.discoverDirect = original;
  await f.run({});
  current = hostile;
  assert.throws(() => f.ble.discoverDirect(), actual => actual === hostile);
  assert.equal(getterCalls, 0);
  assert.equal(f.context.__blinkerFirstConnectGate.events.at(-1).failureCategory, 'unknown');
  current = Object.create(null, { then: { get() { throw Error('private-token'); } } });
  assert.equal(f.ble.discoverDirect(), current);
  assert.equal(f.context.__blinkerFirstConnectGate.events.at(-1).name, 'measurement-observation');
  assert(!JSON.stringify(f.context.__blinkerFirstConnectGate.events).includes('private-'));
  await f.run({ mode: 'close' });
});

test('an observation failure preserves a resolved authentication result and late rejection stops at exit', async () => {
  const f = fixture();
  Object.defineProperty(f.session, 'manifestCache', { get() { throw Error('private-observer-token'); } });
  const authentication = Promise.resolve(f.session);
  f.client.authenticate = () => authentication;
  let rejectDiscovery;
  const discovery = new Promise((_resolve, reject) => { rejectDiscovery = reject; });
  f.ble.discoverDirect = () => discovery;
  const result = await f.run({});
  assert.equal(result.ready, true);
  assert(result.events.some(event => event.name === 'measurement-observation' && event.state === 'unavailable'));
  assert.equal(f.client.authenticate(), authentication);
  assert.equal(f.ble.discoverDirect(), discovery);
  const gate = f.context.__blinkerFirstConnectGate;
  const savedFactory = f.ble.client, savedDiscovery = f.ble.discoverDirect;
  await f.run({ mode: 'close' });
  const afterExit = JSON.stringify(gate.events), restores = gate.restore.length;
  rejectDiscovery(Error('BLE_SCAN_RESOURCES'));
  await assert.rejects(discovery, /BLE_SCAN_RESOURCES/);
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(JSON.stringify(gate.events), afterExit);
  assert.equal(savedDiscovery(), discovery);
  assert.equal(savedFactory(f.link), f.client);
  assert.equal(gate.restore.length, restores);
  assert(!afterExit.includes('private-observer-token'));
});
