import assert from 'node:assert/strict';
import test from 'node:test';
import vm from 'node:vm';
import { webcrypto } from 'node:crypto';
import { runEnrollmentNegativeGate, assertEnrollmentNegativeResult } from './edge_gateway_enrollment_negative_gate.mjs';

const bytes = (size, value) => new Uint8Array(size).fill(value);
const nonce = value => {
  const result = new Uint8Array(12);
  new DataView(result.buffer).setBigUint64(4, BigInt(value), false);
  return result;
};
const packet = (type, body) => {
  const result = new Uint8Array(body.length + 4);
  result.set([0xb3, 0x10 | type]);
  new DataView(result.buffer).setUint16(2, body.length, false);
  result.set(body, 4);
  return result;
};
const response = receipt => Uint8Array.of(0xa4, 0, 2, 1, 4, 2, 2, 3, 0x58, receipt.length, ...receipt);
const disconnected = () => new Error('EDGE_GATEWAY_PERMIT_JOIN_DISCONNECTED');

// A model of the App/link/storage boundary with real AES-GCM, NOT a Noise
// handshake, Service signer, MCU, NVS or transport test. Run the serialized
// action just as CDP does; no imports/closures may leak into the WebView action.
function fixture(fault = 'request-replay', behavior = '') {
  const config = { hub: 'hub_test', instance: '21'.repeat(16), locator: '23'.repeat(8), fault };
  const instance = bytes(16, 0x21), locator = bytes(8, 0x23), intentId = bytes(16, 0x24);
  const sendKey = bytes(32, 0x41), receiveKey = bytes(32, 0x42), receipt = bytes(145, 0x31);
  const observations = { begins: 0, issues: 0, cancels: 0, removes: 0, closes: 0, writes: 0,
    requestCounters: [], ciphertexts: [], loaded: [] };
  let saved, checkpoint = false, window = false, receiveError, request, wireCount = 0, replyCount = 0;
  const incoming = [];
  const crypto = {};
  for (const [name, method] of [['aesGcmEncrypt', 'encrypt'], ['aesGcmDecrypt', 'decrypt']]) {
    crypto[name] = async (key, iv, additionalData, data) => {
      const imported = await webcrypto.subtle.importKey('raw', key, 'AES-GCM', false, [method]);
      return new Uint8Array(await webcrypto.subtle[method]({ name: 'AES-GCM', iv, additionalData }, imported, data));
    };
  }
  const rawEncrypt = crypto.aesGcmEncrypt, rawDecrypt = crypto.aesGcmDecrypt;
  const store = {
    async findPending() { return undefined; },
    async load() {
      if (!saved) return undefined;
      const value = structuredClone(saved);
      observations.loaded.push(value);
      return value;
    },
    async remove() { observations.removes++; saved = undefined; },
  };
  const api = {
    async issue() { observations.issues++; return { intentId, logicalDeviceId: 'ble_test', grant: bytes(65, 1) }; },
    async commit() { throw new Error('unexpected commit'); },
  };
  const link = {
    operationId: bytes(16, 0x25),
    async sendRecord(record) {
      if (record[1] === 0x11) {
        incoming.push(packet(2, await rawEncrypt(receiveKey, nonce(0), bytes(32, 1), bytes(16, 7))));
        return;
      }
      assert.equal(record[1], 0x13);
      const counter = wireCount++;
      observations.requestCounters.push(counter);
      observations.ciphertexts.push(Buffer.from(record.subarray(4)).toString('hex'));
      let plaintext;
      try { plaintext = await rawDecrypt(sendKey, nonce(counter), new Uint8Array(), record.subarray(4)); }
      catch { receiveError = disconnected(); return; }
      if (!request) {
        if (fault === 'signature') { receiveError = disconnected(); return; }
        request = plaintext.slice();
        observations.writes++;
      } else if (!Buffer.from(request).equals(Buffer.from(plaintext))) {
        if (behavior !== 'accept-changed') {
          if (behavior.startsWith('terminal-')) throw Object.assign(new Error('private HTTP response'), {
            code: 'EDGE_GATEWAY_PERMIT_JOIN_RELAY_NOT_READY',
          });
          receiveError = behavior === 'network-error' ? new Error('private HTTP response must not escape')
            : behavior === 'coded-disconnect'
              ? Object.assign(new Error('private HTTP response must not escape'), { code: 'EDGE_GATEWAY_PERMIT_JOIN_DISCONNECTED' })
              : disconnected();
          return;
        }
      }
      const reply = response(receipt);
      if ((counter === 0 && behavior === 'wrong-first') || (counter === 1 && behavior === 'wrong-replay')) {
        reply[reply.length - 1] ^= 1;
      }
      const encrypted = await rawEncrypt(receiveKey, nonce(replyCount++), new Uint8Array(), reply);
      if (counter === 1 && behavior === 'bad-tag') encrypted[encrypted.length - 1] ^= 1;
      incoming.push(packet(3, encrypted));
    },
    async receiveRecord() {
      if (receiveError) throw receiveError;
      assert(incoming.length, 'model has a response');
      return incoming.shift();
    },
    async waitForMode() { throw new Error('unexpected Method 2'); },
  };
  const client = { crypto, api, async cancelPending() {} };
  const ble = {
    accountLinks: new Set(), accountSessions: new Set(),
    accountContext: () => 'test_account', credentialStore: () => store,
    client: () => client,
    accountBoundApi: () => ({ async cancel(id) {
      observations.cancels++;
      return { intentId: id, logicalDeviceId: behavior === 'cancel-mismatch' ? 'wrong_child' : saved.logicalDeviceId,
        deviceInstanceId: saved.deviceInstanceId, accessEpoch: saved.accessEpoch, controllerId: saved.controllerId };
    } }),
  };
  const target = { profile: { modeLocator: locator } };
  const workflow = {
    candidates: [target],
    async enrollAndAttach() {
      const current = ble.client(link, 'test_account');
      await current.cancelPending(behavior === 'wrong-instance' ? bytes(16, 0x99) : instance);
      await link.sendRecord(packet(1, bytes(32, 1)));
      const hello = await link.receiveRecord();
      await current.crypto.aesGcmDecrypt(receiveKey, nonce(0), bytes(32, 1), hello.subarray(4));
      const intent = await current.api.issue({ deviceInstanceId: instance });
      saved = { state: 'pending', logicalDeviceId: intent.logicalDeviceId, intentId: intent.intentId.slice(),
        deviceInstanceId: instance.slice(), accessEpoch: 7, controllerId: bytes(16, 0x53),
        controllerSecret: bytes(32, 0x51), presenceKeys: [{ key: bytes(16, 0x52) }], receipt: receipt.slice() };
      checkpoint = true;
      const enrollment = Uint8Array.of(0xa6, 0, 2, 1, 3, 2, 2, 3, 0x58, intent.grant.length,
        ...intent.grant, 4, 0x58, 32, ...saved.controllerSecret, 5, 0x50, ...saved.presenceKeys[0].key);
      const encoded = await current.crypto.aesGcmEncrypt(sendKey, nonce(behavior === 'bad-counter' ? 1 : 0),
        new Uint8Array(), enrollment);
      await link.sendRecord(packet(3, encoded));
      const received = await link.receiveRecord();
      const value = await current.crypto.aesGcmDecrypt(receiveKey, nonce(0), new Uint8Array(), received.subarray(4));
      if (!Buffer.from(value).equals(Buffer.from(response(receipt)))) throw new Error('BLE_DIRECT_RECEIPT_MISMATCH');
      await link.waitForMode();
    },
    async cancel() {
      observations.closes++;
      ble.accountLinks.clear();
      if (behavior === 'cancel-failure') throw new Error('sensitive cleanup detail');
      window = false;
    },
  };
  const gateway = {
    ble,
    accountPermitJoinApi: () => ({ async get() {
      if (behavior === 'terminal-read-fails') throw new Error('private HTTP response');
      return { operationId: behavior === 'terminal-wrong-operation' ? bytes(16, 9) : link.operationId,
        edgeHubLogicalDeviceId: behavior === 'terminal-wrong-hub' ? 'wrong_hub' : config.hub,
        adapterId: behavior === 'terminal-wrong-adapter' ? 2 : 1,
        state: behavior === 'terminal-ready' ? 'ready' : behavior === 'terminal-expired' ? 'expired' : 'rejected' };
    } }),
    async recoveries() { return checkpoint ? [{ logicalDeviceId: 'ble_test', stage: 'finish_enrollment' }] : []; },
    accountPermitJoinCheckpoints: () => ({ async list() { return window ? [{}] : []; } }),
    accountEnrollmentCheckpoints: () => ({ async remove() { checkpoint = false; } }),
    async beginEnrollment() { observations.begins++; window = true; ble.accountLinks.add(link); return workflow; },
  };
  const page = { available: true, busy: false, edgeHubLogicalDeviceId: config.hub, gateway };
  const original = { client: ble.client, issue: api.issue, commit: api.commit,
    cancelPending: client.cancelPending, send: link.sendRecord, receive: link.receiveRecord,
    wait: link.waitForMode, encrypt: crypto.aesGcmEncrypt, decrypt: crypto.aesGcmDecrypt };
  return { config, observations, page, gateway, store, workflow, link, client,
    get saved() { return saved; },
    async run() {
      return vm.runInNewContext(`(${runEnrollmentNegativeGate.toString()})(input)`, {
        input: config, Uint8Array, DataView, BigInt,
        ng: { getComponent: () => page }, document: { querySelector: () => ({}) },
      });
    },
    restored() {
      assert.deepEqual({ client: ble.client, issue: api.issue, commit: api.commit,
        cancelPending: client.cancelPending, send: link.sendRecord, receive: link.receiveRecord,
        wait: link.waitForMode, encrypt: crypto.aesGcmEncrypt, decrypt: crypto.aesGcmDecrypt }, original);
      for (const value of observations.loaded) {
        assert(value.controllerSecret.every(byte => byte === 0));
        assert(value.receipt.every(byte => byte === 0));
        for (const key of value.presenceKeys) assert(key.key.every(byte => byte === 0));
      }
    },
  };
}

