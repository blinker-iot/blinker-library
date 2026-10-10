import assert from 'node:assert/strict';
import { randomBytes } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { observeOnlinePage, assertOnlinePage, assertCancelledPage } from './edge_gateway_online_page_gate.mjs';
import { accountCredential, loadCloudSdk, startCloudObserver } from './edge_gateway_cloud_observer.mjs';

// Runs inside the authenticated App. Only exact Open/Get/Close; never select a
// candidate, install access material or manufacture a Broker/Direct owner.
export async function permitWindowRequest(input) {
  const ui = ng.getComponent(document.querySelector('app-root')).deviceService.deviceUi;
  const context = ui.ble.accountContext();
  ui.ble.assertAccount(context);
  const session = await Capacitor.Plugins.SecureStorage.get('session');
  if (!session?.accessToken) throw Error('GATE_ACCOUNT');
  const base = '/api/v2/edge-gateway/permit-joins';
  const path = input.action === 'open' ? base : `${base}/${input.operation}${input.action === 'close' ? '/close' : ''}`;
  const started = performance.now();
  const response = await fetch((input.api || 'http://127.0.0.1:7000') + path, {
    method: input.action === 'get' ? 'GET' : 'POST', signal: AbortSignal.timeout(15000),
    headers: { Authorization: 'Bearer ' + session.accessToken, 'Content-Type': 'application/json',
      ...(input.action === 'open' ? { 'Idempotency-Key': input.operation } : {}) },
    body: input.action === 'get' ? undefined : JSON.stringify(input.action === 'open'
      ? { edgeHubLogicalDeviceId: input.hub, adapterId: 1 } : {}),
  });
  ui.ble.assertAccount(context);
  const payload = await response.json(), value = payload.data;
  if (!response.ok || value?.operationId !== input.operation || value?.edgeHubLogicalDeviceId !== input.hub) {
    throw Error('GATE_PERMIT_RESPONSE');
  }
  return { started, observed: performance.now(), state: value.state, expiresAt: value.expiresAt };
}

export function assertManagementYield(page, held, close, terminal, cancelled, expectedCounter) {
  (cancelled ? assertCancelledPage : assertOnlinePage)(page, expectedCounter);
  assert(held.state === 'ready' && terminal.state === 'closed' && held.expiresAt === terminal.expiresAt,
    'GATE_MANAGEMENT_OWNER_CHANGED');
  const time = phase => {
    const event = page.events.find(e => e.phase === phase);
    assert(event && Number.isFinite(event.ms), 'GATE_MANAGEMENT_EVENT');
    return page.started + event.ms;
  };
  // All timestamps use this WebView's monotonic clock. HTTP ACK may follow
  // actual native release, so the boundary is original owner's Close REQUEST.
  assert(time('reserved') < held.started && held.observed <= close.started, 'GATE_NO_CONTENTION');
  if (cancelled) {
    assert(time('page-retired') < held.started && page.finished > terminal.observed + 1000,
      'GATE_CANCEL_NOT_OBSERVED_AFTER_CLOSE');
    assert(!page.events.some(e => e.phase === 'physical-ready'), 'GATE_CANCEL_LATE_READY');
  } else {
    assert(time('physical-ready') >= close.started && time('sdk-connect-start') >= close.started,
      'GATE_MANAGEMENT_PREEMPTED');
  }
}

async function main(config) {
  assert(/^device_[A-Za-z0-9_-]+$/.test(config.hub) && /^ble_[A-Za-z0-9_-]+$/.test(config.child)
    && /^[0-9a-f]{32}$/.test(config.instance), 'GATE_CONFIG');
  const started = performance.now();
  const mark = (phase, detail = {}) => console.log(JSON.stringify({ phase, ms: Math.round(performance.now() - started), ...detail }));
  mark('run-start', { unixMillis: Date.now() });
  const delay = ms => new Promise(r => setTimeout(r, ms));
  const controller = new AbortController(), timer = setTimeout(() => controller.abort(), 150000);
  let observer, operation, pendingPage;
  const window = async action => runGatewayAppGate(permitWindowRequest, { ...config, operation, action, pagePath: '/' }, 20000);
  const until = async (state, timeout = 15000) => {
    const end = performance.now() + timeout;
    do {
      const value = await window('get');
      if (value.state === state) return value;
      assert(['pending_open', 'ready', 'pending_close'].includes(value.state), 'GATE_PERMIT_TERMINATED');
      await delay(250);
    } while (performance.now() < end && !controller.signal.aborted);
    throw Error('GATE_PERMIT_DEADLINE');
  };
  try {
    const sdk = await loadCloudSdk(config.appRoot);
    const credential = await runGatewayAppGate(accountCredential, { pagePath: '/home/device' }, 15000);
    observer = await startCloudObserver(sdk, credential, config.child, controller.signal, mark);
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_BASELINE');
    for (const cancelled of [false, true]) {
      operation = randomBytes(16).toString('base64url');
      mark('permit-opening', { operation, cancelled });
      await window('open');
      mark('permit-ready', { operation, value: await until('ready') });
      // The normal App page owns Direct; this caller independently owns the
      // management window and closes it on its own schedule, never on Direct.
      pendingPage = runGatewayAppGate(observeOnlinePage, { ...config, cancelBeforeReady: cancelled,
        cancelWatchMillis: cancelled ? 7000 : 1500, pagePath: '/home/device' }, 45000);
      void pendingPage.catch(() => {});
      await delay(3000);
      const held = await window('get');
      mark('permit-still-held', { operation, value: held });
      const close = await window('close'), terminal = await until('closed');
      mark('permit-closed', { operation, close, terminal });
      operation = undefined;
      const page = await pendingPage; pendingPage = undefined;
      mark('management-page', { cancelled, result: page });
      assertManagementYield(page, held, close, terminal, cancelled, config.expectedCounter);
      await observer.settled();
      assert(observer.snapshot().fresh && observer.snapshot().ownerActive
        && observer.snapshot().counter === config.expectedCounter, 'GATE_CLOUD_RECOVERY');
      mark('management-passed', { cancelled });
    }
    mark('passed');
  } finally {
    // Exact operation only. A lost response is NOT a reason to open another.
    try {
      if (operation) { await window('close'); mark('cleanup-window', { value: await until('closed') }); }
    } finally {
      try { await pendingPage; } finally { clearTimeout(timer); await observer?.close(); controller.abort(); mark('closed'); }
    }
  }
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main(JSON.parse(await readFile(process.argv[2], 'utf8'))).catch(error => {
    console.log(JSON.stringify({ phase: 'failed', code: /^GATE_[A-Z_]+$/.test(error?.message || '')
      ? error.message : 'GATE_MANAGEMENT_FAILED' })); process.exitCode = 1;
  });
}
