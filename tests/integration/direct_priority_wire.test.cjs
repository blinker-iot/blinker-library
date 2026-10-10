'use strict';
// Cross-repository wire gate using the real App codecs and compiled Broker.
// Northbound only: no copied owner algorithm, MCU, or simulated radio receipt.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const { createRequire } = require('node:module');
const workspace = path.resolve(__dirname, '../../..');
const service = process.env.BLINKER_SERVICE_ROOT || path.join(workspace, 'aily-blinker-services');
const app = process.env.BLINKER_APP_ROOT || path.join(workspace, 'blinker-app');
const serviceRequire = createRequire(path.join(service, 'package.json'));
const ts = serviceRequire('typescript');
const modules = new Map();
function load(file) {
  if (modules.has(file)) return modules.get(file).exports;
  const module = { exports: {} }; modules.set(file, module);
  const js = ts.transpileModule(fs.readFileSync(file, 'utf8'), { compilerOptions: {
    target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS,
  }, fileName: file }).outputText;
  vm.runInNewContext(js, { module, exports: module.exports, Uint8Array, ArrayBuffer, DataView,
    TextEncoder, TextDecoder, Buffer, BigInt,
    require: name => name.startsWith('.') ? load(path.resolve(path.dirname(file), `${name}.ts`)) : serviceRequire(name)
  }, { filename: file });
  return module.exports;
}
const appCodec = load(path.join(app, 'src/app/core/protocol/device-v2/codec.ts'));
const appTypes = load(path.join(app, 'src/app/core/protocol/device-v2/types.ts'));
const brokerCodec = serviceRequire(path.join(service, 'services/broker/dist/protocol/v2/cbor.js'));
const brokerFrame = serviceRequire(path.join(service, 'services/broker/dist/protocol/v2/frame.js'));
const hex = bytes => Buffer.from(bytes).toString('hex');
const plain = object => JSON.parse(JSON.stringify(object));

test('Direct Ready query/status are exact independent App-Broker envelopes, not MCU History', () => {
  assert.equal(appTypes.Bbp2MessageKind.DirectReadyQuery, 0x28);
  assert.equal(appTypes.Bbp2MessageKind.DirectReadyStatus, 0x29);
  assert.equal(brokerFrame.Bbp2MessageKind.DirectReadyQuery, 0x28);
  assert.equal(brokerFrame.Bbp2MessageKind.DirectReadyStatus, 0x29);
  assert.equal(appCodec.BBP2_FEATURE_DIRECT_READY, 1 << 19);
  assert.equal(brokerCodec.FEATURE_DIRECT_READY, 1 << 19);
  const query = { ownerId: 1, revision: 256 }, golden = '830101190100';
  for (const codec of [appCodec, brokerCodec]) {
    assert.equal(hex(codec.encodeDirectReadyQueryBody(query)), golden);
    assert.deepEqual(plain(codec.decodeDirectReadyQueryBody(Buffer.from(golden, 'hex'))), query);
    for (const [code, status] of ['pending', 'ready', 'stale', 'denied'].entries()) {
      const reply = { ...query, status, remainingMillis: code === 1 ? 30000 : 0 };
      const wire = `8501011901000${code}${code === 1 ? '197530' : '00'}`;
      assert.equal(hex(codec.encodeDirectReadyStatusBody(reply)), wire);
      assert.deepEqual(plain(codec.decodeDirectReadyStatusBody(Buffer.from(wire, 'hex'))), reply);
    }
    for (const wire of ['83000101', '83010001', '83010100', '8301180101', '8301010100',
      '9f010101ff', '83011b000000010000000001']) assert.throws(() => codec.decodeDirectReadyQueryBody(Buffer.from(wire, 'hex')));
    for (const wire of ['850101010100', '850101010001', '850101010400', '8501010101197531', '85010101011801',
      '850101010201', '850101010301', '85010101000000']) assert.throws(() => codec.decodeDirectReadyStatusBody(Buffer.from(wire, 'hex')));
    for (const change of [{ ownerId: 0 }, { revision: 0x100000000 }, { remainingMillis: -1 }, { remainingMillis: 0 },
      { remainingMillis: 0.5 }, { remainingMillis: 30001 }, { status: 'pending', remainingMillis: 1 }]) {
      assert.throws(() => codec.encodeDirectReadyStatusBody({ ...query, status: 'ready', remainingMillis: 1, ...change }));
    }
  }
  const envelope = { peerKind: 0, peerId: Buffer.alloc(16, 1), requestId: Buffer.alloc(16, 2),
    messageKind: 0x28, messageFlags: 0, messageBody: appCodec.encodeDirectReadyQueryBody(query) };
  const route = brokerCodec.decodeRouteBody(Buffer.from(appCodec.encodeRouteBody(envelope)));
  const response = appCodec.decodeDeliveryBody(brokerCodec.encodeDeliveryBody({ ...route,
    messageKind: 0x29, messageFlags: 2, messageBody: brokerCodec.encodeDirectReadyStatusBody({ ...query, status: 'ready', remainingMillis: 4000 }) }));
  assert.equal(response.messageKind, 0x29); assert.equal(response.messageFlags, 2);
  assert.equal(hex(response.peerId), hex(envelope.peerId)); assert.equal(hex(response.requestId), hex(envelope.requestId));
  assert.equal(appCodec.decodeDirectReadyStatusBody(response.messageBody).remainingMillis, 4000);
});

