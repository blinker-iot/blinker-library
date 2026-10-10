import assert from 'node:assert/strict';

// Debug-WebView-only instrumentation of the installed App and real child.
// Requires an existing exact PendingChildInstall checkpoint. Never resets
// access, creates a replacement topology, or exports credentials/receipt bytes.
const args = new Map();
for (let i = 2; i < process.argv.length; i += 2) {
  const key = process.argv[i], value = process.argv[i + 1];
  assert(['--hub', '--child', '--instance', '--operation', '--fault', '--cdp'].includes(key)
    && value && !args.has(key), 'Expected unique --name value arguments');
  args.set(key, value);
}
const config = Object.fromEntries([...args].map(([key, value]) => [key.slice(2), value]));
for (const key of ['hub', 'child']) assert(typeof config[key] === 'string'
  && /^[A-Za-z0-9_-]{1,128}$/.test(config[key]), `Exact ${key} required`);
for (const key of ['instance', 'operation']) assert(/^[a-f0-9]{32}$/.test(config[key]), `Exact ${key} hex required`);
assert(['receipt-proof', 'request-replay'].includes(config.fault), 'Choose receipt-proof or request-replay');

async function runInApp(input) {
  const require = (condition, code) => { if (!condition) throw new Error(code); };
  const hex = value => [...value].map(byte => byte.toString(16).padStart(2, '0')).join('');
  const same = (left, right) => left.length === right.length
    && left.every((byte, index) => byte === right[index]);
  const clear = receipt => {
    for (const value of Object.values(receipt ?? {})) if (value instanceof Uint8Array) value.fill(0);
  };
  const page = globalThis.ng?.getComponent(document.querySelector('app-edge-gateway-enrollment'));
  require(page?.available && !page.busy && !page.workflow
    && page.edgeHubLogicalDeviceId === input.hub, 'GATE_PRECONDITION');
  const gateway = page.gateway, ble = gateway.ble, api = gateway.attachApi;
  require(ble.accountLinks.size === 0 && ble.accountSessions.size === 0, 'GATE_BUSY');
  const operation = Uint8Array.from(input.operation.match(/../g), value => parseInt(value, 16));
  const context = gateway.accountContext();
  const current = await api.get(operation);
  require(current.topology.topologyState === 3
    && current.topology.edgeHubLogicalDeviceId === input.hub
    && current.topology.childLogicalDeviceId === input.child
    && hex(current.topology.childDeviceInstanceId) === input.instance, 'GATE_TOPOLOGY');
  const recoveries = await gateway.recoveries(input.hub);
  const recovery = recoveries.find(value => value.logicalDeviceId === input.child
    && value.id === `attachment:${btoa(String.fromCharCode(...operation)).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '')}`);
  require(recovery?.stage === 'finish_attachment', 'GATE_CHECKPOINT');
  const startedAt = Date.now();
  const result = { fault: input.fault, operation: input.operation, installs: 0,
    sameFrame: false, sameReceipt: false, changedRequestError: -1,
    serverRejection: '', serverStatus: 0, serverUnchanged: false,
    method2: 0, commits: 0, injected: false, error: '', checkpoint: false,
    selectResults: [] };
  const originalControl = ble.createGatewayChildControl;
  const originalConfirm = api.confirmReceipt;
  const originalRead = gateway.permitJoin.readRelay;
  gateway.permitJoin.readRelay = async function(...args) {
    const view = await originalRead.apply(this, args);
    // Existing non-secret SelectResult only; never inspect upstream plaintext.
    if (view.selectResult) {
      const value = hex(view.selectResult);
      if (result.selectResults.length < 8 && !result.selectResults.includes(value)) result.selectResults.push(value);
    }
    return view;
  };
  api.confirmReceipt = async () => { result.commits++; throw new Error('GATE_UNEXPECTED_COMMIT'); };
  ble.createGatewayChildControl = function(enrollment, linkFactory) {
    require(enrollment.logicalDeviceId === input.child, 'GATE_CHILD');
    const control = originalControl.call(this, enrollment, linkFactory);
    const session = enrollment.session, channel = session.channel;
    const originalWithAdmin = control.withAdminControl;
    const originalApply = session.applyControllerMutation;
    control.confirmGatewayCredential = async () => { result.method2++; throw new Error('GATE_UNEXPECTED_METHOD2'); };
    control.withAdminControl = async (...args) => {
      session.applyControllerMutation = async (grant, secret) => {
        const originalCreate = channel.createFrame;
        let request, receipt, repeated;
        let badSecret;
        try {
          if (input.fault === 'request-replay') channel.createFrame = function(kind, flags, body) {
            require(kind === 0x32 && flags === 0, 'GATE_REQUEST_KIND');
            const frame = originalCreate.call(this, kind, flags, body);
            if (!request) request = { ...frame, body: body.slice() };
            else {
              frame.sequence = request.sequence;
              if (result.installs === 1) {
                result.sameFrame = kind === request.kind && flags === request.flags && same(body, request.body);
                require(result.sameFrame, 'GATE_REQUEST_CHANGED');
              } else require(!same(body, request.body), 'GATE_MUTATION_MISSING');
            }
            return frame;
          };
          receipt = await originalApply.call(session, grant, secret);
          result.installs++;
          require(hex(receipt.deviceInstanceId) === input.instance, 'GATE_RECEIPT_ID');
          if (input.fault === 'request-replay') {
            repeated = await originalApply.call(session, grant, secret);
            result.installs++;
            result.sameReceipt = same(receipt.encoded, repeated.encoded);
            require(result.sameReceipt, 'GATE_REPLAY_RECEIPT');
            badSecret = secret.slice();
            badSecret[0] ^= 1;
            try {
              clear(await originalApply.call(session, grant, badSecret));
              throw new Error('GATE_CHANGED_REQUEST_ACCEPTED');
            } catch (error) {
              require(Number.isInteger(error?.code), 'GATE_EXPECT_WIRE_REJECTION');
              result.changedRequestError = error.code;
            }
          }
          // After real DirectSecure decryption and decoding: tests receipt
          // validation, not ciphertext authentication. Child really installed.
          receipt.proof[receipt.proof.length - 1] ^= 1;
          receipt.encoded[receipt.encoded.length - 1] = receipt.proof[receipt.proof.length - 1];
          result.injected = true;
          gateway.assertAccount(context);
          const before = (await api.get(operation)).topology;
          try {
            await originalConfirm.call(api, operation, receipt.encoded);
            throw new Error('GATE_SERVER_ACCEPTED_BAD_RECEIPT');
          } catch (error) {
            const code = error?.code;
            require(code === 'EDGE_GATEWAY_RECEIPT_MISMATCH', 'GATE_SERVER_REJECTION');
            result.serverRejection = code;
            result.serverStatus = error.httpStatus;
          }
          const after = (await api.get(operation)).topology;
          result.serverUnchanged = after.topologyState === 3 && after.topologyVersion === before.topologyVersion
            && after.updatedAt === before.updatedAt && !after.childInstalled && !after.gatewayProven;
          require(result.serverUnchanged, 'GATE_SERVER_CHANGED');
          const output = receipt;
          receipt = undefined; // Production relay owns and clears this value.
          return output;
        } finally {
          channel.createFrame = originalCreate;
          request?.body.fill(0);
          badSecret?.fill(0);
          clear(receipt);
          clear(repeated);
        }
      };
      try { return await originalWithAdmin(...args); }
      finally { session.applyControllerMutation = originalApply; }
    };
    return control;
  };
  try {
    await gateway.resumeRecovery(input.hub, recovery);
  } catch (error) {
    result.error = /^(BLE_DIRECT|EDGE_GATEWAY|GATE)_[A-Z0-9_]+$/.test(error?.message ?? '')
      ? error.message : 'UNEXPECTED_FAILURE';
  } finally {
    ble.createGatewayChildControl = originalControl;
    api.confirmReceipt = originalConfirm;
    gateway.permitJoin.readRelay = originalRead;
    operation.fill(0);
  }
  result.checkpoint = (await gateway.recoveries(input.hub)).some(value => value.id === recovery.id);
  result.links = ble.accountLinks.size;
  result.sessions = ble.accountSessions.size;
  result.elapsedMs = Date.now() - startedAt;
  return result;
}

