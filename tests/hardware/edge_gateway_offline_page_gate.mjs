import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Observe a real cold-start normal page. The caller owns radio/reverse setup
// and restoration. No catalog injection, direct-connect shortcut or Action.
export async function observeOfflinePage(input) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  const root = globalThis.ng?.getComponent(document.querySelector('app-root'));
  const ui = root?.deviceService?.deviceUi, ble = ui?.ble, cap = globalThis.Capacitor;
  const idle = () => !ble.active && !ble.opening && !ble.closing && !ble.accountLinks.size && !ble.accountSessions.size;
  must((input.hubIsolated === true || input.pairedCloud === true) && /^ble_[A-Za-z0-9_-]+$/.test(input.child)
    && /^[a-f0-9]{32}$/.test(input.instance), 'GATE_INPUT');
  must(ui?.active && ui.data.auth && ui.network.offline && ui.deviceV2.state.value !== 'ready'
    && idle() && [...ui.connections.values()].every(g => !g.scopes.size), 'GATE_OFFLINE_PRECONDITION');
  const device = ui.data.getDevice(input.child);
  must(device?.deviceType === 'ble' && device.cloudEnabled === true && device.config?.mode === 'bbp2'
    && !device.config.disabled && device.data.cloudReachable === null, 'GATE_CATALOG_NOT_RESTORED');
  const context = ble.accountContext(), instance = await ble.credentialDeviceInstanceId(input.child);
  try { must([...instance].map(b => b.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  must(await ble.hasActiveCredential(input.child), 'GATE_CREDENTIAL');
  ble.assertAccount(context);
  const native = cap.nativePromise, started = performance.now();
  const result = { nativeOffline: true, catalogRestored: true, events: [], connects: 0, closes: 0,
    readyMs: null, expiredMs: null, physicalReleasedMs: null, quietMs: 0, counter: null, physicalReleased: false, error: '' };
  const mark = phase => { if (result.events.length < 24) result.events.push({ phase, ms: Math.round(performance.now() - started) }); };
  const safeCode = error => /^(GATE|BLE_DIRECT|BLE_SCAN)_[A-Z0-9_]{1,80}$/.test(error?.message || '')
    ? error.message : 'GATE_UNEXPECTED';
  const check = () => { ble.assertAccount(context); must(ui.active && ui.network.offline
    && ui.deviceV2.state.value !== 'ready', 'GATE_OFFLINE_LOST'); };
  let target;
  cap.nativePromise = async function(plugin, method, options, ...rest) {
    if (plugin === 'BluetoothLe' && method === 'connect') {
      must(!target || target === options.deviceId, 'GATE_OTHER_DEVICE');
      target = options.deviceId; ++result.connects; mark('sdk-connect-start');
      const value = await native.call(this, plugin, method, options, ...rest); mark('sdk-connect-done'); return value;
    }
    if (plugin === 'BluetoothLe' && method === 'disconnect' && options.deviceId === target) {
      ++result.closes; mark('sdk-close-start');
      const value = await native.call(this, plugin, method, options, ...rest); mark('sdk-close-done'); return value;
    }
    return native.call(this, plugin, method, options, ...rest);
  };
  try {
    check(); must(idle(), 'GATE_BUSY');
    await root.navCtrl.navigateRoot(`/device/${input.child}`);
    const end = performance.now() + 40000;
    do {
      check();
      const page = [...document.querySelectorAll('app-device-v2-page')].map(e => ng.getComponent(e))
        .find(p => p.viewActive && p.device?.deviceName === input.child);
      if (page?.direct && page.canControl && page.snapshot.stateFresh) {
        result.readyMs ??= Math.round(performance.now() - started);
        result.counter = page.snapshot.endpoints.find(e => e.key === 'buttonPresses')?.value;
      }
      if (result.readyMs !== null && page && !page.canControl && result.closes && idle()) {
        result.expiredMs = Math.round(performance.now() - started);
        // Android's manager list and the per-GATT disconnect callback are
        // distinct observations. Never equate the first with the second.
        const releaseDeadline = performance.now() + 4000;
        do {
          check(); must(idle() && result.connects === 1, 'GATE_RELEASE_INTERRUPTED');
          const state = await native.call(cap, 'BluetoothLe', 'getConnectedDevices', {});
          result.physicalReleased = !state.devices.some(d => d.deviceId === target);
          if (result.physicalReleased) { result.physicalReleasedMs = Math.round(performance.now() - started); break; }
          await new Promise(resolve => setTimeout(resolve, 100));
        } while (performance.now() < releaseDeadline);
        break;
      }
      await new Promise(resolve => setTimeout(resolve, 150));
    } while (performance.now() < end);
    must(result.readyMs !== null && result.expiredMs !== null && result.physicalReleased
      && result.counter === input.expectedCounter && result.connects === 1, 'GATE_OFFLINE_NOT_COMPLETED');
    const quiet = performance.now();
    do {
      check(); must(idle() && result.connects === 1, 'GATE_QUIET_INTERRUPTED');
      await new Promise(resolve => setTimeout(resolve, 150));
    } while (performance.now() - quiet < 6000);
    result.quietMs = Math.round(performance.now() - quiet);
  } catch (error) { result.error = safeCode(error); }
  finally {
    try { await root.navCtrl.navigateRoot('/home/device'); await ble.disconnect(input.child); }
    catch (error) { result.error ||= safeCode(error); }
    finally { cap.nativePromise = native; }
    result.links = ble.accountLinks.size; result.sessions = ble.accountSessions.size;
    result.closing = !!ble.closing; result.opening = !!ble.opening;
    result.scopes = [...ui.connections.values()].reduce((n, g) => n + g.scopes.size, 0);
  }
  return result;
}

export function assertOfflinePageResult(result, expectedCounter) {
  assert(result && result.error === '' && result.nativeOffline === true && result.catalogRestored === true
    && result.physicalReleased === true && result.connects === 1 && result.closes >= 1
    && Number.isFinite(result.readyMs) && result.readyMs >= 0 && result.expiredMs > result.readyMs
    && Number.isFinite(result.physicalReleasedMs) && result.physicalReleasedMs >= result.expiredMs
    && result.quietMs >= 6000 && result.counter === expectedCounter
    && result.links === 0 && result.sessions === 0 && result.scopes === 0
    && result.closing === false && result.opening === false, 'GATE_OFFLINE_FAILED');
  assert(result.events.some(e => e.phase === 'sdk-close-done'), 'GATE_SDK_RELEASE_MISSING');
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const [child, instance, expectedCounter, isolated] = process.argv.slice(2);
  assert(isolated === '--hub-isolated' && /^\d+$/.test(expectedCounter || ''), 'Confirm isolated Hub and expected counter');
  const input = { child, instance, expectedCounter: Number(expectedCounter), hubIsolated: true, pagePath: '/home/device' };
  const result = await runGatewayAppGate(observeOfflinePage, input, 55000);
  console.log(JSON.stringify(result));
  assertOfflinePageResult(result, input.expectedCounter);
}