test('Direct reservation numbers and feature are identical, independent from cloud and Hub Yielded', () => {
  for (const kind of ['DirectPriority', 'DirectPriorityStatus']) {
    assert.equal(appTypes.Bbp2MessageKind[kind], brokerFrame.Bbp2MessageKind[kind]);
  }
  assert.equal(appTypes.Bbp2MessageKind.DirectPriority, 0x1e);
  assert.equal(appTypes.Bbp2MessageKind.DirectPriorityStatus, 0x1f);
  assert.equal(appCodec.BBP2_FEATURE_DIRECT_PRIORITY, 0x20000);
  assert.equal(appCodec.BBP2_FEATURE_DIRECT_PRIORITY, brokerCodec.FEATURE_DIRECT_PRIORITY);
  assert.equal(brokerCodec.FEATURE_GATEWAY_DIRECT_YIELD, 0x40000);
});

test('App and Broker exact Direct request/status goldens round-trip all operations and outcomes', () => {
  for (const [index, action] of ['acquire', 'renew', 'release'].entries()) {
    const request = { ownerId: 1, revision: 1, action, purpose: 'direct' };
    const golden = `840101010${index}`;
    assert.equal(hex(appCodec.encodeDirectPriorityBody(request)), golden);
    assert.equal(hex(brokerCodec.encodeDirectPriorityBody(request)), golden);
    assert.deepEqual(plain(appCodec.decodeDirectPriorityBody(Buffer.from(golden, 'hex'))), request);
    assert.deepEqual(brokerCodec.decodeDirectPriorityBody(Buffer.from(golden, 'hex')), request);
  }
  for (const [index, status] of ['accepted', 'released', 'stale', 'denied', 'busy', 'capacity'].entries()) {
    const reply = { ownerId: 0xffffffff, revision: 256, status }, golden = `84011affffffff1901000${index}`;
    assert.equal(hex(appCodec.encodeDirectPriorityStatusBody(reply)), golden);
    assert.equal(hex(brokerCodec.encodeDirectPriorityStatusBody(reply)), golden);
    assert.deepEqual(plain(appCodec.decodeDirectPriorityStatusBody(Buffer.from(golden, 'hex'))), reply);
  }
});

test('real Route/Delivery preserve Direct kind, peer, request and frame correlation', () => {
  const request = { ownerId: 1, revision: 1, action: 'acquire', purpose: 'direct' };
  const envelope = { peerKind: 0, peerId: Buffer.alloc(16, 1), requestId: Buffer.alloc(16, 2),
    messageKind: 0x1e, messageFlags: 0, messageBody: appCodec.encodeDirectPriorityBody(request) };
  const encoded = appCodec.encodeFrame({ kind: 9, flags: 1, sequence: 7, body: appCodec.encodeRouteBody(envelope) });
  const parsed = brokerFrame.parseBbp2Frame(Buffer.from(encoded)), route = brokerCodec.decodeRouteBody(parsed.body);
  assert.equal(parsed.sequence, 7); assert.equal(route.messageKind, 0x1e);
  assert.equal(hex(route.peerId), '01'.repeat(16)); assert.equal(hex(route.requestId), '02'.repeat(16));
  assert.deepEqual(brokerCodec.decodeDirectPriorityBody(route.messageBody), request);
  const outbound = brokerFrame.encodeBbp2Frame({ kind: 10, flags: 2, sequence: 8,
    body: brokerCodec.encodeDeliveryBody({ ...route, messageKind: 0x1f, messageFlags: 2,
      messageBody: brokerCodec.encodeDirectPriorityStatusBody({ ownerId: 1, revision: 1, status: 'accepted' }) }) });
  const response = appCodec.decodeDeliveryBody(appCodec.decodeFrame(outbound).body);
  assert.equal(response.messageKind, 0x1f); assert.equal(response.messageFlags, 2);
  assert.equal(hex(response.requestId), hex(route.requestId));
  assert.equal(appCodec.decodeDirectPriorityStatusBody(response.messageBody).status, 'accepted');
});

test('both ends reject noncanonical/oversized/unknown Direct bodies and cloud purpose erasure', () => {
  for (const codec of [appCodec, brokerCodec]) {
    for (const body of ['83010101', '850101010000', '8400010100', '8401000100', '8401010000',
      '8401010103', '840101010000', '840118010100', '9f01010100ff', '84011b00000001000000000100']) {
      assert.throws(() => codec.decodeDirectPriorityBody(Buffer.from(body, 'hex')), body);
    }
    assert.throws(() => codec.decodeDirectPriorityStatusBody(Buffer.from('8401010106', 'hex')));
    assert.throws(() => codec.encodeConnectionDemandBody({ ownerId: 1, revision: 1, action: 'acquire', purpose: 'direct' }));
  }
});
