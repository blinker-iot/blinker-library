import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

export async function connectInApp(input) {
  const require = (condition, code) => { if (!condition) throw new Error(code); };
  const hex = value => [...value].map(byte => byte.toString(16).padStart(2, '0')).join('');
  const safeError = error => {
    for (const code of [error?.code, error?.error?.code, error?.message]) {
      if (typeof code === 'string' && code.length <= 120
        && /^(BLE_DIRECT|EDGE_GATEWAY|GATE)_[A-Z0-9_]+$/.test(code)) return code;
    }
    if (error?.name === 'BleDirectProtocolError' && Number.isInteger(error.code)
      && error.code >= 0 && error.code <= 0xffff) return `BLE_DIRECT_WIRE_${error.code}`;
    // Only known static messages; never print arbitrary HTTP/crypto errors.
    const known = {
      'BLE Direct request timed out': 'BLE_DIRECT_REQUEST_TIMEOUT',
      'BLE Direct session closed': 'BLE_DIRECT_SESSION_CLOSED',
      'BLE Direct session is closed': 'BLE_DIRECT_SESSION_CLOSED',
      'BLE Direct response does not match the request': 'BLE_DIRECT_RESPONSE_MISMATCH',
      'BLE Direct response sequence does not match the request': 'BLE_DIRECT_RESPONSE_SEQUENCE_MISMATCH',
      'CBOR unsigned value is out of range': 'GATE_CBOR_UNSIGNED_RANGE',
      'CBOR byte string is invalid': 'GATE_CBOR_BYTES_INVALID',
      'unsupported canonical CBOR type': 'GATE_CBOR_TYPE',
      'unsupported canonical CBOR': 'GATE_CBOR_UNSUPPORTED',
      'truncated canonical CBOR': 'GATE_CBOR_TRUNCATED',
      'trailing canonical CBOR data': 'GATE_CBOR_TRAILING',
      'non-canonical CBOR argument': 'GATE_CBOR_NON_CANONICAL',
    };
    return Object.hasOwn(known, error?.message) ? known[error.message] : 'UNEXPECTED_FAILURE';
  };
  const page = globalThis.ng?.getComponent(document.querySelector('app-edge-gateway-enrollment'));
  require(page?.available && !page.busy && !page.workflow && page.edgeHubLogicalDeviceId === input.hub,
    'GATE_PRECONDITION');
  const gateway = page.gateway, ble = gateway.ble;
  require(ble.accountLinks.size === 0 && ble.accountSessions.size === 0, 'GATE_BUSY');
  const instance = await ble.credentialDeviceInstanceId(input.child);
  try { require(hex(instance) === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  const context = gateway.accountContext();
  const checkpoints = gateway.accountPermitJoinCheckpoints(context);
  require((await checkpoints.list()).length === 0, 'GATE_CHECKPOINT_BUSY');
  const results = [];
  for (let i = 0; i < input.count; i++) {
    gateway.assertAccount(context);
    const startedAt = Date.now();
    const result = { iteration: i + 1, startedAt, events: [], relayTrace: [], frameTrace: [], error: '' };
    const event = (phase, detail = {}) => {
      if (result.events.length < 40) result.events.push({ ms: Date.now() - startedAt, phase, ...detail });
    };
    const originalRead = gateway.permitJoin.readRelay;
    const originalSend = gateway.permitJoin.sendRelay;
    const originalClient = ble.client;
    const restoreFrames = [];
    ble.client = function(...args) {
      const client = originalClient.apply(this, args);
      const connect = client.connectWithCredential, authenticate = client.authenticate;
      restoreFrames.push(() => { client.connectWithCredential = connect; client.authenticate = authenticate; });
      client.authenticate = async function(...credentials) {
        event('authenticate-start');
        try {
          const session = await authenticate.apply(this, credentials);
          event('authenticate-ready');
          return session;
        } catch (error) { event('authenticate-failed', { code: safeError(error) }); throw error; }
      };
      client.connectWithCredential = async function(...credentials) {
        const session = await connect.apply(this, credentials);
        const receive = session.receive;
        const send = session.channel.send;
        const metadata = frame => frame && ({ kind: frame.kind, flags: frame.flags, sequence: frame.sequence });
        const trace = (direction, frame) => {
          if (direction === 'receive' && frame.kind === 20 && frame.flags === 2
            && session.pending?.kind !== 20) {
            result.ackDuringOtherResponse = (result.ackDuringOtherResponse || 0) + 1;
          }
          if (result.frameTrace.length === 32) result.frameTrace.shift();
          result.frameTrace.push({ ms: Date.now() - startedAt, direction,
            frame: metadata(frame), pending: metadata(session.pending) });
        };
        session.receive = async function(frame) {
          if (input.fault === 'delayed-ack' && !result.delayedAck && frame.kind === 20
            && session.pending?.kind === 20) {
            // Delay handling, not fabricate a decrypted frame. The existing
            // reliable retry must travel through Broker/Hub to the real child.
            result.delayedAck = true;
            event('application-ack-delayed');
            await new Promise(resolve => setTimeout(resolve, 1300));
          }
          trace('receive', frame);
          return receive.call(this, frame);
        };
        session.channel.send = function(frame) { trace('send', frame); return send.call(this, frame); };
        restoreFrames.push(() => { session.receive = receive; session.channel.send = send; });
        return session;
      };
      return client;
    };
    const traceRelay = (source, frame) => {
      // Test-only bounded prefix reader: never inspect or retain opaque packets.
      if (!frame || frame.length < 23 || frame[0] !== 0x86 || frame[1] !== 1
        || (frame[2] !== 10 && frame[2] !== 11) || frame[3] !== 0x50) return;
      let offset = 20;
      const uint = () => {
        const head = frame[offset++];
        if (head < 24) return head;
        if (head === 24 && offset < frame.length) return frame[offset++];
        if (head === 25 && offset + 1 < frame.length) return frame[offset++] * 256 + frame[offset++];
        return undefined;
      };
      const direction = uint(), sequence = uint();
      if (direction === undefined || sequence === undefined) return;
      const entry = { ms: Date.now() - startedAt, source, kind: frame[2], direction, sequence };
      if (frame[2] === 11) entry.status = uint();
      const previous = result.relayTrace.at(-1);
      if (previous && ['source', 'kind', 'direction', 'sequence', 'status']
        .every(key => previous[key] === entry[key])) return;
      if (result.relayTrace.length === 32) result.relayTrace.shift();
      result.relayTrace.push(entry);
    };
    const traceView = view => {
      traceRelay('view', view.downAck);
      traceRelay('view', view.upstreamBatch);
    };
    gateway.permitJoin.sendRelay = async function(...args) {
      traceRelay('send', args[1]);
      try {
        const view = await originalSend.apply(this, args);
        traceView(view);
        if (input.fault === 'duplicate-ack' && args[1][2] === 11) {
          // Repeat only the public relay ACK on this exact operation. Never
          // replay a Noise/DirectSecure record or alter credential contents.
          result.duplicateAcks = (result.duplicateAcks || 0) + 1;
          const replay = await originalSend.apply(this, args);
          traceView(replay);
          return replay;
        }
        return view;
      } catch (error) { event('send-relay-failed', { code: safeError(error) }); throw error; }
    };
    let lastSelect = '';
    let lastCandidates = '';
    let snapshotEvents = 0;
    gateway.permitJoin.readRelay = async function(...args) {
      const view = await originalRead.apply(this, args);
      traceView(view);
      if (view.candidateSnapshot && hex(view.candidateSnapshot) !== lastCandidates) {
        lastCandidates = hex(view.candidateSnapshot);
        result.lastCandidateSnapshot = { ms: Date.now() - startedAt, encoded: lastCandidates };
        // RSSI/revision may change rapidly. Reserve event space for failure/cleanup.
        if (snapshotEvents++ < 8) event('candidate-snapshot', { encoded: lastCandidates });
      }
      if (view.selectResult && hex(view.selectResult) !== lastSelect) {
        lastSelect = hex(view.selectResult);
        event('select-result', { encoded: lastSelect }); // Public locator/status only.
      }
      return view;
    };
    const link = gateway.permitJoinLink(input.hub, context);
    const originals = new Map();
    // Handshake failures precede the secure session. Keep only fixed public
    // BBP/2 headers (never Auth/Noise bodies or decrypted application values).
    result.recordTrace = [];
    const traceRecord = (direction, record) => {
      const metadata = { ms: Date.now() - startedAt, direction, size: record.length };
      if (record.length >= 10 && record[0] === 0x42 && record[1] === 0x4b && record[2] === 2) {
        Object.assign(metadata, { kind: record[3], flags: record[4], headerSize: record[5],
          sequence: record[6] * 256 + record[7], bodySize: record[8] * 256 + record[9] });
      }
      if (result.recordTrace.length === 32) result.recordTrace.shift();
      result.recordTrace.push(metadata);
    };
    for (const method of ['sendRecord', 'receiveRecord']) {
      const original = link[method];
      originals.set(method, original);
      link[method] = async function(...args) {
        if (method === 'sendRecord') traceRecord('send', args[0]);
        try {
          const value = await original.apply(this, args);
          if (method === 'receiveRecord') traceRecord('receive', value);
          return value;
        } catch (error) {
          event(`${method}-failed`, { code: safeError(error), closing: link.closing === true });
          throw error;
        }
      };
    }
    for (const method of ['waitForMode', 'connect', 'disconnect']) {
      const original = link[method];
      originals.set(method, original);
      link[method] = async function(...args) {
        event(`${method}-start`);
        try {
          const value = await original.apply(this, args);
          if (method === 'waitForMode') event('target', { locator: hex(value.profile.modeLocator) });
          if (method === 'connect') result.operationId = hex(link.operationId);
          event(`${method}-end`);
          return value;
        } catch (error) { event(`${method}-failed`, { code: safeError(error) }); throw error; }
      };
    }
    let connected;
    try {
      connected = await ble.connectUsing(link, input.child);
      event('connect-synchronized'); // connectUsing includes the initial Manifest/State sync.
      await connected.session.synchronize(); // Read Manifest/State only; no mutation.
      event('synchronized');
      if (i === 0 && input.fault === 'selected-acl') {
        const operationId = link.operationId.slice();
        try {
          // Valid fragmentation, deliberately invalid application record on
          // an authenticated ACL: the real child must retire that BLE peer.
          // No access reset, credential mutation, mock GAP event or DB write.
          event('invalid-record-sent');
          try { await link.sendRecord(Uint8Array.of(0)); }
          catch { event('injection-send-ended'); } // ACK may race the ACL loss.
          const deadline = Date.now() + 15_000;
          let window;
          do {
            gateway.assertAccount(context);
            window = await gateway.permitJoin.get(operationId);
            if (window.state !== 'ready') break;
            await new Promise(resolve => setTimeout(resolve, 250));
          } while (Date.now() < deadline);
          result.terminalState = window.state;
          require(window.state === 'rejected', 'GATE_EXPECTED_REJECTED');
          event('selected-acl-rejected');
        } finally { operationId.fill(0); }
      }
    } catch (error) {
      result.error = safeError(error);
      const status = error?.httpStatus ?? error?.status;
      if (Number.isInteger(status) && status >= 0 && status <= 599) result.httpStatus = status;
    } finally {
      try {
        try { await connected?.session.close(); }
        finally { await link.disconnect(); }
      } catch {
        result.error ||= 'GATE_CLEANUP_FAILED';
        event('cleanup-failed');
      } finally {
        for (const [method, original] of originals) link[method] = original;
        gateway.permitJoin.readRelay = originalRead;
        gateway.permitJoin.sendRelay = originalSend;
        ble.client = originalClient;
        for (const restore of restoreFrames) restore();
      }
    }
    result.elapsedMs = Date.now() - startedAt;
    result.links = ble.accountLinks.size;
    result.sessions = ble.accountSessions.size;
    result.checkpoints = (await checkpoints.list()).length;
    if (!result.error) { delete result.relayTrace; delete result.frameTrace; delete result.recordTrace; }
    results.push(result);
    if (result.error || result.links || result.sessions || result.checkpoints) break;
  }
  return results;
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const args = new Map();
  for (let i = 2; i < process.argv.length; i += 2) {
    const key = process.argv[i], value = process.argv[i + 1];
    assert(['--hub', '--child', '--instance', '--count', '--cdp', '--fault'].includes(key)
      && value && !args.has(key), 'Expected unique --name value arguments');
    args.set(key, value);
  }
  const config = Object.fromEntries([...args].map(([key, value]) => [key.slice(2), value]));
  for (const key of ['hub', 'child']) assert(typeof config[key] === 'string'
    && /^[A-Za-z0-9_-]{1,128}$/.test(config[key]), `Exact ${key} required`);
  assert(/^[a-f0-9]{32}$/.test(config.instance), 'Exact child instance required');
  config.count = Number(config.count || 1);
  assert(Number.isInteger(config.count) && config.count >= 1 && config.count <= 10, 'Count must be 1..10');
  assert(!config.fault || config.fault === 'duplicate-ack' || config.fault === 'delayed-ack'
    || (config.fault === 'selected-acl' && config.count >= 2),
    'Fault must be duplicate-ack, delayed-ack, or selected-acl with at least two iterations');
  const result = await runGatewayAppGate(connectInApp, config, config.count * 120_000);
  console.log(JSON.stringify(result));
  assert.equal(result.length, config.count);
  for (const iteration of result) {
    assert.equal(iteration.error, '');
    assert.equal(iteration.links, 0);
    assert.equal(iteration.sessions, 0);
    assert.equal(iteration.checkpoints, 0);
  }
  assert.equal(new Set(result.map(iteration => iteration.operationId)).size, config.count);
  if (config.fault === 'selected-acl') assert.equal(result[0].terminalState, 'rejected');
  if (config.fault === 'duplicate-ack') assert(result.every(iteration => iteration.duplicateAcks > 0));
  if (config.fault === 'delayed-ack') assert(result.every(iteration =>
    iteration.delayedAck && iteration.ackDuringOtherResponse > 0));
}