for (const fault of ['signature', 'request-tag', 'receipt-proof', 'request-replay']) {
  test(`${fault}: exact acceptance assertions, cleanup and hook restoration`, async () => {
    const f = fixture(fault);
    const result = await f.run();
    assertEnrollmentNegativeResult(result, f.config);
    f.restored();
    if (fault === 'request-replay') {
      assert.deepEqual(f.observations.requestCounters, [0, 1, 2]);
      assert.equal(new Set(f.observations.ciphertexts).size, 3);
      assert.equal(f.observations.writes, 1); // model only; not MCU NVS evidence
      assert.equal(f.observations.cancels, 0);
      assert.equal(f.observations.removes, 0);
      assert(f.saved.controllerSecret.every(value => value === 0x51));
      assert(f.saved.receipt.every(value => value === 0x31));
    }
  });
}

for (const behavior of ['wrong-first', 'wrong-replay', 'bad-tag', 'accept-changed', 'network-error', 'bad-counter', 'cancel-failure']) {
  test(`replay gate fails closed on ${behavior}, retains recovery and restores hooks`, async () => {
    const f = fixture('request-replay', behavior);
    const result = await f.run();
    assert.throws(() => assertEnrollmentNegativeResult(result, f.config));
    assert.equal(f.observations.cancels, 0);
    assert.equal(f.observations.removes, 0);
    assert.equal(result.pending, true);
    assert.equal(result.enrollmentCheckpoint, true);
    assert(!JSON.stringify(result).includes('private HTTP'));
    assert(!JSON.stringify(result).includes('sensitive cleanup'));
    if (['wrong-first', 'bad-counter'].includes(behavior)) assert.equal(result.injected, false);
    if (behavior === 'network-error') assert.equal(result.changedRequestRejected, false);
    f.restored();
  });
}

