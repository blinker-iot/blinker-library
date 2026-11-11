'use strict';
// Cross-repository INTERNAL contract gate. Loads the actual two TypeScript
// modules, not copies/fakes of their state machines. No MQTT, hardware or App UI.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const path = require('node:path');
const { createRequire } = require('node:module');
const vm = require('node:vm');
const workspace = path.resolve(__dirname, '../../..');
const service = process.env.BLINKER_SERVICE_ROOT || path.join(workspace, 'aily-blinker-services');
const app = process.env.BLINKER_APP_ROOT || path.join(workspace, 'blinker-app');
const serviceRequire = createRequire(path.join(service, 'package.json'));
const ts = serviceRequire('typescript');
const turn = () => new Promise(setImmediate);
function held() { let resolve; const promise = new Promise(r => { resolve = r; }); return { promise, resolve }; }

function load(file, now) {
  const module = { exports: {} };
  const js = ts.transpileModule(readFileSync(file, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS, strict: true,
  }, fileName: file }).outputText;
  vm.runInNewContext(js, { module, exports: module.exports, require: serviceRequire,
    performance: { now }, AbortController,
    setTimeout: (...args) => setTimeout(...args), clearTimeout: timer => clearTimeout(timer) }, { filename: file });
  return module.exports;
}

function fixture(t) {
  t.mock.timers.enable({ apis: ['setTimeout'] });
  let now = 0;
  const { ConnectionDemandOwners } = load(path.join(service, 'services/broker/src/protocol/v2/connection-demand.ts'), () => now);
  const { DeviceV2DemandOwners } = load(path.join(app, 'src/app/core/protocol/device-v2/connection-demand.ts'), () => now);
  const grant = { tenantId: 'tenant', version: 'grant:1', binding: 'hubSession_1', bindingDeadline: 1000000 };
  const f = { grant, calls: [], changes: [], clients: [] };
  f.authorize = async () => grant;
  f.broker = new ConnectionDemandOwners({ now: () => now, current: s => !s.closed, deadline: s => s.deadline,
    authorize: (...args) => f.authorize(...args), changed: snapshot => f.changes.push(snapshot) });
  f.deliver = (session, request) => f.broker.apply(session, request);
  f.client = () => {
    const session = { deadline: 1000000, closed: false };
    const owners = new DeviceV2DemandOwners(async (request, signal) => {
      f.calls.push({ session, request, signal });
      return f.deliver(session, request, signal);
    }, async request => ({ ownerId: request.ownerId, revision: request.revision, status: 'pending', remainingMillis: 0 }));
    // Reservation-only fixture has no physical Hub. Never fabricate Ready here.
    const client = { session, owners, stop: () => {
      session.closed = true; f.broker.closeSession(session); owners.close();
    } };
    f.clients.push(client); return client;
  };
  f.snapshot = () => f.broker.snapshot('tenant', 'ble_child');
  f.tick = async ms => { now += ms; t.mock.timers.tick(ms); await turn(); };
  t.after(() => { f.clients.forEach(c => c.stop()); f.broker.close(); t.mock.timers.reset(); });
  return f;
}

test('real App and Broker modules aggregate, renew and release two independent owners', async t => {
  const f = fixture(t), c = f.client();
  const a = c.owners.acquire('ble_child'), b = c.owners.acquire('ble_child');
  await Promise.all([a.accepted, b.accepted]); assert.equal(f.snapshot().owners, 2);
  a.release(); await turn(); assert.equal(f.snapshot().owners, 1);
  await f.tick(10000); assert.equal(f.snapshot().deadline, 40000);
  b.release(); await turn(); assert.equal(f.snapshot().owners, 0);
  assert.deepEqual(f.calls.map(c => c.request.action), ['acquire', 'acquire', 'release', 'renew', 'release']);
});

test('release during real Broker authorization rejects the App waiter and cannot revive on late grant', async t => {
  const f = fixture(t), c = f.client(), h = held(); f.authorize = () => h.promise;
  const owner = c.owners.acquire('ble_child'), result = owner.accepted.catch(e => e);
  await turn(); owner.release(); await turn();
  assert.equal((await result).reason, 'cancelled'); assert.equal(f.snapshot().owners, 0);
  h.resolve(f.grant); await turn(); assert.equal(f.snapshot().owners, 0);
  assert.equal(f.changes.some(x => x.owners > 0), false);
});

