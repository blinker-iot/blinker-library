import assert from 'node:assert/strict';
import { pathToFileURL } from 'node:url';
import { readFile } from 'node:fs/promises';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { accountCredential, loadCloudSdk, startCloudObserver } from './edge_gateway_cloud_observer.mjs';

// Normal online page only. Wrappers observe existing promises/native calls;
// no Ready injection, direct-connect bypass, Action or account mutation.
export async function observeOnlinePage(input) {
  const must = (ok, code) => { if (!ok) throw Error(code); };
  const cancelWatchMillis = input.cancelWatchMillis ?? 1500;
  const holdMillis = input.holdMillis ?? 0;
  must(Number.isInteger(cancelWatchMillis) && cancelWatchMillis >= 1500 && cancelWatchMillis <= 8000, 'GATE_CONFIG');
  must(Number.isInteger(holdMillis) && holdMillis >= 0 && holdMillis <= 8000
    && !(holdMillis && (input.cancelBeforeReady || input.expectReadyRejected))
    && !(input.cancelBeforeReady && input.expectReadyRejected), 'GATE_CONFIG');
  const root = ng.getComponent(document.querySelector('app-root'));
  const ui = root.deviceService.deviceUi, ble = ui.ble, cap = globalThis.Capacitor;
  const idle = () => ble.accountLinks.size === 0 && ble.accountSessions.size === 0 && !ble.opening && !ble.closing
    && [...ui.connections.values()].every(g => g.scopes.size === 0);
  const context = ble.accountContext();
  const check = () => { ble.assertAccount(context); must(ui.active && !ui.network.offline && ui.deviceV2.state.value === 'ready', 'GATE_ONLINE_LOST'); };
  check(); must(idle(), 'GATE_BUSY');
  must(ui.data.getDevice(input.child)?.cloudEnabled === true && await ble.hasActiveCredential(input.child), 'GATE_CHILD_PROFILE');
  const instance = await ble.credentialDeviceInstanceId(input.child);
  try { must([...instance].map(x => x.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
  finally { instance.fill(0); }
  const started = performance.now(), events = [];
  const mark = phase => events.push({ phase, ms: Math.round(performance.now() - started) });
  const navigation = root.navCtrl.router.events.subscribe(event => {
    if (event.url === '/home/device' && event.toString().startsWith('NavigationStart')) {
      mark('navigation-away');
    }
  });
  const reserve = ui.deviceV2.reserveDirectPriority, native = cap.nativePromise;
  let ready = false, reservationRejected = false, target, connects = 0, closes = 0, readyMs, counter, physicalReleased = false, error = '', violation = '';
  ui.deviceV2.reserveDirectPriority = async function(...args) {
    must(args[0] === input.child, 'GATE_OTHER_DEVICE'); mark('reserve-start');
    const owner = await reserve.apply(this, args);
    void owner.accepted.then(() => mark('reserved'), () => { reservationRejected = true; mark('reserve-rejected'); });
    void owner.ready.then(() => { ready = true; mark('physical-ready'); }, () => mark('ready-rejected'));
    return owner;
  };
  cap.nativePromise = async function(plugin, method, options, ...rest) {
    if (plugin === 'BluetoothLe' && method === 'connect') {
      if (!ready || input.cancelBeforeReady) violation = 'GATE_CONNECT_BEFORE_READY';
      must(!target || target === options.deviceId, 'GATE_OTHER_DEVICE'); target = options.deviceId;
      ++connects; mark('sdk-connect-start');
      const value = await native.call(this, plugin, method, options, ...rest); mark('sdk-connect-done'); return value;
    }
    if (plugin === 'BluetoothLe' && method === 'disconnect' && options.deviceId === target) {
      ++closes; mark('sdk-close-start');
      const value = await native.call(this, plugin, method, options, ...rest); mark('sdk-close-done'); return value;
    }
    return native.call(this, plugin, method, options, ...rest);
  };
  const wait = async (matches, millis) => {
    const end = performance.now() + millis;
    while (!matches()) {
      check();
      must(performance.now() < end, 'GATE_PAGE_DEADLINE'); await new Promise(r => setTimeout(r, 100));
    }
  };
  try {
    await root.navCtrl.navigateRoot(`/device/${input.child}`);
    if (input.expectReadyRejected) {
      await wait(() => reservationRejected || events.some(e => e.phase === 'ready-rejected'), 31000);
      must(!ready && connects === 0, 'GATE_REJECTED_ACQUIRED'); mark('rejection-observed');
    } else if (input.cancelBeforeReady) {
      await wait(() => events.some(e => e.phase === 'reserve-start'), 3000);
      must(!ready, 'GATE_CANCEL_TOO_LATE'); mark('cancel-requested');
    } else {
      await wait(() => {
        must(!reservationRejected, 'GATE_RESERVATION_REJECTED');
        const page = [...document.querySelectorAll('app-device-v2-page')].map(e => ng.getComponent(e))
          .find(p => p.viewActive && p.device?.deviceName === input.child);
        if (!page?.direct || !page.canControl || !page.snapshot.stateFresh) return false;
        counter = page.snapshot.endpoints.find(e => e.key === 'buttonPresses')?.value;
        readyMs = Math.round(performance.now() - started); mark('page-ready'); return true;
      }, 25000);
      must(counter === input.expectedCounter, 'GATE_COUNTER');
      if (holdMillis) {
        const connected = async () => (await native.call(cap, 'BluetoothLe', 'getConnectedDevices', {}))
          .devices.some(d => d.deviceId === target);
        must(await connected(), 'GATE_HOLD_NOT_CONNECTED'); mark('hold-start');
        await new Promise(r => setTimeout(r, holdMillis)); check();
        must(await connected(), 'GATE_HOLD_DISCONNECTED'); mark('hold-finished');
      }
    }
    await root.navCtrl.navigateRoot('/home/device');
    await wait(idle, 7000); mark('page-retired');
    if (target) {
      const end = performance.now() + 4000;
      do {
        const state = await native.call(cap, 'BluetoothLe', 'getConnectedDevices', {});
        if (!state.devices.some(d => d.deviceId === target)) { physicalReleased = true; mark('os-released'); break; }
        await new Promise(r => setTimeout(r, 100));
      } while (performance.now() < end);
      must(physicalReleased, 'GATE_NATIVE_RETAINED');
    }
    if (input.cancelBeforeReady) {
      await new Promise(r => setTimeout(r, cancelWatchMillis)); check();
      must(connects === 0 && idle(), 'GATE_CANCEL_RESURRECTED');
    }
  } catch (cause) { error = /^GATE_[A-Z0-9_]+$/.test(cause?.message || '') ? cause.message : 'GATE_UNEXPECTED'; }
  finally {
    try { await root.navCtrl.navigateRoot('/home/device'); if (error) await ble.disconnect(input.child); }
    finally { navigation.unsubscribe(); ui.deviceV2.reserveDirectPriority = reserve; cap.nativePromise = native; }
  }
  return { started, finished: performance.now(), events, readyMs, counter, connects, closes, physicalReleased, idle: idle(), error: error || violation };
}

export function assertOnlinePage(result, expectedCounter) {
  assert(result.error === '' && result.idle && result.physicalReleased && result.counter === expectedCounter
    && result.connects === 1 && result.closes === 1 && result.readyMs > 0, 'GATE_ONLINE_PAGE');
  const phases = result.events.map(e => e.phase);
  const before = (a, b) => phases.includes(a) && phases.indexOf(a) < phases.indexOf(b);
  assert(before('reserved', 'physical-ready') && before('physical-ready', 'sdk-connect-start')
    && before('sdk-connect-start', 'page-ready') && before('sdk-close-done', 'os-released'), 'GATE_ONLINE_ORDER');
}

export function assertCancelledPage(result) {
  const phases = result.events.map(e => e.phase);
  assert(result.error === '' && result.idle && result.connects === 0 && result.closes === 0
    && phases.includes('cancel-requested') && phases.indexOf('navigation-away') > phases.indexOf('cancel-requested')
    && !phases.includes('sdk-connect-start'), 'GATE_CANCEL_PAGE');
}

export function onlineRounds(config) {
  const rounds = config.rounds ?? 2;
  assert([2, 10, 30, 100].includes(rounds), 'GATE_ROUNDS');
  return rounds;
}

async function main(config) {
  assert(/^ble_[A-Za-z0-9_-]+$/.test(config.child) && /^[0-9a-f]{32}$/.test(config.instance), 'GATE_CONFIG');
  const rounds = onlineRounds(config);
  const started = performance.now();
  const mark = (phase, value = {}) => console.log(JSON.stringify({ phase, ms: Math.round(performance.now() - started), ...value }));
  mark('run-start', { rounds, unixMillis: Date.now() });
  const controller = new AbortController(), timer = setTimeout(() => controller.abort(), rounds * 45000 + 30000);
  let observer;
  try {
    const sdk = await loadCloudSdk(config.appRoot);
    const credentials = await runGatewayAppGate(accountCredential, { pagePath: '/home/device' }, 15000);
    observer = await startCloudObserver(sdk, credentials, config.child, controller.signal, mark);
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_BASELINE');
    for (let round = 1; round <= rounds; ++round) {
      assert(!controller.signal.aborted, 'GATE_DEADLINE');
      const result = await runGatewayAppGate(observeOnlinePage, { ...config, pagePath: '/home/device' }, 45000);
      mark('online-page', { round, result }); assertOnlinePage(result, config.expectedCounter);
      await observer.settled();
      assert(observer.snapshot().fresh && observer.snapshot().ownerActive && observer.snapshot().counter === config.expectedCounter, 'GATE_CLOUD_RECOVERY');
      mark('cloud-restored', { round });
    }
    const result = await runGatewayAppGate(observeOnlinePage, { ...config, holdMillis: 0, cancelBeforeReady: true, pagePath: '/home/device' }, 18000);
    mark('cancel-page', { result });
    assertCancelledPage(result);
    await observer.settled();
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_CANCEL_RECOVERY');
    mark('passed');
  } finally { clearTimeout(timer); await observer?.close(); controller.abort(); mark('closed'); }
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main(JSON.parse(await readFile(process.argv[2], 'utf8'))).catch(() => {
    console.log(JSON.stringify({ phase: 'failed', code: 'GATE_ONLINE_FAILED' })); process.exitCode = 1;
  });
}