test('typed HTTP rejection uses only its public code, never arbitrary message text', async () => {
  const f = fixture('request-replay', 'coded-disconnect');
  const result = await f.run();
  assertEnrollmentNegativeResult(result, f.config);
  assert(!JSON.stringify(result).includes('private HTTP'));
  f.restored();
});

for (const behavior of ['terminal-rejected', 'terminal-ready', 'terminal-expired', 'terminal-wrong-operation',
  'terminal-wrong-hub', 'terminal-wrong-adapter', 'terminal-read-fails']) {
  test(`${behavior}: NOT_READY requires an exact authoritative rejection before Close`, async () => {
    const f = fixture('request-replay', behavior);
    const result = await f.run();
    if (behavior === 'terminal-rejected') {
      assertEnrollmentNegativeResult(result, f.config);
      assert.equal(result.changedWriteCompleted, false); // terminal result overtook the final ACK
    } else assert.throws(() => assertEnrollmentNegativeResult(result, f.config));
    assert.equal(f.observations.cancels, 0);
    assert.equal(f.observations.removes, 0);
    assert.equal(result.enrollmentCheckpoint, true);
    f.restored();
  });
}

for (const precondition of ['busy', 'links', 'recovery', 'window', 'pending']) {
  test(`existing ${precondition} fails before starting a permit window`, async () => {
    const f = fixture();
    if (precondition === 'busy') f.page.busy = true;
    if (precondition === 'links') f.gateway.ble.accountLinks.add({});
    if (precondition === 'recovery') f.gateway.recoveries = async () => [{}];
    if (precondition === 'window') f.gateway.accountPermitJoinCheckpoints = () => ({ list: async () => [{}] });
    if (precondition === 'pending') f.store.findPending = async () => ({ controllerSecret: bytes(32, 1), presenceKeys: [] });
    await assert.rejects(f.run());
    assert.equal(f.observations.begins, 0);
    assert.equal(f.observations.issues, 0);
    f.restored();
  });
}

