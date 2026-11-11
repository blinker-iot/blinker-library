import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Android scan-only integration; never selects a peripheral, provisions or sends an Action.
export async function scanResourceGate() {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  const root = globalThis.ng.getComponent(document.querySelector('app-root')), ui = root.deviceService.deviceUi;
  must(ui.active && ui.data.auth && !ui.ble.active && !ui.ble.opening && !ui.ble.presence
    && !ui.ble.accountLinks.size && !ui.ble.accountSessions.size
    && [...ui.connections.values()].every(group => !group.scopes.size), 'GATE_BUSY');
  const native = globalThis.Capacitor.Plugins.BlinkerBleScan, prov = globalThis.Capacitor.Plugins.WiFiProv;
  const id = `scan-gate:${crypto.randomUUID()}`, other = `scan-gate:${crypto.randomUUID()}`;
  const services = ['5f6d0001-3f5b-4e4f-9f4d-626c696e6b32'];
  const errors = [];
  let failure, timer, phase = 'direct-start';
  const listener = await prov.addListener('bleScanState', event => {
    if (event.state === 'failed') {
      const code = /^BLE_SCAN_[A-Z_]+$/.test(event.message || '') ? event.message : 'GATE_SCAN_FAILED';
      errors.push(code); failure?.(code);
    }
  });
  const busy = async () => {
    try { await native.start({ id: other, services }); throw Error('GATE_NOT_EXCLUSIVE'); }
    catch (error) { must(error?.message === 'BLE_SCAN_BUSY', 'GATE_EXPECTED_BUSY'); }
  };
  try {
    await native.start({ id, services });
    phase = 'provisioning-blocked';
    const blocked = new Promise((resolve, reject) => {
      failure = resolve; timer = setTimeout(() => reject(Error('GATE_SCAN_EVENT_TIMEOUT')), 5000);
    });
    await prov.startBleScan({ prefix: 'Blinker' });
    must(await blocked === 'BLE_SCAN_BUSY', 'GATE_EXPECTED_PROV_BUSY'); clearTimeout(timer); failure = undefined;
    await prov.stopBleScan(); // A non-owner must not stop Direct's native scanner.
    await busy();
    await native.stop({ id });
    phase = 'provisioning-start';
    await prov.startBleScan({ prefix: 'Blinker' });
    await busy();
    await native.stop({ id }); // An old Direct id must not stop WiFiProv.
    await busy();
    await prov.stopBleScan();
    phase = 'direct-recover';
    await native.start({ id: other, services });
    await native.stop({ id: other });
    return { directBlocksProvisioning: true, provisioningBlocksDirect: true,
      staleStopIsolated: true, released: true, scanErrors: errors };
  } catch (error) {
    return { phase, released: false, error: /^(BLE_SCAN|GATE)_[A-Z_]+$/.test(error?.message || '') ? error.message : 'GATE_UNEXPECTED' };
  } finally {
    clearTimeout(timer);
    await Promise.allSettled([native.stop({ id }), native.stop({ id: other }), prov.stopBleScan()]);
    await listener.remove();
  }
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const result = await runGatewayAppGate(scanResourceGate, { pagePath: '/home/device' }, 20_000);
  console.log(JSON.stringify(result)); assert(result.released);
}