test('a lost release expires without depending on any later message or creating a wake', async t => {
  const f = fixture(t), c = f.client();
  f.deliver = (session, request) => request.action === 'release'
    ? Promise.reject(new Error('loss')) : f.broker.apply(session, request);
  const owner = c.owners.acquire('ble_child'); await owner.accepted;
  owner.release(); await turn(); assert.equal(f.snapshot().owners, 1);
  await f.tick(30000); assert.equal(f.snapshot().owners, 0);
  await f.tick(60000); assert.equal(f.calls.length, 2);
});

test('retiring an account isolates late work from a fresh session reusing owner number one', async t => {
  const f = fixture(t), old = f.client(), h = held();
  f.authorize = session => session === old.session ? h.promise : Promise.resolve(f.grant);
  const owner = old.owners.acquire('ble_child'), result = owner.accepted.catch(e => e);
  await turn(); old.stop(); assert.equal((await result).reason, 'session-ended');
  const fresh = f.client(), freshOwner = fresh.owners.acquire('ble_child'); await freshOwner.accepted;
  h.resolve(f.grant); await turn();
  assert.equal(f.snapshot().owners, 1);
  assert.equal(f.calls[0].request.ownerId, f.calls[1].request.ownerId);
  assert.notStrictEqual(f.calls[0].session, f.calls[1].session);
});

test('release arriving before its acquire fences the delayed message without tombstone growth', async t => {
  const f = fixture(t), c = f.client(), h = held();
  f.deliver = async (session, request) => {
    if (request.action === 'acquire') await h.promise;
    return f.broker.apply(session, request);
  };
  const owner = c.owners.acquire('ble_child'), result = owner.accepted.catch(e => e);
  await turn(); owner.release(); await turn();
  assert.equal((await result).reason, 'cancelled');
  h.resolve(); await turn(); assert.equal(f.snapshot().owners, 0);
  assert.equal(f.changes.some(x => x.owners > 0), false);
});

test('all renew/release loss stops App renewal and expires the original Broker lease', async t => {
  const f = fixture(t), c = f.client(), h = held();
  f.deliver = (session, request) => request.action === 'acquire'
    ? f.broker.apply(session, request) : h.promise;
  const owner = c.owners.acquire('ble_child'); await owner.accepted;
  await f.tick(10000); await f.tick(5000); assert.equal(await owner.closed, 'timeout');
  assert.equal(f.snapshot().owners, 1);
  await f.tick(15000); assert.equal(f.snapshot().owners, 0);
  h.resolve({ ownerId: 1, revision: 2, status: 'accepted' }); await turn();
  await f.tick(60000); assert.equal(f.calls.length, 3);
});

test('ACL invalidation retires ownership and a later App renewal does not recreate it', async t => {
  const f = fixture(t), c = f.client(); const owner = c.owners.acquire('ble_child'); await owner.accepted;
  f.broker.invalidate(c.session, 'ble_child'); assert.equal(f.snapshot().owners, 0);
  await f.tick(10000); assert.equal(await owner.closed, 'rejected');
  assert.equal(f.snapshot().owners, 0);
});

test('Direct and cloud share lifecycle machinery without mixing aggregation or cancellation', async t => {
  const f = fixture(t), c = f.client();
  const cloud = c.owners.acquire('ble_child');
  const direct = c.owners.acquire('ble_child', undefined, 'direct');
  await Promise.all([cloud.accepted, direct.accepted]);
  assert.equal(f.snapshot().owners, 1); assert.equal(f.snapshot().directOwners, 1);
  await f.tick(10000);
  direct.release(); await turn();
  assert.equal(f.snapshot().directOwners, 0); assert.equal(f.snapshot().owners, 1);
  assert.deepEqual(f.calls.filter(c => c.request.purpose === 'direct').map(c => c.request.action),
    ['acquire', 'renew', 'release']);
  cloud.release(); await turn(); assert.equal(f.snapshot().owners, 0);
});