test('signature rejection plus failed Close retains pending recovery', async () => {
  const f = fixture('signature', 'cancel-failure');
  const result = await f.run();
  assert.throws(() => assertEnrollmentNegativeResult(result, f.config));
  assert.equal(result.cleanupError, 'GATE_CANCEL_FAILED');
  assert.equal(result.pending, true);
  assert.equal(result.enrollmentCheckpoint, true);
  assert.equal(f.observations.cancels, 0);
  assert.equal(f.observations.removes, 0);
  f.restored();
});

test('mismatched intent cancellation never deletes credential/checkpoint', async () => {
  const f = fixture('signature', 'cancel-mismatch');
  await assert.rejects(f.run(), /GATE_CANCEL_IDENTITY/);
  assert.equal(f.observations.removes, 0);
  assert(f.saved);
  assert.equal((await f.gateway.recoveries()).length, 1);
  f.restored();
});

for (const selection of ['wrong-instance', 'ambiguous-locator']) {
  test(`${selection} cannot issue an enrollment intent`, async () => {
    const f = fixture('request-replay', selection);
    if (selection === 'ambiguous-locator') f.workflow.candidates.push(f.workflow.candidates[0]);
    const result = await f.run();
    assert.equal(result.issued, 0);
    assert.equal(f.observations.issues, 0);
    assert.equal(f.observations.closes, 1);
    assert.equal(result.permitCheckpoints, 0);
    f.restored();
  });
}
