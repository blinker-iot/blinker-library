'use strict';
// Real App Session/codecs and Broker MQTT dispatch, joined by a binary channel.
// Software wire gate, NOT a live MQTT/Hub/phone test. No credentials or hardware.
const { test, after } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync, mkdtempSync, existsSync } = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { createRequire } = require('node:module');
const vm = require('node:vm');
const workspace = path.resolve(__dirname, '../../..');
const service = process.env.BLINKER_SERVICE_ROOT || path.join(workspace, 'aily-blinker-services');
const app = process.env.BLINKER_APP_ROOT || path.join(workspace, 'blinker-app');
const serviceRequire = createRequire(path.join(service, 'package.json'));
const ts = serviceRequire('typescript');
const cache = new Map();
function loadApp(file) {
  if (cache.has(file)) return cache.get(file).exports;
  const module = { exports: {} }; cache.set(file, module);
  const js = ts.transpileModule(readFileSync(file, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS, strict: true,
  }, fileName: file }).outputText;
  vm.runInNewContext(js, { module, exports: module.exports,
    require: name => {
      if (!name.startsWith('.')) return serviceRequire(name);
      const source = path.resolve(path.dirname(file), name);
      return loadApp(existsSync(`${source}.ts`) ? `${source}.ts` : path.join(source, 'index.ts'));
    },
    performance, AbortController, Uint8Array, ArrayBuffer, TextEncoder, TextDecoder, DOMException,
    setTimeout, clearTimeout, atob, btoa, crypto: globalThis.crypto }, { filename: file });
  return module.exports;
}
const root = path.join(app, 'src/app/core/protocol/device-v2');
const { DeviceV2Session } = loadApp(path.join(root, 'session.ts'));
const { DeviceUiConnectionScope } = loadApp(path.join(app, 'src/app/core/device-v2/connection-scope.ts'));
const appCodec = loadApp(path.join(root, 'codec.ts'));
const temp = mkdtempSync(path.join(os.tmpdir(), 'blinker-demand-wire-'));
process.env.DB_FILENAME = path.join(temp, 'fixture.db');
const db = serviceRequire('./services/broker/dist/database');
const config = serviceRequire('./services/broker/dist/config').default;
const lane = serviceRequire('./services/broker/dist/protocol/v2/app-lane');
const cbor = serviceRequire('./services/broker/dist/protocol/v2/cbor');
const frame = serviceRequire('./services/broker/dist/protocol/v2/frame');
const target = 'ble_ASNFZ4mrze8BI0VniavN7w';
const tenantId = 'wire-owner';
let serial = 0;
const turn = () => new Promise(setImmediate);
function held() { let resolve; const promise = new Promise(r => { resolve = r; }); return { promise, resolve }; }
after(() => { lane.clearV2AppConnections(); db.closeDatabase(); });

function fixture(t, shared = false) {
  lane.clearV2AppConnections(); db.closeDatabase();
  config.database.filename = path.join(temp, `${++serial}.db`);
  db.initDatabase({ createDefaultDevice: false });
  db.provisionV2Device({ logicalDeviceId: target, tenantId, name: 'wire fixture',
    deviceType: 'ble', credentialVersion: 1, idempotencyKey: 'wire-fixture' });
  const f = { clients: [], packets: [], errors: [],
    grant: { allowed: true, relationship: 'share', targetTenantId: tenantId,
      version: 'grant:1', canRead: true, canCommand: false, commandEndpointKeys: [] } };
  f.authorize = async () => f.grant;
  f.client = () => {
    const client = { id: `wire-app-${f.clients.length}`, connected: true, closed: false };
    const claims = lane.registerV2AppConnection(client, { accountId: 'wire-account',
      tenantId: shared ? 'wire-reader' : tenantId, credentialId: 'test-only', expiresAt: Date.now() + 60000 });
    const messages = new Set(), closes = new Set(); let nextRequest = 0;
    const emit = value => messages.forEach(listener => listener(new Uint8Array(value)));
    const channel = {
      publish: async payload => {
        const packet = Buffer.from(payload); f.packets.push(packet);
        const result = lane.handleV2AppInboundPublish(client, packet,
          () => { throw new Error('Demand must not dispatch Action'); }, input => f.authorize(input));
        result.outbound.forEach(emit);
        // MQTT write completion is independent of async ACL/response completion.
        if (result.pending) void result.pending.then(response => {
          if (lane.getV2AppConnection(client) === claims) emit(response);
        }).catch(error => f.errors.push(error));
      },
      onMessage: listener => { messages.add(listener); return () => messages.delete(listener); },
      onClose: listener => { closes.add(listener); return () => closes.delete(listener); },
      close: async () => { lane.removeV2AppConnection(client); closes.forEach(listener => listener()); },
    };
    const session = new DeviceV2Session(channel, undefined, { requestId: () => new Uint8Array(16).fill(++nextRequest) });
    const result = { session, client, claims }; f.clients.push(result); return result;
  };
  f.snapshot = () => lane.getV2AppConnectionDemand(tenantId, target);
  t.after(async () => { for (const c of f.clients) await c.session.close(); lane.clearV2AppConnections();
    assert.deepEqual(f.errors, []); });
  return f;
}

