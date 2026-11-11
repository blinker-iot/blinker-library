import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Runs the installed production App coordinator, real WebCrypto and the Hub's
// GATT relay. Hooks exist only in this debug WebView transaction, never src/.
export async function runEnrollmentNegativeGate(input) {
  const require = (condition, code) => { if (!condition) throw new Error(code); };
  const same = (left, right) => left.length === right.length
    && left.every((byte, index) => byte === right[index]);
  const safeError = error => {
    for (const value of [error?.code, error?.error?.code, error?.message]) {
      if (typeof value === 'string' && value.length <= 120
        && /^(BLE_DIRECT|EDGE_GATEWAY|DEVICE_V2|GATE)_[A-Z0-9_]+$/.test(value)) return value;
    }
    return 'UNEXPECTED_FAILURE';
  };
  const clearCredential = value => {
    value?.controllerSecret.fill(0);
    value?.receipt?.fill(0);
    for (const key of value?.presenceKeys ?? []) key.key.fill(0);
  };
  const startedAt = Date.now();
  const hex = value => [...value].map(byte => byte.toString(16).padStart(2, '0')).join('');
  const page = globalThis.ng?.getComponent(document.querySelector('app-edge-gateway-enrollment'));
  if (!page?.available || page.busy || page.workflow
    || page.edgeHubLogicalDeviceId !== input.hub) throw new Error('gate precondition');
  const gateway = page.gateway, ble = gateway.ble;
  require(ble.accountLinks.size === 0 && ble.accountSessions.size === 0, 'GATE_BUSY');
  require((await gateway.recoveries(input.hub)).length === 0, 'GATE_EXISTING_RECOVERY');
  const context = ble.accountContext();
  require((await gateway.accountPermitJoinCheckpoints(context).list()).length === 0,
    'GATE_EXISTING_WINDOW');
  const store = ble.credentialStore(context);
  const instance = Uint8Array.from(input.instance.match(/../g), value => parseInt(value, 16));
  const pending = await store.findPending(instance);
  if (pending) {
    clearCredential(pending);
    throw new Error('existing recovery must finish first');
  }
  const result = { fault: input.fault, issued: 0, sends: 0, receives: 0,
    decrypts: 0, injected: false, method2: 0, commits: 0, pending: false,
    cleanup: false, cleanupError: '', error: '', intentId: '', logicalDeviceId: '', permitId: '',
    exactResponse: false, freshCounters: false, changedRequestRejected: false,
    changedWriteCompleted: false, terminalState: '' };
  const originalClient = ble.client;
  const restore = [];
  let replayMaterial;
  let workflow, intentId;
  ble.client = function(link, account) {
    const client = originalClient.call(this, link, account);
    const hook = (object, key, value) => {
      const descriptor = Object.getOwnPropertyDescriptor(object, key);
      restore.push(() => descriptor ? Object.defineProperty(object, key, descriptor) : delete object[key]);
      object[key] = value;
    };
    const cancelPending = client.cancelPending.bind(client);
    hook(client, 'cancelPending', async instanceId => {
      if (hex(instanceId) !== input.instance) throw new Error('wrong child instance');
      return cancelPending(instanceId);
    });
    const issue = client.api.issue.bind(client.api);
    hook(client.api, 'issue', async request => {
      if (hex(request.deviceInstanceId) !== input.instance || result.issued) {
        throw new Error('wrong child instance');
      }
      const intent = await issue(request);
      intentId = intent.intentId.slice();
      result.issued++;
      result.intentId = hex(intentId);
      result.logicalDeviceId = intent.logicalDeviceId;
      if (input.fault === 'signature') {
        // The frozen canonical grant ends in its 64-byte P-256 signature.
        intent.grant[intent.grant.length - 1] ^= 1;
        result.injected = true;
      }
      return intent;
    });
    hook(client.api, 'commit', async () => { result.commits++; throw new Error('unexpected commit'); });
    hook(link, 'waitForMode', async () => { result.method2++; throw new Error('unexpected Method 2'); });
    const send = link.sendRecord.bind(link), receive = link.receiveRecord.bind(link);
    hook(link, 'sendRecord', async (...values) => {
      result.sends++;
      result.permitId = link.operationId ? hex(link.operationId) : '';
      if (input.fault === 'request-tag' && result.sends === 2) {
        values[0] = values[0].slice();
        values[0][values[0].length - 1] ^= 1;
        result.injected = true;
      }
      return send(...values);
    });
    hook(link, 'receiveRecord', async (...values) => {
      const record = await receive(...values);
      result.receives++;
      return record;
    });
    const encrypt = client.crypto.aesGcmEncrypt.bind(client.crypto);
    if (input.fault === 'request-replay') hook(client.crypto, 'aesGcmEncrypt', async (...values) => {
      if (values[2].length === 0) {
        require(!replayMaterial && result.issued === 1 && result.sends === 1
          && values[0].length === 32 && values[1].length === 12
          && values[1].every(value => value === 0), 'GATE_NOISE_SEND_CONTEXT');
        replayMaterial = { key: values[0].slice(), nonce: values[1].slice(), request: values[3].slice() };
        // Frozen canonical Enrollment requestId=2, not the cloud intentId.
        require(same(replayMaterial.request.subarray(0, 7), Uint8Array.of(0xa6, 0, 2, 1, 3, 2, 2)),
          'GATE_ENROLLMENT_WIRE');
      }
      return encrypt(...values);
    });
    const decrypt = client.crypto.aesGcmDecrypt.bind(client.crypto);
    hook(client.crypto, 'aesGcmDecrypt', async (...values) => {
      const plaintext = await decrypt(...values);
      result.decrypts++;
      if (input.fault === 'request-replay' && values[2].length === 0 && !result.injected) {
        let saved, expected, repeated, changed, receiveNonce;
        try {
          require(replayMaterial && result.sends === 2 && result.receives === 2
            && values[1].length === 12 && values[1].every(value => value === 0), 'GATE_NOISE_RECEIVE_CONTEXT');
          saved = await store.load(result.logicalDeviceId);
          require(saved?.state === 'pending' && same(saved.intentId, intentId)
            && saved.receipt.length >= 24 && saved.receipt.length <= 145, 'GATE_PENDING_RECEIPT');
          // Match the ENTIRE canonical response against the HMAC receipt
          // computed and saved by the production client before its first send.
          expected = new Uint8Array(10 + saved.receipt.length);
          expected.set([0xa4, 0, 2, 1, 4, 2, 2, 3, 0x58, saved.receipt.length]);
          expected.set(saved.receipt, 10);
          require(same(plaintext, expected), 'GATE_FIRST_RECEIPT_MISMATCH');
          const sendRequest = async (request, counter) => {
            new DataView(replayMaterial.nonce.buffer).setBigUint64(4, BigInt(counter), false);
            const ciphertext = await encrypt(replayMaterial.key, replayMaterial.nonce, new Uint8Array(), request);
            const record = new Uint8Array(4 + ciphertext.length);
            record.set([0xb3, 0x13]);
            new DataView(record.buffer).setUint16(2, ciphertext.length, false);
            record.set(ciphertext, 4);
            await link.sendRecord(record);
          };
          result.injected = true;
          await sendRequest(replayMaterial.request, 1);
          const record = await link.receiveRecord();
          require(record.length >= 21 && record[0] === 0xb3 && record[1] === 0x13
            && new DataView(record.buffer, record.byteOffset, record.byteLength).getUint16(2, false)
              === record.length - 4, 'GATE_REPLAY_RECORD');
          receiveNonce = values[1].slice();
          new DataView(receiveNonce.buffer).setBigUint64(4, 1n, false);
          repeated = await decrypt(values[0], receiveNonce, new Uint8Array(), record.subarray(4));
          result.decrypts++;
          result.exactResponse = same(plaintext, repeated);
          require(result.exactResponse, 'GATE_REPLAY_RECEIPT_MISMATCH');
          changed = replayMaterial.request.slice();
          changed[changed.length - 1] ^= 1; // PresenceKey, same canonical requestId.
          result.freshCounters = true; // TX 0/1/2, RX 0/1; no reused ciphertext.
          // The test owns these extra counters and then ABORTS the Noise
          // transaction. Never hand this artificially advanced link to Method 2.
          try {
            await sendRequest(changed, 2);
            result.changedWriteCompleted = true;
            await link.receiveRecord();
          }
          catch (error) {
            const code = safeError(error);
            if (code === 'EDGE_GATEWAY_PERMIT_JOIN_DISCONNECTED') result.changedRequestRejected = true;
            else if (code === 'EDGE_GATEWAY_PERMIT_JOIN_RELAY_NOT_READY') {
              // The terminal window may overtake its last SelectResult/ACK.
              // NOT_READY alone also means expiry/wrong binding: never count
              // it as rejection without this exact authoritative GET, BEFORE Close.
              const window = await gateway.accountPermitJoinApi(context).get(link.operationId);
              require(hex(window.operationId) === result.permitId
                && window.edgeHubLogicalDeviceId === input.hub && window.adapterId === 1,
              'GATE_REJECTION_WINDOW_MISMATCH');
              result.terminalState = window.state;
              result.changedRequestRejected = window.state === 'rejected';
            }
            throw error;
          }
          throw new Error('GATE_CHANGED_REQUEST_ACCEPTED');
        } finally {
          clearCredential(saved);
          for (const value of [plaintext, expected, repeated, changed, receiveNonce]) value?.fill(0);
        }
      }
      if (input.fault === 'receipt-proof' && result.sends === 2
        && values[2].length === 0 && !result.injected) {
        // Mutate only AFTER real AEAD authentication: tests App receipt/HMAC
        // validation, not on-air tag rejection. Child has really installed.
        plaintext[plaintext.length - 1] ^= 1;
        result.injected = true;
      }
      return plaintext;
    });
    return client;
  };
  try {
    workflow = await gateway.beginEnrollment(input.hub);
    const candidates = workflow.candidates.filter(value => hex(value.profile.modeLocator) === input.locator);
    if (candidates.length !== 1) throw new Error('exact candidate not found');
    await workflow.enrollAndAttach(candidates[0], { displayName: 'N3 encrypted enrollment gate' });
  } catch (error) {
    // Never export arbitrary HTTP/bridge errors, which might contain secrets.
    result.error = safeError(error);
  } finally {
    ble.client = originalClient;
    for (const undo of restore.reverse()) undo();
    for (const value of Object.values(replayMaterial ?? {})) value.fill(0);
    try { await workflow?.cancel(); }
    catch { result.cleanupError = 'GATE_CANCEL_FAILED'; }
    if (intentId) {
      const saved = await store.load(result.logicalDeviceId);
      try {
        result.pending = saved?.state === 'pending' && hex(saved.intentId) === hex(intentId);
        if (['signature', 'request-tag'].includes(input.fault)
          && !result.cleanupError
          && result.error === 'EDGE_GATEWAY_PERMIT_JOIN_DISCONNECTED' && result.pending) {
          // Cancel only this rejected intent; receipt-proof keeps the installed
          // credential/checkpoint for normal production Method 2 recovery.
          const cancelled = await ble.accountBoundApi(context).cancel(intentId);
          require(same(cancelled.intentId, saved.intentId)
            && cancelled.logicalDeviceId === saved.logicalDeviceId
            && same(cancelled.deviceInstanceId, saved.deviceInstanceId)
            && cancelled.accessEpoch === saved.accessEpoch
            && same(cancelled.controllerId, saved.controllerId), 'GATE_CANCEL_IDENTITY');
          await store.remove(result.logicalDeviceId);
          await gateway.accountEnrollmentCheckpoints(context).remove(result.logicalDeviceId);
          result.cleanup = true;
        }
      } finally {
        clearCredential(saved);
        intentId.fill(0);
      }
    }
  }
  result.elapsedMs = Date.now() - startedAt;
  result.links = ble.accountLinks.size;
  result.sessions = ble.accountSessions.size;
  result.permitCheckpoints = (await gateway.accountPermitJoinCheckpoints(context).list()).length;
  result.enrollmentCheckpoint = (await gateway.recoveries(input.hub))
    .some(value => value.logicalDeviceId === result.logicalDeviceId);
  return result;
}