const pages = await (await fetch(`${config.cdp || 'http://127.0.0.1:9222'}/json/list`, {
  signal: AbortSignal.timeout(5000),
})).json();
const page = pages.find(value => value.type === 'page' && value.url?.includes('/gateway-enrollment')
  && value.webSocketDebuggerUrl);
assert(page, 'Open the exact Hub enrollment page in the signed-in debug App');
const socket = new WebSocket(page.webSocketDebuggerUrl);
try {
  const result = await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('Gate deadline; inspect checkpoint before retrying')), 180_000);
    const fail = () => { clearTimeout(timer); reject(new Error('CDP disconnected; inspect recovery')); };
    socket.onerror = fail;
    socket.onclose = fail;
    socket.onopen = () => socket.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate', params: {
      expression: `(${runInApp.toString()})(${JSON.stringify(config)})`,
      awaitPromise: true, returnByValue: true,
    } }));
    socket.onmessage = event => {
      const message = JSON.parse(event.data);
      if (message.id !== 1) return;
      clearTimeout(timer);
      if (message.error || message.result?.exceptionDetails) reject(new Error('App gate failed; inspect preconditions/recovery'));
      else resolve(message.result.result.value);
    };
  });
  console.log(JSON.stringify(result));
  assert.equal(result.error, 'EDGE_GATEWAY_RECEIPT_CONTEXT_MISMATCH');
  assert.equal(result.installs, config.fault === 'request-replay' ? 2 : 1);
  assert.equal(result.serverRejection, 'EDGE_GATEWAY_RECEIPT_MISMATCH');
  assert.equal(result.serverStatus, 409);
  for (const key of ['injected', 'serverUnchanged', 'checkpoint']) assert.equal(result[key], true, key);
  for (const key of ['method2', 'commits', 'links', 'sessions']) assert.equal(result[key], 0, key);
  if (config.fault === 'request-replay') {
    assert.equal(result.sameFrame, true);
    assert.equal(result.sameReceipt, true);
    assert.equal(result.changedRequestError, 2); // BBP/2 AuthenticationRequired
  }
} finally {
  socket.close();
}