test('cross-codec goldens cover every status/action and uint32 edges', () => {
  for (const ownerId of [1, 23, 24, 255, 256, 65535, 65536, 0xffffffff]) {
    for (const action of ['acquire', 'renew', 'release']) {
      const request = { ownerId, revision: ownerId, action };
      const encoded = appCodec.encodeConnectionDemandBody(request);
      assert.deepEqual(cbor.decodeConnectionDemandBody(Buffer.from(encoded)), request);
      assert.deepEqual(Buffer.from(encoded), cbor.encodeConnectionDemandBody(request));
    }
    for (const status of ['accepted', 'released', 'stale', 'denied', 'busy', 'capacity']) {
      const reply = { ownerId, revision: ownerId, status };
      const encoded = cbor.encodeConnectionDemandStatusBody(reply);
      assert.deepEqual(JSON.parse(JSON.stringify(appCodec.decodeConnectionDemandStatusBody(new Uint8Array(encoded)))), reply);
      assert.deepEqual(Buffer.from(appCodec.encodeConnectionDemandStatusBody(reply)), encoded);
    }
  }
});

test('actual App Hello/Route/Delivery creates independent owners with no ready side effect', async t => {
  const f = fixture(t), c = f.client(); await c.session.start();
  const a = c.session.acquireConnectionDemand(target), b = c.session.acquireConnectionDemand(target);
  await Promise.all([a.accepted, b.accepted]); assert.equal(f.snapshot().owners, 2);
  assert.equal(c.session.store.snapshot(target).manifestAccepted, false);
  a.release(); await turn(); assert.equal(f.snapshot().owners, 1);
  b.release(); await turn(); assert.equal(f.snapshot().owners, 0);
  const routes = f.packets.map(frame.parseBbp2Frame).filter(p => p.kind === frame.Bbp2MessageKind.Route);
  assert.equal(routes.length, 4);
  assert.ok(routes.every(p => cbor.decodeRouteBody(p.body).messageKind === frame.Bbp2MessageKind.ConnectionDemand));
});

test('real UI scopes retain two wire owners after preparation, then release independently', async t => {
  const f = fixture(t), c = f.client(); await c.session.start();
  const open = () => new DeviceUiConnectionScope(scope => scope.hold(c.session.acquireConnectionDemand(target, scope.signal)));
  const a = open(), b = open(); t.after(() => { a.close(); b.close(); });
  await Promise.all([a.ready, b.ready]); assert.equal(f.snapshot().owners, 2);
  a.close(); await a.closed; await turn(); assert.equal(f.snapshot().owners, 1);
  b.close(); await b.closed; await turn(); assert.equal(f.snapshot().owners, 0);
  assert.equal(c.session.store.snapshot(target).manifestAccepted, false); // Ownership never manufactures readiness.
});

