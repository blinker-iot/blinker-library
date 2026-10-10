import { afterEach, test } from 'node:test';
import assert from 'node:assert/strict';
import { appPageDemandAction, runPageDemandScenario } from './edge_gateway_page_demand_gate.mjs';

afterEach(() => { delete globalThis.ng; delete globalThis.document; });
function fixture() {
  const input = { child: 'ble_child', action: 'status' }, calls = [];
  const root = { navCtrl: { navigateRoot: async path => calls.push(path) }, deviceService: { deviceUi: {
    data: { getDevice: () => ({ deviceType: 'ble', cloudEnabled: true }) }, connect: async id => calls.push(id),
  } } };
  const page = { device: { deviceName: input.child }, viewActive: true, pageVisible: true, appActive: true,
    canControl: true, synchronized: true, connection: {}, direct: false, snapshot: { stateFresh: true, endpoints: [
      { key: 'buttonPresses', value: 3 }, { key: 'button', role: 'action', valueType: 'null' },
    ] }, execute: async () => { calls.push('button'); page.snapshot.endpoints[0].value++; }, fieldError: () => '' };
  globalThis.document = { querySelector: () => root, querySelectorAll: () => [page] };
  globalThis.ng = { getComponent: element => element };
  return { input, root, page, calls };
}
test('normal page navigation/read use production paths, not protocol owner internals', async () => {
  const f = fixture();
  for (const action of ['enter', 'leave', 'read']) await appPageDemandAction({ ...f.input, action });
  assert.deepEqual(f.calls, ['/device/ble_child', '/home/device', 'ble_child']);
});
test('metadata status is passive and excludes account credentials', async () => {
  const f = fixture(), result = await appPageDemandAction(f.input);
  assert.equal(result.counter, 3); assert.equal(result.canControl, true); assert.deepEqual(f.calls, []);
  assert.deepEqual(Object.keys(result).sort(), ['appActive','canControl','connectionScope','counter','direct','error','fresh','synchronized','visible'].sort());
});
test('exactly one Action is issued via the actual page handler', async () => {
  const f = fixture(), result = await appPageDemandAction({ ...f.input, action: 'button' });
  assert.equal(result.before, 3); assert.equal(result.counter, 4); assert.deepEqual(f.calls, ['button']);
});
test('hidden/wrong/not-ready pages never submit an Action', async () => {
  const f = fixture();
  f.page.viewActive = false;
  await assert.rejects(appPageDemandAction({ ...f.input, action: 'button' }), /GATE_EXACT_VISIBLE_PAGE/);
  f.page.viewActive = true; f.page.canControl = false;
  await assert.rejects(appPageDemandAction({ ...f.input, action: 'button' }), /GATE_NOT_CONTROLLABLE/);
  assert.deepEqual(f.calls, []);
});

test('session replacement retires the real page scope before permitting a new login', async () => {
  const f = fixture(), controller = new AbortController();
  let finish;
  f.page.connection = { signal: controller.signal, closed: new Promise(resolve => { finish = resolve; }) };
  f.root.deviceService.deviceUi.deviceV2 = {
    stop: async () => { controller.abort(); f.page.canControl = false; finish(); f.calls.push('stop'); },
    start: async () => f.calls.push('start'),
  };
  const state = await appPageDemandAction({ ...f.input, action: 'end-session' });
  assert(state.retired && !state.canControl);
  await appPageDemandAction({ ...f.input, action: 'start-session' });
  assert.deepEqual(f.calls, ['stop', 'start']);
});

function scenarioFixture(failAction = false) {
  const calls = [], marks = [], events = [];
  let owners = 1, counter = 3;
  return { calls, marks, io: {
    app: async action => {
      calls.push(action);
      if (action === 'status') return { canControl: true, fresh: true, direct: false, counter };
      if (action === 'scope-status') return { scopes: owners, active: true };
      if (action === 'enter' || action === 'start-session') owners = 1;
      if (action === 'leave' || action === 'end-session') {
        owners = 0;
        events.push({ phase: 'down', execution: true, operation: 2, leaseMillis: 0 });
      }
      if (action === 'end-session') return { retired: true, canControl: false };
      if (action === 'button') {
        ++counter;
        if (failAction) throw Error('UNKNOWN_RESULT');
        events.push({ phase: 'down', kind: 0x12, digest: 'exact-command' },
          { phase: 'down', kind: 0x12, digest: 'exact-command' }, // legal exact-frame retry
          { phase: 'accepted', kind: 0x14 }, { phase: 'accepted', kind: 0x11 });
        return { counter, commandError: false };
      }
      return {};
    },
    broker: async () => ({ sameMqtt: true, ready: true, closed: false, session: 's', initialSession: 's',
      demandOwners: owners, buttonPresses: counter, events }),
    background: async () => { calls.push('background'); owners = 0; },
    foreground: async () => { calls.push('foreground'); owners = 1; },
    serialWanted: async wanted => { assert.equal(owners, wanted); },
    mark: (phase, value) => marks.push({ phase, ...value }),
  } };
}

test('scenario covers real lifecycle, session replacement and read with exactly one Action', async () => {
  const f = scenarioFixture();
  await runPageDemandScenario(f.io);
  assert.equal(f.calls.filter(v => v === 'button').length, 1);
  assert.deepEqual(f.calls.filter(v => ['background','foreground','end-session','start-session','read'].includes(v)),
    ['background','foreground','end-session','start-session','read']);
  assert.equal(f.marks.at(-1).phase, 'passed');
});

test('unknown Action stops the gate without reissuing or masking it with later steps', async () => {
  const f = scenarioFixture(true);
  await assert.rejects(runPageDemandScenario(f.io), /UNKNOWN_RESULT/);
  assert.equal(f.calls.filter(v => v === 'button').length, 1);
  assert(!f.calls.includes('background') && !f.calls.includes('read'));
});

test('route replacement is not mistaken for page demand success', async () => {
  const f = scenarioFixture();
  f.io.broker = async () => ({ sameMqtt: false });
  await assert.rejects(runPageDemandScenario(f.io), /GATE_ROUTE_CHANGED/);
  assert(!f.calls.includes('button'));
});
