import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { accountCredential, loadCloudSdk, startCloudObserver } from './edge_gateway_cloud_observer.mjs';
import { observeOnlinePage, assertOnlinePage } from './edge_gateway_online_page_gate.mjs';

// A separately armed, exact-Hub fault must be verified from the Broker trace.
// This measures caller recovery; it does NOT pass the ordinary 30-round gate.
export async function measureRecovery(config) {
  assert(/^ble_[A-Za-z0-9_-]{22}$/.test(config.child) && /^[0-9a-f]{32}$/.test(config.instance), 'GATE_CONFIG');
  assert([0, 5000].includes(config.holdMillis ?? 0), 'GATE_CONFIG');
  assert(config.expectReadyRejected === undefined || config.expectReadyRejected === true, 'GATE_CONFIG');
  const firstRead = config.firstRead ?? 'timeout';
  assert(['timeout', 'ready'].includes(firstRead), 'GATE_CONFIG');
  const started = performance.now();
  const mark = (phase, value = {}) => console.log(JSON.stringify({ phase, ms: Math.round(performance.now() - started), ...value }));
  mark('run-start', { unixMillis: Date.now(), holdMillis: config.holdMillis ?? 0 });
  const controller = new AbortController(), timer = setTimeout(() => controller.abort(), 85000);
  let observer;
  try {
    const sdk = await loadCloudSdk(config.appRoot);
    const credential = await runGatewayAppGate(accountCredential, { pagePath: '/home/device' }, 15000);
    observer = await startCloudObserver(sdk, credential, config.child, controller.signal, mark);
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_BASELINE');
    const result = await runGatewayAppGate(observeOnlinePage, { ...config, pagePath: '/home/device' }, 45000);
    mark('online-page', { result });
    if (config.expectReadyRejected) {
      assert(!result.error && result.idle && result.connects === 0 && result.closes === 0
        && result.events.some(e => e.phase === 'rejection-observed'), 'GATE_REJECTED_ACQUIRED');
    } else assertOnlinePage(result, config.expectedCounter);
    await observer.settled();
    const first = observer.snapshot();
    if (config.expectReadyRejected) {
      assert(!first.fatal && first.ownerActive && first.fresh && first.counter === config.expectedCounter, 'GATE_UNGRANTED_RECOVERY');
      mark('ungranted-recovery-measured', { nativeConnects: 0 }); return;
    }
    if (firstRead === 'ready') {
      assert(!first.fatal && first.ownerActive && first.fresh && first.counter === config.expectedCounter, 'GATE_FIRST_READ_RECOVERY');
      mark('recovery-measured', { originalReadPassed: true, explicitReadPassed: false }); return;
    }
    assert(first.fatal === 'GATE_CLOUD_READ_FAILED', 'GATE_EXPECTED_TIMEOUT_NOT_REPRODUCED');
    await observer.recoverAfterTimeout();
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_RECOVERY');
    mark('recovery-measured', { originalReadPassed: false, explicitReadPassed: true });
  } finally { clearTimeout(timer); await observer?.close(); controller.abort(); mark('closed'); }
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  measureRecovery(JSON.parse(await readFile(process.argv[2], 'utf8'))).catch(() => {
    console.log(JSON.stringify({ phase: 'failed', code: 'GATE_RECOVERY_MEASUREMENT_FAILED' })); process.exitCode = 1;
  });
}
