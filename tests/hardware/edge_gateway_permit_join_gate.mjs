import assert from 'node:assert/strict';
import { createHash, randomBytes } from 'node:crypto';
import { createRequire } from 'node:module';

// Public owner API only. Authentication stays inside the signed-in Android
// WebView; this gate neither installs credentials nor creates a topology.
const args = new Map();
for (let index = 2; index < process.argv.length; index += 2) {
  const name = process.argv[index];
  const value = process.argv[index + 1];
  assert(['--hub', '--cdp', '--api', '--protocol'].includes(name) && value,
    'Expected --hub <logical ID> [--cdp <URL>] [--api <URL>] [--protocol <module>]');
  assert(!args.has(name), `Duplicate argument: ${name}`);
  args.set(name, value);
}
const hub = args.get('--hub');
assert(hub && hub.length <= 128 && !/[\0/]/.test(hub), 'Exact Hub logical ID required');
const protocol = createRequire(import.meta.url)(args.get('--protocol')
  || '../../../aily-blinker-services/packages/device-v2-protocol/src/index.js');
const api = args.get('--api') || 'http://127.0.0.1:7000';
const cdp = args.get('--cdp') || 'http://127.0.0.1:9222';
const pages = await (await fetch(`${cdp}/json/list`, {
  signal: AbortSignal.timeout(5000)
})).json();
const page = pages.find(item => item.type === 'page' && item.webSocketDebuggerUrl);
assert(page, 'No debuggable Android WebView');
const socket = new WebSocket(page.webSocketDebuggerUrl);
await new Promise((resolve, reject) => {
  const timer = setTimeout(() => reject(new Error('CDP connection timeout')), 5000);
  socket.addEventListener('open', () => { clearTimeout(timer); resolve(); }, { once: true });
  socket.addEventListener('error', () => {
    clearTimeout(timer); reject(new Error('CDP connection failed'));
  }, { once: true });
});
let nextId = 0;
async function evaluate(expression) {
  const id = ++nextId;
  return new Promise((resolve, reject) => {
    const finish = (error, value) => {
      clearTimeout(timer);
      socket.removeEventListener('message', receive);
      if (error) reject(error); else resolve(value);
    };
    const receive = event => {
      const result = JSON.parse(event.data);
      if (result.id !== id) return;
      if (result.error || result.result?.exceptionDetails) {
        // Do not print WebView exceptions: they may embed authentication data.
        finish(new Error('WebView request failed; check login and local API reachability'));
      } else finish(null, result.result?.result?.value);
    };
    const timer = setTimeout(() => finish(new Error('CDP request timeout')), 20_000);
    socket.addEventListener('message', receive);
    socket.send(JSON.stringify({ id, method: 'Runtime.evaluate', params: {
      expression, awaitPromise: true, returnByValue: true
    } }));
  });
}
async function request(path, method = 'GET', body, idempotencyKey) {
  return evaluate(`(async () => {
    const session = await Capacitor.Plugins.SecureStorage.get('session');
    if (!session?.accessToken) throw new Error('not signed in');
    const response = await fetch(${JSON.stringify(api + path)}, {
      method: ${JSON.stringify(method)}, signal: AbortSignal.timeout(15000),
      headers: { Authorization: 'Bearer ' + session.accessToken,
        'Content-Type': 'application/json',
        ...(${JSON.stringify(idempotencyKey || '')}
          ? { 'Idempotency-Key': ${JSON.stringify(idempotencyKey || '')} } : {}) },
      body: ${JSON.stringify(body === undefined ? null : JSON.stringify(body))} || undefined
    });
    const payload = await response.json();
    return { status: response.status, data: payload.data, code: payload.errorCode };
  })()`);
}
function success(response, status = 200) {
  assert.equal(response?.status, status, `HTTP ${response?.status}: ${response?.code || 'unexpected response'}`);
  assert(response.data, 'Missing API data envelope');
  return response.data;
}
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(label, read, accept, timeout = 30_000) {
  const deadline = Date.now() + timeout;
  do {
    const result = await read();
    if (accept(result)) return result;
    await delay(1000);
  } while (Date.now() < deadline);
  throw new Error(`${label} deadline exceeded`);
}
const operationId = randomBytes(16);
const operation = operationId.toString('base64url');
const root = '/api/v2/edge-gateway/permit-joins';
const item = `${root}/${operation}`;
const openBody = { edgeHubLogicalDeviceId: hub, adapterId: 1 };
function log(stage, detail = {}) {
  console.log(JSON.stringify({ operation, stage, ...detail }));
}
async function windowState() {
  const window = success(await request(item));
  assert.equal(window.operationId, operation);
  assert.equal(window.edgeHubLogicalDeviceId, hub);
  return window;
}
async function relay() {
  const view = success(await request(`${item}/relay`));
  assert.equal(view.operationId, operation);
  return view;
}
function select(token) {
  return Buffer.from(protocol.encodePermitJoinSelectCommand({
    version: 1, operationId, candidateToken: token
  })).toString('base64url');
}
let opened = false;
try {
  // Set before sending: even a lost HTTP response must trigger exact Close.
  opened = true;
  const initial = success(await request(root, 'POST', openBody, operation), 201);
  assert.equal(initial.operationId, operation);
  log('opened', { state: initial.state });
  await until('Hub Ready', windowState, value => {
    assert(['pending_open', 'ready'].includes(value.state), `Window ${value.state}`);
    return value.state === 'ready';
  });
  log('ready');
  const repeated = success(await request(root, 'POST', openBody, operation));
  assert.equal(repeated.replayed, true);
  assert.equal(repeated.expiresAt, initial.expiresAt);
  const changedOpen = await request(root, 'POST', { ...openBody, adapterId: 2 }, operation);
  assert.equal(changedOpen.status, 409);
  assert.equal(changedOpen.code, 'EDGE_GATEWAY_PERMIT_JOIN_IDEMPOTENCY_CONFLICT');
  const discovered = await until('Real Hub candidate', async () => {
    const view = await relay();
    return view.candidateSnapshot
      ? protocol.decodePermitJoinCandidateSnapshot(Buffer.from(view.candidateSnapshot, 'base64url')) : null;
  }, value => value?.candidates.length > 0);
  assert.deepEqual(Buffer.from(discovered.operationId), operationId);
  const token = Buffer.from(discovered.candidates[0].candidateToken);
  const changed = Buffer.from(token);
  changed[changed.length - 1] ^= 1;
  assert(!discovered.candidates.some(value => Buffer.from(value.candidateToken).equals(changed)),
    'Bit-flipped token unexpectedly matches a real candidate; do not select it');
  const frame = select(changed);
  success(await request(`${item}/relay`, 'POST', { frame }));
  const selected = await until('NotFound result', relay, view => !!view.selectResult);
  const result = protocol.decodePermitJoinSelectResult(Buffer.from(selected.selectResult, 'base64url'));
  assert.deepEqual(Buffer.from(result.operationId), operationId);
  assert.deepEqual(Buffer.from(result.candidateToken), changed);
  assert.equal(result.status, protocol.EdgeGatewayPermitJoinSelectStatus.NotFound);
  assert.equal(result.maxPacketSize, 0);
  const exact = success(await request(`${item}/relay`, 'POST', { frame }));
  assert.equal(exact.relaySessionId, selected.relaySessionId);
  assert.equal(exact.selectResult, selected.selectResult);
  const conflict = await request(`${item}/relay`, 'POST', { frame: select(token) });
  assert.equal(conflict.status, 409);
  assert.equal(conflict.code, 'EDGE_GATEWAY_PERMIT_JOIN_RELAY_CONFLICT');
  const unchanged = await relay();
  assert.equal(unchanged.selectResult, selected.selectResult);
  assert.equal(unchanged.upstreamBatch, null);
  log('select-negative-passed', {
    candidates: discovered.candidates.length, notFound: true, exactReplay: true,
    changedReplay: conflict.status,
    resultDigest: createHash('sha256').update(Buffer.from(selected.selectResult, 'base64url')).digest('hex')
  });
} finally {
  try {
    if (opened) {
      const closed = await request(`${item}/close`, 'POST', {});
      if (closed.status !== 404) {
        success(closed);
        const terminal = await until('Window Close', windowState,
          value => ['closed', 'expired', 'rejected', 'busy', 'unsupported'].includes(value.state));
        log('closed', { state: terminal.state });
      }
    }
  } finally {
    socket.close();
  }
}
