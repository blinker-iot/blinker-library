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
        await current.authenticate();
        if (location.pathname !== '/home/device') p.canControl = true;
      })().catch(() => { p.error = true; });
    } } } };
  const original = { factory: ble.client, connect: link.connect, authenticate: client.authenticate };
  const cap = { nativePromise: async (...args) => { nativeCalls.push(args); } };
  const context = vm.createContext({ location, performance, clearTimeout,
    setTimeout: (callback, ms) => setTimeout(callback, Math.min(ms, 5)),
    document: { querySelector: selector => selector },
    ng: { getComponent: selector => selector === 'app-root' ? root : p },
    Capacitor: cap });
  const run = options => {
    context.input = { mode: 'open', logicalDeviceId: 'logical-id', ownerId: 'owner',
      expectedTransport: 'ble', measureBlePhases: true, ...options };
    return vm.runInContext(`(${firstConnectAppGate.toString()})(input)`, context);
  };
  return { run, calls, nativeCalls, original, ble, link, client, cap, p, ui };
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

test('explicit BLE experiment only hints after GATT; cached account still requires admission', async () => {
  const f = fixture();
  const result = await f.run({ useCachedAccountForBle: true, testConnectionPriority: 1 });
  assert.deepEqual(f.calls, ['gatt']);
  assert.deepEqual(JSON.parse(JSON.stringify(f.nativeCalls)), [
    ['BluetoothLe', 'requestConnectionPriority', { deviceId: 'native-address', connectionPriority: 1 }],
    ['BluetoothLe', 'requestConnectionPriority', { deviceId: 'native-address', connectionPriority: 0 }],
  ]);
  const names = result.events.map(event => event.name + ':' + event.state);
  assert(names.indexOf('gatt:resolved') < names.indexOf('priority-request:start'));
  assert(names.indexOf('priority-request:resolved') < names.indexOf('authentication:start'));
  assert(names.indexOf('authentication:resolved') < names.indexOf('priority-balanced:start'));
  await f.run({ mode: 'close' });
  f.ble.hasActiveCredential = async () => false;
  await assert.rejects(f.run({ useCachedAccountForBle: true }), /credential/);
});

test('rejects wrong owner, unsupported priority and non-BLE shortcuts before navigation', async () => {
  const f = fixture();
  for (const options of [
    { ownerId: 'other-owner' }, { testConnectionPriority: 0 },
    { testConnectionPriority: 1, measureBlePhases: false },
    { testConnectionPriority: 1, expectedTransport: 'cloud' },
    { useCachedAccountForBle: true, expectedTransport: 'cloud' },
    { useCachedAccountForBle: true, measureBlePhases: false },
    { testPriorityRejection: true }, { testPriorityRejection: 'yes', testConnectionPriority: 1 },
    { cancelAtPhase: 'authentication' }, { mode: 'cancel', cancelAtPhase: 'unknown' },
    { mode: 'cancel', cancelAtPhase: 'gatt', expectedTransport: 'cloud' },
  ]) await assert.rejects(f.run(options));
  assert.equal(f.calls.length, 0);
  assert.equal(f.nativeCalls.length, 0);
});

test('real native hint rejection cannot reject authentication or once-only Property/Action', async () => {
  const f = fixture();
  f.cap.nativePromise = async () => { throw Error('private native rejection'); };
  const ready = await f.run({ testConnectionPriority: 1 });
  assert.equal(ready.ready, true);
  assert(ready.events.some(e => e.name === 'priority-request' && e.state === 'rejected'));
  assert(ready.events.some(e => e.name === 'priority-balanced' && e.state === 'rejected'));
  assert(!JSON.stringify(ready).includes('private native rejection'));
  const commands = await f.run({ mode: 'commands' });
  assert.equal(commands.commandsFailed, false);
  assert.equal(f.p.snapshot.endpoints[1].value, 1);
  await assert.rejects(f.run({ mode: 'commands' }), /once-only/);
  await f.run({ mode: 'close' });
});

test('explicit unsupported-hint injection is labelled and never calls the native hint API', async () => {
  const f = fixture();
  const ready = await f.run({ testConnectionPriority: 1, testPriorityRejection: true });
  assert.equal(ready.testPriorityRejection, true);
  assert.equal(ready.ready, true);
  assert.equal(f.nativeCalls.length, 0);
  assert.equal((await f.run({ mode: 'commands' })).commandsFailed, false);
  await f.run({ mode: 'close' });
});

test('phase cancellation is observed before exit and late authentication cannot leave wrappers installed', async () => {
  const f = fixture();
  let finish;
  const session = await f.original.authenticate();
  f.client.authenticate = () => new Promise(resolve => { finish = resolve; });
  const originalAuthenticate = f.client.authenticate;
  const cancelled = await f.run({ mode: 'cancel', cancelAtPhase: 'authentication', testConnectionPriority: 1 });
  assert.equal(cancelled.phaseReached, true);
  assert.equal(cancelled.route, '/home/device');
  assert.equal(cancelled.ready, false);
  assert(!cancelled.events.some(e => e.name === 'authentication' && e.state === 'resolved'));
  assert.equal(f.client.authenticate, originalAuthenticate);
  assert.equal(f.link.connect, f.original.connect);
  finish(session); await new Promise(resolve => setImmediate(resolve));
  assert.equal(f.client.authenticate, originalAuthenticate);
  assert.equal(f.nativeCalls.length, 1); // Disconnect, never a late BALANCED request to a successor.
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

test('missing optional native hint method does not change readiness', async () => {
  const f = fixture();
  delete f.cap.nativePromise;
  const result = await f.run({ testConnectionPriority: 1 });
  assert.equal(result.ready, true);
  assert.equal((await f.run({ mode: 'commands' })).commandsFailed, false);
  await f.run({ mode: 'close' });
});

test('late GATT completion after phase cancellation cannot request HIGH', async () => {
  const f = fixture();
  let finish;
  f.link.connect = () => new Promise(resolve => { finish = resolve; });
  const originalConnect = f.link.connect;
  const result = await f.run({ mode: 'cancel', cancelAtPhase: 'gatt', testConnectionPriority: 1 });
  assert.equal(result.phaseReached, true);
  assert.equal(result.ready, false);
  finish(); await new Promise(resolve => setImmediate(resolve));
  assert.equal(f.nativeCalls.length, 0);
  assert.equal(f.link.connect, originalConnect);
});
