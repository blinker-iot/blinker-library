import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Lookup only: random nonexistent provisioning name; no selection, GATT or keys.
export async function scanLookupGate() {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  const root = globalThis.ng.getComponent(document.querySelector('app-root')), ui = root.deviceService.deviceUi;
  must(ui.active && ui.data.auth && !ui.ble.active && !ui.ble.opening && !ui.ble.presence
    && !ui.ble.accountLinks.size && !ui.ble.accountSessions.size
    && [...ui.connections.values()].every(group => !group.scopes.size), 'GATE_BUSY');
  const native = globalThis.Capacitor.Plugins.BlinkerBleScan, prov = globalThis.Capacitor.Plugins.WiFiProv;
  const empty = async () => {
    try { await prov.getDeviceInfo(); return false; }
    catch (error) { must(error.message === 'No ESP device is selected.', 'GATE_PROV_INFO'); return true; }
  };
  must(await empty(), 'GATE_PROV_SELECTED');
  const nonce = crypto.randomUUID().replaceAll('-', ''), name = `GATE_${nonce.slice(0, 20)}`, id = `lookup:${nonce}`;
  const services = ['5f6d0001-3f5b-4e4f-9f4d-626c696e6b32'];
  const timers = new Set(), events = [];
  const bounded = (work, ms) => new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(Error('GATE_LOOKUP_TIMEOUT')), ms); timers.add(timer);
    Promise.resolve(work).then(resolve, reject).finally(() => { clearTimeout(timer); timers.delete(timer); });
  });
  const result = work => work.then(() => 'GATE_UNEXPECTED_SELECTION', error =>
    /^BLE_SCAN_[A-Z_]+$/.test(error.message || '') ? error.message
      : error.message === `No BLE provisioning device named ${name} was found.` ? 'NOT_FOUND' : 'GATE_LOOKUP_FAILED');
  let started;
  const listener = await prov.addListener('bleScanState', event => {
    if (event.state === 'started') started?.();
    if (event.state === 'failed') events.push(/^BLE_SCAN_[A-Z_]+$/.test(event.message || '') ? event.message : 'GATE_SCAN_FAILED');
  });
  const begin = async qr => {
    const submitted = new Promise(resolve => { started = resolve; });
    const pending = result(qr ? prov.createDeviceFromQr({ payload: JSON.stringify({ ver: 'v1', transport: 'ble', name, security: 1 }) })
      : prov.createDevice({ name, transport: 'ble', security: 1 }));
    // start event precedes submission; a subsequent native Busy proves the slot.
    await bounded(submitted, 2000); started = undefined;
    try { await native.start({ id, services }); throw Error('GATE_NOT_EXCLUSIVE'); }
    catch (error) {
      if (error.message !== 'BLE_SCAN_BUSY') throw Error(/^BLE_SCAN_[A-Z_]+$/.test(error.message || '') ? error.message : 'GATE_EXPECTED_BUSY');
    }
    return { pending };
  };
  let phase = 'name';
  try {
    const first = await begin(false);
    must(await bounded(result(prov.createDevice({ name, transport: 'ble', security: 1 })), 2000) === 'BLE_SCAN_BUSY', 'GATE_OVERLAP');
    await prov.stopBleScan();
    must(await bounded(first.pending, 2000) === 'BLE_SCAN_CANCELLED', 'GATE_NAME_CANCEL');
    must(await empty(), 'GATE_LATE_SELECTION');
    await native.start({ id, services }); await native.stop({ id });
    phase = 'qr';
    const second = await begin(true);
    await prov.clearState();
    must(await bounded(second.pending, 2000) === 'BLE_SCAN_CANCELLED', 'GATE_QR_CANCEL');
    must(await empty(), 'GATE_LATE_SELECTION');
    await native.start({ id, services }); await native.stop({ id });
    phase = 'deadline';
    const before = performance.now(), third = await begin(false);
    must(await bounded(third.pending, 13_000) === 'NOT_FOUND', 'GATE_LOOKUP_DEADLINE');
    must(await empty(), 'GATE_LATE_SELECTION');
    return { nameCancelled: true, qrCancelled: true, overlapRejected: true, noSelection: true,
      handoff: true, deadlineMs: Math.round(performance.now() - before), events, passed: true };
  } catch (error) {
    return { phase, passed: false, error: /^(BLE_SCAN|GATE)_[A-Z_]+$/.test(error.message || '') ? error.message : 'GATE_UNEXPECTED', events };
  } finally {
    for (const timer of timers) clearTimeout(timer);
    // This gate owns no pre-existing provisioning selection or connection.
    await Promise.allSettled([native.stop({ id }), prov.stopBleScan()]);
    await listener.remove();
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const result = await runGatewayAppGate(scanLookupGate, { pagePath: '/home/device' }, 25_000);
  console.log(JSON.stringify(result)); assert(result.passed);
}
