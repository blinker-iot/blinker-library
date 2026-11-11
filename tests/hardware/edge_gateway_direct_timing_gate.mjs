import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Timing observer only. The operator must isolate the Hub's child radio first.
// This is NOT Ready, offline admission, a fairness test, or a permit-join flow.
// Uses the installed App's real discovery, Method 2 and initial State; no Action.
export async function measureDirectInApp(input) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  must(input.mode === 'isolated-baseline' && input.hubIsolated === true
    && /^ble_[A-Za-z0-9_-]+$/.test(input.child) && /^[a-f0-9]{32}$/.test(input.instance)
    && Number.isInteger(input.count) && input.count >= 1 && input.count <= 10, 'GATE_INPUT');
  const root = globalThis.ng?.getComponent(document.querySelector('app-root'));
  const ui = root?.deviceService?.deviceUi, ble = ui?.ble;
  must(ui?.active && ui.data.auth && ble && !ble.active && !ble.opening
    && ble.accountLinks.size === 0 && ble.accountSessions.size === 0
    && [...ui.connections.values()].every(group => group.scopes.size === 0)
    && !globalThis.__edgeDirectTiming, 'GATE_BUSY');
  const context = ble.accountContext();
  const instance = await ble.credentialDeviceInstanceId(input.child);
  try { must([...instance].map(b => b.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  must(await ble.hasActiveCredential(input.child), 'GATE_CREDENTIAL');
  ble.assertAccount(context);
  must(ui.active && !globalThis.__edgeDirectTiming && !ble.active && !ble.opening
    && ble.accountLinks.size === 0 && ble.accountSessions.size === 0
    && [...ui.connections.values()].every(group => group.scopes.size === 0), 'GATE_BUSY');
  const abort = new AbortController();
  globalThis.__edgeDirectTiming = abort;
  const originalClient = ble.client, originalDiscover = ble.discoverDirect;
  const restore = [], results = [], closing = new Set();
  let current;
  const mark = phase => { if (current?.events.length < 24) current.events.push({ phase, ms: Math.round(performance.now() - current.start) }); };
  const wrap = (object, name, phase) => {
    const original = object[name], row = current;
    const event = suffix => { if (row.events.length < 24) row.events.push({ phase: `${phase}-${suffix}`, ms: Math.round(performance.now() - row.start) }); };
    object[name] = async function(...args) {
      event('start');
      const work = Promise.resolve().then(() => original.apply(this, args));
      if (phase === 'native-close') closing.add(work);
      try { const value = await work; event('done'); return value; }
      finally { closing.delete(work); }
    };
    restore.push(() => { object[name] = original; });
  };
  ble.discoverDirect = async function(...args) {
    mark('discovery-start'); const target = await originalDiscover.apply(this, args);
    // Only public advertisement version/capability, never locator/address/key.
    current.profile = { wireVersion: target.profile.wireVersion, capabilities: target.profile.capabilities };
    must(target.profile.wireVersion === 3, 'GATE_AUTHORIZED_LOCATOR_REQUIRED');
    mark('discovery-done'); return target;
  };
  ble.client = function(link, ...args) {
    wrap(link, 'connect', 'native-gatt'); wrap(link, 'disconnect', 'native-close');
    const client = originalClient.call(this, link, ...args), authenticate = client.authenticate;
    client.authenticate = async function(...values) {
      mark('method2-start'); const session = await authenticate.apply(this, values);
      mark('method2-done'); wrap(session, 'synchronize', 'initial-state'); return session;
    };
    restore.push(() => { client.authenticate = authenticate; });
    return client;
  };
  const timer = setTimeout(() => { abort.abort(); void ble.disconnect(input.child).catch(() => undefined); }, 60_000);
  const check = () => { abort.signal.throwIfAborted(); ble.assertAccount(context); must(ui.active, 'GATE_BACKGROUND'); };
  const close = async () => {
    let failed = false;
    try { await ble.disconnect(input.child); } catch { failed = true; }
    // One failed close must not abandon other SDK work still being observed.
    while (closing.size) {
      if ((await Promise.allSettled([...closing])).some(value => value.status === 'rejected')) failed = true;
    }
    must(!failed, 'GATE_CLOSE_FAILED');
  };
  try {
    for (let index = 0; index < input.count; ++index) {
      check(); current = { iteration: index + 1, start: performance.now(), events: [], error: '' };
      try {
        // Neutral test admission cancels this link on the new App. Old APKs
        // are separately identified and cannot certify the new native guard.
        await ble.ensureReady(input.child, 5000, context, { signal: abort.signal, assertAcquire: check, assertActive: check });
        check(); must(ble.active?.logicalDeviceId === input.child && ble.active.session.state === 'ready', 'GATE_NOT_READY');
        const snapshot = ble.active.session.store.snapshot(input.child);
        must(snapshot.manifestAccepted && snapshot.stateFresh, 'GATE_STATE_NOT_FRESH');
        mark('ready');
      } catch (error) {
        const message = error?.message || '';
        current.error = /^(BLE_DIRECT|BLE_SCAN|GATE)_[A-Z0-9_]{1,80}$/.test(message) ? message
          : error?.name === 'AbortError' ? 'GATE_CANCELLED' : 'GATE_UNEXPECTED';
      } finally {
        // Old APKs may return logical closed before a concurrent native close.
        // Keep that observation in its own iteration and do not overlap tests.
        try { await close(); mark('closed'); }
        catch { current.error = 'GATE_CLOSE_FAILED'; }
        current.links = ble.accountLinks.size; current.sessions = ble.accountSessions.size;
        delete current.start; results.push(current); while (restore.length) restore.pop()();
      }
      if (current.error || current.links || current.sessions) break; // No hidden retry.
    }
    return { mode: input.mode, nativeAdmissionBuild: typeof ble.commandConnected === 'function', results };
  } finally {
    clearTimeout(timer); abort.abort();
    try { await close(); }
    finally {
      ble.client = originalClient; ble.discoverDirect = originalDiscover;
      while (restore.length) restore.pop()();
      delete globalThis.__edgeDirectTiming;
    }
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const { readFile } = await import('node:fs/promises');
  const config = JSON.parse(await readFile(process.argv[2], 'utf8'));
  const result = await runGatewayAppGate(measureDirectInApp, { ...config, pagePath: '/home/device' }, 75_000);
  console.log(JSON.stringify(result));
  assert.equal(result.results.length, config.count);
  for (const row of result.results) assert(!row.error && row.links === 0 && row.sessions === 0, 'GATE_DIRECT_TIMING_FAILED');
}