export function assertEnrollmentNegativeResult(result, config) {
  const replay = config.fault === 'request-replay';
  const installed = replay || config.fault === 'receipt-proof';
  assert.equal(result.issued, 1);
  assert.equal(result.sends, replay ? 4 : 2);
  assert.equal(result.receives, replay ? 3 : installed ? 2 : 1);
  assert.equal(result.decrypts, replay ? 3 : installed ? 2 : 1);
  assert.equal(result.injected, true);
  assert.equal(result.method2, 0);
  assert.equal(result.commits, 0);
  assert.equal(result.pending, true);
  const terminalRejection = replay && result.error === 'EDGE_GATEWAY_PERMIT_JOIN_RELAY_NOT_READY'
    && result.terminalState === 'rejected';
  if (!terminalRejection) assert.equal(result.error, config.fault === 'receipt-proof'
    ? 'BLE_DIRECT_RECEIPT_MISMATCH' : 'EDGE_GATEWAY_PERMIT_JOIN_DISCONNECTED');
  assert.equal(result.cleanup, !installed);
  assert.equal(result.enrollmentCheckpoint, installed);
  assert.equal(result.cleanupError, '');
  for (const key of ['links', 'sessions', 'permitCheckpoints']) assert.equal(result[key], 0, key);
  if (replay) for (const key of ['exactResponse', 'freshCounters', 'changedRequestRejected']) {
    assert.equal(result[key], true, key);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const args = new Map();
  for (let i = 2; i < process.argv.length; i += 2) {
    const name = process.argv[i], value = process.argv[i + 1];
    assert(['--hub', '--instance', '--locator', '--fault', '--cdp'].includes(name)
      && value && !args.has(name), 'Expected unique --name value arguments');
    args.set(name, value);
  }
  const config = Object.fromEntries([...args].map(([key, value]) => [key.slice(2), value]));
  assert(config.hub && /^[A-Za-z0-9_-]{1,128}$/.test(config.hub), 'Exact Hub ID required');
  assert(/^[a-f0-9]{32}$/.test(config.instance), 'Exact child instance hex required');
  assert(/^[a-f0-9]{16}$/.test(config.locator), 'Explicit provisioning locator hex required');
  assert(['signature', 'request-tag', 'receipt-proof', 'request-replay'].includes(config.fault),
    'Choose signature, request-tag, receipt-proof or request-replay');
  const result = await runGatewayAppGate(runEnrollmentNegativeGate, config);
  console.log(JSON.stringify(result));
  assertEnrollmentNegativeResult(result, config);
}
