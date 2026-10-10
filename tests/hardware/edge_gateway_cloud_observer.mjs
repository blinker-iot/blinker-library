// Test-only caller using the production account client and channel. Credentials
// stay in process memory; no device login, Broker owner injection or Action API.
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

// Whitelist readiness facts, never dump the store, credentials or user values.
export function cloudProjection(snapshot) {
  return { reachable: snapshot.cloudReachable === true ? true : snapshot.cloudReachable === false ? false : null,
    manifest: snapshot.manifestAccepted === true, fresh: snapshot.stateFresh === true,
    revision: Number.isSafeInteger(snapshot.stateRevision) && snapshot.stateRevision >= 0 ? snapshot.stateRevision : null };
}

export function readinessErrorCode(error) {
  const code = error?.code;
  return Number.isInteger(code) && code >= 0 && code <= 255 ? code
    : typeof code === 'string' && /^DEVICE_V2_READY_(TIMEOUT|CANCELLED|RETIRED|CAPACITY)$/.test(code) ? code : 'UNCLASSIFIED';
}

export async function loadCloudSdk(appRoot) {
  const require = createRequire(appRoot + '/package.json');
  const bundlePath = fileURLToPath(new URL('../../build/fairness-cloud-sdk.cjs', import.meta.url));
  await require('esbuild').build({ stdin: { contents:
    "export { DeviceV2AccountClient } from './src/app/core/device-v2/account-client';\n"
    + "export { openMqttDeviceV2Channel } from './src/app/core/device-v2/mqtt-channel';\n"
    + "export { isDeviceV2TargetReady } from './src/app/core/protocol/device-v2';",
    resolveDir: appRoot, loader: 'ts' }, platform: 'node', format: 'cjs', bundle: true,
    outfile: bundlePath, logLevel: 'silent' });
  return require(bundlePath);
}

// Runs in the signed-in debug App. The runner must never log the return value.
export async function accountCredential() {
  const ui = ng.getComponent(document.querySelector('app-root')).deviceService.deviceUi;
  if (!ui.active || !ui.data.auth || ui.network.offline || ui.ble.active || ui.ble.opening
    || [...ui.connections.values()].some(g => g.scopes.size)) throw Error('GATE_CREDENTIAL_CONTEXT');
  const context = ui.ble.accountContext();
  const credential = await ui.deviceV2.client.credentials();
  ui.ble.assertAccount(context);
  return credential;
}

export async function startCloudObserver(sdk, credentials, child, signal, mark) {
  let issued = false, closing = false, fatal = '', owner, active = false, reading = false;
  let ownerRetired = false, recoveryCode, explicitRecoveryStarted = false;
  let pending = Promise.resolve();
  const client = new sdk.DeviceV2AccountClient(async () => {
    if (issued) throw Error('GATE_CLOUD_CREDENTIAL_REUSED');
    issued = true; return credentials;
  }, response => sdk.openMqttDeviceV2Channel(response.mqtt));
  const ready = () => sdk.isDeviceV2TargetReady(client.snapshot(child));
  let lastReady = false, lastProjection = '';
  const detach = client.store.subscribe(id => {
    if (id !== child) return;
    const fresh = ready();
    const projection = cloudProjection(client.snapshot(child)), key = JSON.stringify(projection);
    if (key !== lastProjection) { lastProjection = key; mark('cloud-projection', projection); }
    if (fresh !== lastReady) { lastReady = fresh; mark('cloud-fresh', { fresh, counter: client.snapshot(child).values?.buttonPresses?.value }); }
    if (active && !fresh && !reading && !fatal) {
      reading = true;
      // Re-arm after each actual loss, including a second normal Hub yield.
      // Production readiness already waits for Presence; no polling or Action retry.
      pending = client.waitUntilReady(child, signal).then(() => mark('cloud-resynchronized', {}), error => {
        if (!closing) { fatal = 'GATE_CLOUD_READ_FAILED'; recoveryCode = readinessErrorCode(error); mark('cloud-recovery-failed', { code: recoveryCode,
          ...cloudProjection(client.snapshot(child)) }); }
      }).finally(() => { reading = false; });
    }
  });
  const detachState = client.subscribeState(state => { mark('cloud-account', { state }); });
  const close = async () => {
    closing = true; active = false; owner?.release();
    try { await client.stop(); } finally { detach(); detachState(); credentials = undefined; }
  };
  try {
    await client.start();
    owner = await client.acquireConnectionDemand(child, signal);
    await owner.accepted;
    mark('cloud-owner-accepted', {});
    void owner.closed.then(reason => { ownerRetired = true; if (!closing) { fatal = 'GATE_CLOUD_OWNER_RETIRED'; mark('cloud-owner-retired', { reason }); } });
    await client.waitUntilReady(child, signal);
    active = true;
  } catch (error) { await close(); throw error; }
  return {
    snapshot: () => ({ fresh: ready(), counter: client.snapshot(child).values?.buttonPresses?.value,
      state: client.state, ownerActive: !closing && !fatal, fatal }),
    settled: () => pending,
    // Diagnostic caller only: explicitly measure ONE new read after timeout.
    // Keep the original failure/fatal visible; never re-acquire demand or retry Action.
    recoverAfterTimeout: async () => {
      if (closing || ownerRetired || reading || explicitRecoveryStarted || signal.aborted
        || fatal !== 'GATE_CLOUD_READ_FAILED' || recoveryCode !== 'DEVICE_V2_READY_TIMEOUT') {
        throw Error('GATE_RECOVERY_PRECONDITION');
      }
      explicitRecoveryStarted = true;
      mark('cloud-explicit-read-start', { attempt: 2 });
      try {
        await client.waitUntilReady(child, signal);
        if (closing || ownerRetired || signal.aborted) throw Error('GATE_RECOVERY_RETIRED');
        mark('cloud-explicit-read-ready', { attempt: 2 });
        return cloudProjection(client.snapshot(child));
      } catch (error) {
        mark('cloud-explicit-read-failed', { attempt: 2, code: readinessErrorCode(error) });
        throw error;
      }
    },
    close,
  };
}
