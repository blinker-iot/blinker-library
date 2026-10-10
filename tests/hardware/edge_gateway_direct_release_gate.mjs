import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Test-only bridge observation. Calls the REAL SDK, never fabricates a reject
// or release receipt. No production hooks, credentials, Action or enrollment.
export async function checkDirectReleaseInApp(input) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  must(input.hubIsolated === true && ['late-connect', 'close-timeout'].includes(input.mode)
    && /^ble_[A-Za-z0-9_-]+$/.test(input.child) && /^[a-f0-9]{32}$/.test(input.instance), 'GATE_INPUT');
  const ui = globalThis.ng?.getComponent(document.querySelector('app-root'))?.deviceService?.deviceUi;
  const ble = ui?.ble, cap = globalThis.Capacitor;
  const idle = () => ui?.active && ui.data.auth && ble && !ble.active && !ble.opening && !ble.closing
    && ble.accountLinks.size === 0 && ble.accountSessions.size === 0
    && [...ui.connections.values()].every(group => group.scopes.size === 0);
  must(idle() && typeof cap?.nativePromise === 'function' && !globalThis.__edgeDirectRelease, 'GATE_BUSY');
  const context = ble.accountContext(), instance = await ble.credentialDeviceInstanceId(input.child);
  try { must([...instance].map(b => b.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  must(await ble.hasActiveCredential(input.child), 'GATE_CREDENTIAL');
  ble.assertAccount(context);
  must(idle() && !globalThis.__edgeDirectRelease, 'GATE_BUSY');
  const abort = new AbortController(), native = cap.nativePromise, started = performance.now();
  const result = { mode: input.mode, events: [], nativeConnects: 0, nativeCloses: 0,
    sdkTimeout: false, ready: false, physicalReleased: false, blocked: false, error: '' };
  let target, injected = false;
  const mark = phase => { if (result.events.length < 24) result.events.push({ phase, ms: Math.round(performance.now() - started) }); };
  const code = error => /^(BLE_DIRECT|BLE_SCAN|GATE)_[A-Z0-9_]{1,80}$/.test(error?.message || '')
    ? error.message : error?.name === 'AbortError' ? 'GATE_CANCELLED' : 'GATE_UNEXPECTED';
  const check = () => { abort.signal.throwIfAborted(); ble.assertAccount(context); must(ui.active, 'GATE_BACKGROUND'); };
  const admission = { signal: abort.signal, assertAcquire: check, assertActive: check };
  globalThis.__edgeDirectRelease = abort;
  const timer = setTimeout(() => abort.abort(), 45_000);
  cap.nativePromise = async function(plugin, method, options, ...rest) {
    if (plugin === 'BluetoothLe' && method === 'connect') {
      must(!target || target === options.deviceId, 'GATE_OTHER_DEVICE');
      target = options.deviceId; ++result.nativeConnects; mark('sdk-connect-start');
      const value = await native.call(this, plugin, method, options, ...rest);
      mark('sdk-connect-done');
      if (input.mode === 'late-connect' && !injected) {
        injected = true; abort.abort(); mark('cancel-before-connect-delivery');
      }
      return value;
    }
    if (plugin === 'BluetoothLe' && method === 'disconnect' && target === options.deviceId) {
      ++result.nativeCloses; mark('sdk-close-start');
      if (input.mode === 'close-timeout' && !injected) { injected = true; options = { ...options, timeout: 1 }; }
      try {
        const value = await native.call(this, plugin, method, options, ...rest);
        mark('sdk-close-done'); return value;
      } catch (error) {
        result.sdkTimeout ||= error?.message === 'Disconnection timeout.';
        mark('sdk-close-rejected'); throw error; // Preserve the actual SDK result.
      }
    }
    return native.call(this, plugin, method, options, ...rest);
  };
  try {
    let openingError;
    try {
      await ble.ensureReady(input.child, 5000, context, admission);
      const state = ble.active?.session.store.snapshot(input.child);
      result.ready = !!state?.manifestAccepted && state.stateFresh;
    } catch (error) { openingError = code(error); }
    mark('opening-settled');
    if (input.mode === 'late-connect') {
      must(injected && !result.ready && ['GATE_CANCELLED', 'BLE_DIRECT_CONNECT_CANCELLED'].includes(openingError), 'GATE_LATE_NOT_CANCELLED');
      await ble.disconnect(input.child);
      must(ble.accountLinks.size === 0 && ble.accountSessions.size === 0 && !ble.opening && !ble.closing, 'GATE_RELEASE_REMAINS');
    } else {
      must(result.ready && !openingError, openingError || 'GATE_NOT_READY');
      let failure;
      try { await ble.disconnect(input.child); } catch (error) { failure = code(error); }
      must(injected && result.sdkTimeout && failure === 'BLE_DIRECT_NATIVE_RELEASE_FAILED', 'GATE_FAULT_NOT_OBSERVED');
      const before = result.nativeConnects;
      let repeated, successor;
      try { await ble.disconnect(input.child); } catch (error) { repeated = code(error); }
      try { await ble.ensureReady(input.child, 5000, context, admission); } catch (error) { successor = code(error); }
      result.blocked = repeated === failure && successor === failure && before === result.nativeConnects;
      must(result.blocked, 'GATE_FAILED_CLOSE_BYPASSED');
    }
    // Query Android's real connected-device state; keep locator/address inside
    // the WebView. OS absence is evidence, not permission to clear quarantine.
    const deadline = performance.now() + 4000;
    do {
      const state = await native.call(cap, 'BluetoothLe', 'getConnectedDevices', {});
      result.physicalReleased = !state.devices.some(device => device.deviceId === target);
      if (result.physicalReleased) break;
      await new Promise(resolve => setTimeout(resolve, 100));
    } while (performance.now() < deadline);
    must(result.physicalReleased, 'GATE_PHYSICAL_RELEASE_UNCONFIRMED');
    mark('physical-release-observed');
  } catch (error) { result.error = code(error); }
  finally {
    abort.abort();
    try { await ble.disconnect(input.child); }
    catch (error) { if (!(result.blocked && code(error) === 'BLE_DIRECT_NATIVE_RELEASE_FAILED')) result.error ||= code(error); }
    finally {
      cap.nativePromise = native; clearTimeout(timer); delete globalThis.__edgeDirectRelease;
      result.links = ble.accountLinks.size; result.sessions = ble.accountSessions.size;
      result.closing = !!ble.closing;
    }
  }
  return result;
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const { readFile } = await import('node:fs/promises');
  const config = JSON.parse(await readFile(process.argv[2], 'utf8'));
  const result = await runGatewayAppGate(checkDirectReleaseInApp, { ...config, pagePath: '/home/device' }, 60_000);
  console.log(JSON.stringify(result));
  assert(!result.error && result.physicalReleased, 'GATE_RELEASE_FAILED');
}