test('read failure and account retirement close real UI scopes across the wire without Action', async t => {
  const f = fixture(t), c = f.client(); await c.session.start();
  const failed = new DeviceUiConnectionScope(async scope => {
    await scope.hold(c.session.acquireConnectionDemand(target, scope.signal));
    throw Error('READ_FAILED');
  });
  await assert.rejects(failed.ready, /READ_FAILED/); await turn(); assert.equal(f.snapshot().owners, 0);
  const visible = new DeviceUiConnectionScope(scope => scope.hold(c.session.acquireConnectionDemand(target, scope.signal)));
  await visible.ready; assert.equal(f.snapshot().owners, 1);
  await c.session.close(); await visible.closed; assert.equal(f.snapshot().owners, 0);
  const replacement = f.client(); await replacement.session.start(); await turn();
  assert.equal(f.snapshot().owners, 0); // Only a new foreground caller may acquire in the replacement Session.
});

test('closing the UI scope during shared authorization cannot resurrect a late accepted owner', async t => {
  const f = fixture(t, true), c = f.client(), h = held(); f.authorize = () => h.promise;
  await c.session.start();
  const scope = new DeviceUiConnectionScope(current => current.hold(c.session.acquireConnectionDemand(target, current.signal)));
  const result = scope.ready.catch(error => error);
  await turn(); scope.close();
  assert.equal((await result).code, 'DEVICE_V2_READY_CANCELLED');
  h.resolve(f.grant); await turn(); assert.equal(f.snapshot().owners, 0);
});

test('cancel crosses real wire before shared ACL finishes; late grant/ACK cannot revive', async t => {
  const f = fixture(t, true), c = f.client(), h = held(); let signal;
  f.authorize = input => { signal = input.signal; return h.promise; };
  await c.session.start(); const controller = new AbortController();
  const owner = c.session.acquireConnectionDemand(target, controller.signal), pending = owner.accepted.catch(e => e);
  await turn(); controller.abort(); await turn();
  assert.equal((await pending).reason, 'cancelled'); assert.equal(signal.aborted, true);
  h.resolve(f.grant); await turn(); assert.equal(f.snapshot().owners, 0);
});

test('real wire duplicate accepted request is stale after release', async t => {
  const f = fixture(t), c = f.client(); await c.session.start();
  const owner = c.session.acquireConnectionDemand(target); await owner.accepted;
  const packet = f.packets.find(p => frame.parseBbp2Frame(p).kind === frame.Bbp2MessageKind.Route);
  owner.release(); await turn();
  const result = lane.handleV2AppInboundPublish(c.client, packet);
  const delivery = cbor.decodeDeliveryBody(frame.parseBbp2Frame(await result.pending).body);
  assert.equal(appCodec.decodeConnectionDemandStatusBody(new Uint8Array(delivery.messageBody)).status, 'stale');
  assert.equal(f.snapshot().owners, 0);
});

test('account session close isolates a new owner one from a late old grant', async t => {
  const f = fixture(t, true), old = f.client(), h = held(); f.authorize = () => h.promise;
  await old.session.start(); const owner = old.session.acquireConnectionDemand(target), result = owner.accepted.catch(e => e);
  await turn(); await old.session.close(); assert.equal((await result).reason, 'session-ended');
  f.authorize = async () => f.grant;
  const next = f.client(); await next.session.start(); const current = next.session.acquireConnectionDemand(target);
  await current.accepted; h.resolve(f.grant); await turn(); assert.equal(f.snapshot().owners, 1);
  current.release(); await turn(); assert.equal(f.snapshot().owners, 0);
});

test('read-only ACL permits ownership; its actual invalidation removes it without a Manifest gate', async t => {
  const f = fixture(t, true), c = f.client(); await c.session.start();
  const owner = c.session.acquireConnectionDemand(target); await owner.accepted;
  assert.equal(c.claims.targetGates.size, 0); assert.equal(f.snapshot().owners, 1);
  lane.invalidateV2AppTargetAuthorization('wire-account', target, 'grant:2');
  assert.equal(f.snapshot().owners, 0); owner.release(); await turn(); assert.equal(f.snapshot().owners, 0);
});
