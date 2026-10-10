import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { resolve } from 'node:path';
import { createHash } from 'node:crypto';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// One authorized Direct baseline -> ordinary Cloud page -> scan-only recovery.
// Rotating advertisements are authenticated with the App's production codec,
// never matched by MAC or by a previously observed nonce/tag byte string.
export async function observeRadioRecovery(input, protocol) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  must(input.hubIsolated === true && /^ble_[A-Za-z0-9_-]+$/.test(input.child)
    && /^[a-f0-9]{32}$/.test(input.instance), 'GATE_INPUT');
  const root = globalThis.ng.getComponent(document.querySelector('app-root')), ui = root.deviceService.deviceUi, ble = ui.ble;
  must(ui.active && ui.data.auth && !ble.presence && !ble.active && !ble.opening && !ble.accountLinks.size && !ble.accountSessions.size
    && [...ui.connections.values()].every(group => !group.scopes.size), 'GATE_BUSY');
  const context = ble.accountContext(), instance = await ble.credentialDeviceInstanceId(input.child);
  try { must([...instance].map(x => x.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  const abort = new AbortController(), timers = new Set();
  const check = () => { abort.signal.throwIfAborted(); ble.assertAccount(context); must(ui.active, 'GATE_BACKGROUND'); };
  const bounded = (work, ms) => new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(Error('GATE_WAIT_DEADLINE')), ms); timers.add(timer);
    Promise.resolve(work).then(resolve, reject).finally(() => { clearTimeout(timer); timers.delete(timer); });
  });
  const native = globalThis.Capacitor.Plugins.BlinkerBleScan, id = `radio:${crypto.randomUUID()}`;
  let credential, matching, handle, registered, advertisement, scanError, released = false, releaseAt, seenAt, stopConfirmed = false, phase = 'prime';
  const events = [];
  try {
    try {
      await bounded(ble.ensureReady(input.child, 5000, context, { signal: abort.signal, assertAcquire: check, assertActive: check }), 12_000);
      check(); must(ble.active?.session.state === 'ready' && ble.active.logicalDeviceId === input.child, 'GATE_PRIME');
    } finally { await ble.disconnect(input.child); }
    credential = await ble.credentialStore(context).load(input.child);
    // Only PresenceKey is needed for scan-only matching. No credential leaves
    // this WebView invocation, and the controller secret is discarded now.
    credential?.controllerSecret.fill(0);
    check(); must(credential?.state === 'active' && credential.logicalDeviceId === input.child
      && [...credential.deviceInstanceId].map(x => x.toString(16).padStart(2, '0')).join('') === input.instance
      && credential.presenceKeys?.length, 'GATE_PRESENCE_CREDENTIAL');
    must(!ble.accountLinks.size && !ble.accountSessions.size, 'GATE_DIRECT_CLOSE');
    phase = 'cloud';
    await root.navCtrl.navigateRoot(`/device/${input.child}`);
    const deadline = performance.now() + 15_000;
    let page;
    while (performance.now() < deadline) {
      check(); const node = document.querySelector('app-device-v2-page'); page = node && globalThis.ng.getComponent(node);
      if (page?.snapshot?.stateFresh && page.canControl && !page.direct) break;
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    must(page?.snapshot?.stateFresh && page.canControl && !page.direct, 'GATE_CLOUD_READY');
    const counter = page.snapshot.endpoints.find(e => e.key === 'buttonPresses')?.value;
    const firstNativeResult = new Promise(resolve => { registered = resolve; });
    const recovered = new Promise(resolve => { advertisement = resolve; });
    phase = 'native-start';
    handle = await native.addListener('scan', event => {
      if (event.id !== id) return;
      if (event.error && event.error !== 'BLE_SCAN_CANCELLED') { scanError = /^BLE_SCAN_[A-Z_]+$/.test(event.error) ? event.error : 'GATE_SCAN_FAILED'; registered?.(); advertisement?.(); }
      if (!event.result) return;
      registered?.(); // An actual SDK result, not just start() resolve.
      const value = Object.entries(event.result.serviceData || {}).find(([uuid]) => uuid.toLowerCase() === '5f6d0001-3f5b-4e4f-9f4d-626c696e6b32')?.[1];
      if (typeof value !== 'string' || !/^[a-fA-F0-9]{26}$/.test(value) || seenAt !== undefined) return;
      let profile;
      try { profile = protocol.decodeBleModeProfile(Uint8Array.from(value.match(/../g), x => parseInt(x, 16))); }
      catch { return; } // Malformed/bystander advertisements are not identity.
      if (profile.wireVersion !== 3 || profile.mode !== 2) return;
      if (matching) { scanError = 'GATE_MATCHER_BUSY'; advertisement?.(); return; }
      const observedAt = performance.now(), wasReleased = released;
      matching = (async () => {
        for (const presence of credential.presenceKeys) {
          check();
          const matches = await protocol.matchesBlePresenceLocator(presence.key, {
            deviceInstanceId: credential.deviceInstanceId, accessEpoch: presence.accessEpoch, presenceKeyVersion: presence.version,
          }, profile.modeLocator);
          check(); if (!matches) continue;
          if (!wasReleased) scanError = 'GATE_ADVERTISING_WHILE_HELD';
          else seenAt = observedAt; // Arrival, not asynchronous HMAC completion.
          advertisement?.(); break;
        }
      })().catch(() => { scanError = 'GATE_MATCH_FAILED'; advertisement?.(); })
        .finally(() => { profile.modeLocator.fill(0); matching = undefined; });
    });
    await native.start({ id, services: [] });
    phase = 'native-observation';
    await bounded(firstNativeResult, 3000); check(); must(!scanError, scanError);
    events.push({ phase: 'scanner-observed', at: performance.now() });
    released = true; releaseAt = performance.now();
    phase = 'release';
    await root.navCtrl.navigateRoot('/home/device');
    phase = 'advertisement';
    await bounded(recovered, 15_000); check(); must(!scanError && seenAt !== undefined, scanError || 'GATE_ADVERTISEMENT');
    await native.stop({ id }); stopConfirmed = true;
    must(!ble.accountLinks.size && !ble.accountSessions.size && [...ui.connections.values()].every(group => !group.scopes.size), 'GATE_SCOPE_LEAK');
    return { releaseAt, seenAt, pageToAdvertisementMs: seenAt - releaseAt, counter, events, closed: true };
  } catch (error) {
    return { closed: false, phase, error: /^(BLE_SCAN|BLE_DIRECT|GATE)_[A-Z0-9_]+$/.test(error?.message || '') ? error.message : 'GATE_UNEXPECTED', events };
  } finally {
    abort.abort();
    for (const timer of timers) clearTimeout(timer);
    try { await ble.disconnect(input.child); }
    finally {
      try { if (!stopConfirmed) await native.stop({ id }); }
      finally {
        try { await handle?.remove(); await matching; }
        finally {
          for (const presence of credential?.presenceKeys ?? []) presence.key.fill(0);
          credential?.controllerSecret.fill(0); credential?.deviceInstanceId.fill(0);
          await root.navCtrl.navigateRoot('/home/device');
        }
      }
    }
  }
}

