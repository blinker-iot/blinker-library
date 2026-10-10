import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Run prepare, force-stop/relaunch the DEBUG App without clearing data, then
// after-restart. Run recover explicitly after the reported cooldown; no retry loop.
export async function scanRestartPhase({ phase }) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  must(['prepare', 'after-restart', 'recover'].includes(phase), 'GATE_PHASE');
  const ui = globalThis.ng.getComponent(document.querySelector('app-root')).deviceService.deviceUi;
  must(ui.active && ui.data.auth && !ui.ble.active && !ui.ble.opening && !ui.ble.presence
    && !ui.ble.accountLinks.size && !ui.ble.accountSessions.size
    && [...ui.connections.values()].every(group => !group.scopes.size), 'GATE_BUSY');
  const port = globalThis.Capacitor.Plugins.BlinkerBleScan, prefix = `restart:${crypto.randomUUID()}`, rows = [];
  let id, row, done, timer, closeFailed = false;
  const code = value => /^BLE_SCAN_[A-Z_]+$/.test(value || '') ? value : 'GATE_SCAN_FAILED';
  const listener = await port.addListener('scan', event => {
    if (event.id !== id) return;
    if (event.error && event.error !== 'BLE_SCAN_CANCELLED') {
      row.error = code(event.error); row.retryAfterMs = event.retryAfterMs; done?.();
    } else if (event.result) { ++row.advertisements; done?.(); }
  });
  try {
    for (let i = 0; i < (phase === 'prepare' ? 6 : 1); ++i) {
      id = `${prefix}:${i + 1}`;
      row = { iteration: i + 1, submitted: false, advertisements: 0, error: '', retryAfterMs: 0 }; rows.push(row);
      const observed = new Promise(resolve => { done = resolve; timer = setTimeout(resolve, 2500); });
      try {
        await port.start({ id, services: ['5f6d0001-3f5b-4e4f-9f4d-626c696e6b32'] });
        row.submitted = true; await observed;
      } catch (error) { row.error = code(error.message); }
      finally {
        clearTimeout(timer); done = undefined;
        try { await port.stop({ id }); } catch { closeFailed = true; }
      }
      if (row.error || !row.advertisements || closeFailed) break;
    }
  } finally {
    clearTimeout(timer); await listener.remove();
  }
  const last = rows.at(-1);
  return { phase, rows, closed: !closeFailed, passed: !closeFailed && (phase === 'recover'
    ? !last.error && last.advertisements > 0 : last.error === 'BLE_SCAN_THROTTLED' && !last.submitted) };
}

if (import.meta.url === pathToFileURL(process.argv[1] || '').href) {
  const result = await runGatewayAppGate(scanRestartPhase, { phase: process.argv[2], pagePath: '/home/device' }, 25_000);
  console.log(JSON.stringify(result)); assert(result.passed);
}