test('a second phone gets Busy without a successor queue and cannot cancel the first Direct holder', async t => {
  const f = fixture(t), a = f.client(), b = f.client();
  const holder = a.owners.acquire('ble_child', undefined, 'direct'); await holder.accepted;
  const rejected = b.owners.acquire('ble_child', undefined, 'direct');
  await assert.rejects(rejected.accepted, e => e.reason === 'rejected'); await turn();
  assert.equal(f.snapshot().directOwners, 1); assert.equal(f.snapshot().owners, 0);
  holder.release(); await turn(); assert.equal(f.snapshot().directOwners, 0);
  await f.tick(10000);
  assert.equal(f.snapshot().directOwners, 0); // Busy candidate does not auto-acquire later.
  const next = b.owners.acquire('ble_child', undefined, 'direct'); await next.accepted;
  assert.equal(f.snapshot().directOwners, 1);
});

test('cloud renewals cannot extend a lost Direct release; original priority expires independently', async t => {
  const f = fixture(t), c = f.client();
  f.deliver = (session, request) => request.purpose === 'direct' && request.action === 'release'
    ? Promise.reject(Error('loss')) : f.broker.apply(session, request);
  const cloud = c.owners.acquire('ble_child');
  const direct = c.owners.acquire('ble_child', undefined, 'direct');
  await Promise.all([cloud.accepted, direct.accepted]);
  direct.release(); await turn();
  for (let i = 0; i < 3; ++i) await f.tick(10000);
  assert.equal(f.snapshot().directOwners, 0); assert.equal(f.snapshot().owners, 1);
  assert.equal(f.calls.filter(c => c.request.purpose === 'direct').length, 2);
});

test('Direct cancellation wins over late authorization and releases the reservation for another phone', async t => {
  const f = fixture(t), a = f.client(), b = f.client(), h = held();
  f.authorize = session => session === a.session ? h.promise : Promise.resolve(f.grant);
  const pending = a.owners.acquire('ble_child', undefined, 'direct');
  const accepted = pending.accepted.catch(e => e); await turn();
  pending.release(); await turn(); assert.equal((await accepted).reason, 'cancelled');
  const next = b.owners.acquire('ble_child', undefined, 'direct'); await next.accepted;
  h.resolve(f.grant); await turn();
  assert.equal(f.snapshot().directOwners, 1); assert.equal(f.snapshot().owners, 0);
  a.stop(); assert.equal(f.snapshot().directOwners, 1);
  b.stop(); assert.equal(f.snapshot().directOwners, 0);
});

test('target ACL revocation removes both purposes without touching an unrelated Direct target', async t => {
  const f = fixture(t), c = f.client();
  const cloud = c.owners.acquire('ble_child');
  const direct = c.owners.acquire('ble_child', undefined, 'direct');
  const other = c.owners.acquire('ble_other', undefined, 'direct');
  await Promise.all([cloud.accepted, direct.accepted, other.accepted]);
  f.broker.invalidate(c.session, 'ble_child');
  assert.equal(f.snapshot().owners, 0); assert.equal(f.snapshot().directOwners, 0);
  assert.equal(f.broker.snapshot('tenant', 'ble_other').directOwners, 1);
  await f.tick(10000);
  assert.equal(await cloud.closed, 'rejected'); assert.equal(await direct.closed, 'rejected');
  assert.equal(f.broker.snapshot('tenant', 'ble_other').directOwners, 1);
});

test('new Hub binding retires Direct on renewal instead of rebinding the old App owner', async t => {
  const f = fixture(t), c = f.client();
  const cloud = c.owners.acquire('ble_child');
  const direct = c.owners.acquire('ble_child', undefined, 'direct');
  await Promise.all([cloud.accepted, direct.accepted]);
  f.authorize = async () => ({ ...f.grant, binding: 'hubSession_2' });
  await f.tick(10000);
  assert.equal(await direct.closed, 'rejected');
  assert.equal(f.snapshot().directOwners, 0); assert.equal(f.snapshot().owners, 1);
  assert.equal(f.calls.filter(c => c.request.purpose === 'direct' && c.request.action === 'acquire').length, 1);
});