// Request/reply bounds: never subtract unsynchronized phone and MCU clocks.
export function clockOffset(samples) {
  assert(samples.length >= 2, 'GATE_CLOCK_SAMPLES');
  const lower = Math.max(...samples.map(s => s.start - s.local));
  const upper = Math.min(...samples.map(s => s.end - s.local));
  assert(Number.isFinite(lower) && Number.isFinite(upper) && lower <= upper, 'GATE_CLOCK_INCONSISTENT');
  return { lower, upper };
}
export function elapsedBounds(later, laterClock, earlier, earlierClock) {
  return { minMs: later + laterClock.lower - earlier - earlierClock.upper,
    maxMs: later + laterClock.upper - earlier - earlierClock.lower };
}

async function main() {
  const config = JSON.parse(await readFile(process.argv[2], 'utf8'));
  // Compile existing pure production modules in memory. Do not add a debug
  // API to the App or copy the cryptography into this hardware tool.
  const appRoot = resolve(config.appRoot || '../blinker-app');
  const { build } = await import(pathToFileURL(resolve(appRoot, 'node_modules/esbuild/lib/main.js')).href);
  const bundle = await build({ stdin: { contents: "export { matchesBlePresenceLocator } from './presence'; export { decodeBleModeProfile } from './wire';",
    resolveDir: resolve(appRoot, 'src/app/core/device-v2/ble-direct') }, bundle: true, write: false,
    format: 'iife', globalName: 'radioProtocol', platform: 'browser', minify: true });
  const appAction = new Function('input', `${bundle.outputFiles[0].text}\nreturn (${observeRadioRecovery.toString()})(input, radioProtocol);`);
  console.log(JSON.stringify({ phase: 'production-codec', sha256: createHash('sha256').update(bundle.outputFiles[0].text).digest('hex') }));
  assert(/^COM\d+$/.test(config.port) && config.python, 'GATE_SERIAL_CONFIG');
  const records = [], waiters = new Set();
  const script = 'import serial,sys,threading\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.03;s.dtr=False;s.rts=False;s.open()\ndone=threading.Event()\ndef commands():\n for line in sys.stdin:\n  if line.strip()=="clock":s.write(b"t")\n  elif line.strip()=="quit":done.set();break\nthreading.Thread(target=commands,daemon=True).start()\nwhile not done.is_set():\n line=s.readline().decode("utf8","replace").strip()\n if line.startswith(("radioProbe=","childState=")):print(line,flush=True)\ns.close()';
  const serial = spawn(config.python, ['-u', '-c', script, config.port], { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
  let pending = '', failed = false, baseUs;
  serial.on('error', () => { failed = true; }); serial.stderr.on('data', () => { failed = true; });
  serial.stdout.on('data', bytes => {
    pending += bytes.toString();
    for (;;) {
      const end = pending.indexOf('\n'); if (end < 0) break;
      const line = pending.slice(0, end).trim(); pending = pending.slice(end + 1);
      const values = Object.fromEntries([...line.matchAll(/([a-zA-Z0-9_]+)=([a-zA-Z0-9_]+)/g)].map(m => [m[1], /^\d+$/.test(m[2]) ? Number(m[2]) : m[2]]));
      if (records.length >= 128) { failed = true; break; }
      const row = { ...values, received: performance.now() }; records.push(row);
      for (const waiter of [...waiters]) if (waiter.match(row)) waiter.resolve(row);
    }
  });
  const wait = (match, ms) => new Promise((resolve, reject) => {
    const waiter = { match, resolve: row => { clearTimeout(timer); waiters.delete(waiter); resolve(row); } };
    const timer = setTimeout(() => { waiters.delete(waiter); reject(Error('GATE_SERIAL_DEADLINE')); }, ms); waiters.add(waiter);
  });
  const mcuTime = us => ((us - baseUs) >>> 0) / 1000;
  const mcuSamples = [], phoneSamples = [];
  const sync = async () => {
    for (let i = 0; i < 2; ++i) {
      const start = performance.now(), response = wait(row => row.radioProbe === 'clock', 2000);
      serial.stdin.write('clock\n'); const row = await response;
      baseUs ??= row.us; mcuSamples.push({ start, end: row.received, local: mcuTime(row.us) });
      const before = performance.now();
      const local = await runGatewayAppGate(() => performance.now(), { ...config, pagePath: '/home/device' }, 5000);
      phoneSamples.push({ start: before, end: performance.now(), local });
    }
  };
  try {
    // Existing normal diagnostics still validate actual operator isolation.
    const idle = await wait(row => row.childState === 0 && row.executionWanted === 0 && row.secure === 0 && row.managementSubscribed === 1, 12_000);
    assert(idle.permitState === 0 && !idle.revokeActive && !idle.controlPending, 'GATE_MANAGEMENT_BUSY');
    await sync(); const started = performance.now();
    const app = await runGatewayAppGate(appAction, { ...config, pagePath: '/home/device' }, 50_000);
    if (!app.closed) { console.log(JSON.stringify({ phase: 'app-failed', app })); throw Error(app.error); }
    const endIdle = records.findLast(row => row.radioProbe === 'idle' && row.received >= started)
      || await wait(row => row.radioProbe === 'idle', 5000);
    await sync(); assert(!failed && !endIdle.dropped, 'GATE_SERIAL_INVALID');
    const gap = records.find(row => row.radioProbe === 'gap_disconnected' && row.seq === endIdle.seq && row.received >= started);
    assert(gap && records.some(row => row.radioProbe === 'connected' && row.seq === gap.seq && row.received >= started), 'GATE_NO_SDK_CLOSE');
    assert(!records.some(row => row.radioProbe === 'overlap'), 'GATE_RADIO_OVERLAP');
    const phone = clockOffset(phoneSamples), mcu = clockOffset(mcuSamples);
    const pageToGap = elapsedBounds(mcuTime(gap.us), mcu, app.releaseAt, phone);
    assert(pageToGap.maxMs >= 0, 'GATE_DISCONNECTED_BEFORE_RELEASE');
    console.log(JSON.stringify({ app, sdk: { gapToIdleMs: endIdle.gapUs / 1000, sampleSpanMs: endIdle.sampleSpanUs / 1000,
      sampleCostMs: endIdle.sampleCostUs / 1000 }, clockWidthMs: { phone: phone.upper - phone.lower, mcu: mcu.upper - mcu.lower },
      pageToGap, gapToAdvertisement: elapsedBounds(app.seenAt, phone, mcuTime(gap.us), mcu),
      idleToAdvertisement: elapsedBounds(app.seenAt, phone, mcuTime(endIdle.us), mcu) }));
  } finally {
    console.log(JSON.stringify({ phase: 'radio-clock-evidence', baseUs, mcuSamples, phoneSamples,
      radio: records.filter(row => row.radioProbe), serialFailed: failed }));
    serial.stdin.end('quit\n');
    const kill = setTimeout(() => serial.kill(), 1000); serial.once('exit', () => clearTimeout(kill));
  }
}
if (import.meta.url === pathToFileURL(process.argv[1] || '').href) await main();
