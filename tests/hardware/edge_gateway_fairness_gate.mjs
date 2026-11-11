import assert from 'node:assert/strict';
import { execFile, spawn } from 'node:child_process';
import { promisify } from 'node:util';
import { pathToFileURL } from 'node:url';
import { readFile } from 'node:fs/promises';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { observeOfflinePage, assertOfflinePageResult } from './edge_gateway_offline_page_gate.mjs';
import { startCloudObserver, loadCloudSdk, accountCredential } from './edge_gateway_cloud_observer.mjs';

export function assertPairedEvidence(events, phone, expectedCounter) {
  assertOfflinePageResult(phone, expectedCounter);
  const accepted = events.filter(e => e.phase === 'cloud-owner-accepted');
  assert(accepted.length === 1 && !events.some(e => e.phase === 'cloud-owner-retired'), 'GATE_CONTINUOUS_OWNER');
  const entered = events.find(e => e.phase === 'phone-start');
  const lost = events.find(e => e.phase === 'cloud-fresh' && e.fresh === false && e.ms >= entered?.ms);
  const restored = events.find(e => e.phase === 'cloud-fresh' && e.fresh === true && e.ms > lost?.ms);
  assert(entered && lost && restored, 'GATE_NO_PAIRED_RECOVERY');
  assert(events.some(e => e.phase === 'hub' && e.wanted === 1 && e.secure === 1), 'GATE_NO_HUB_EXECUTION');
  assert(events.some(e => e.phase === 'hub' && e.wanted === 1 && e.childState === 0), 'GATE_NO_HUB_YIELD');
  assert(!events.some(e => e.phase === 'hub' && e.wanted === 0 && e.ms >= entered.ms), 'GATE_DEMAND_DISAPPEARED');
  const stopped = events.find(e => e.phase === 'cloud-account' && e.state === 'stopped')?.ms ?? Infinity;
  const changes = events.filter(e => e.phase === 'cloud-fresh' && e.ms >= lost.ms && e.ms < stopped);
  const cloudOutagesMillis = [], cloudUsefulWindowsMillis = [];
  for (let i = 1; i < changes.length; ++i) {
    const before = changes[i - 1], after = changes[i];
    if (before.fresh === after.fresh) continue;
    (before.fresh ? cloudUsefulWindowsMillis : cloudOutagesMillis).push(after.ms - before.ms);
  }
  return { cloudUnavailableMs: restored.ms - lost.ms, cloudOutagesMillis,
    cloudUsefulWindowsMillis, phoneReadyMs: phone.readyMs };
}

async function main(config) {
  assert(/^ble_[A-Za-z0-9_-]+$/.test(config.child) && /^[0-9a-f]{32}$/.test(config.instance)
    && config.appRoot && config.adb && config.phone && config.python && config.port, 'GATE_CONFIG');
  const execute = promisify(execFile);
  const run = async (file, args) => (await execute(file, args, { windowsHide: true, timeout: 20000, maxBuffer: 128 * 1024 })).stdout.trim();
  const adb = (...args) => run(config.adb, ['-s', config.phone, ...args]);
  const delay = ms => new Promise(r => setTimeout(r, ms));
  const started = performance.now(), events = [];
  const mark = (phase, fields = {}) => {
    const event = { phase, ms: Math.round(performance.now() - started), ...fields };
    if (events.length < 256) events.push(event);
    console.log(JSON.stringify(event));
  };
  const sdk = await loadCloudSdk(config.appRoot);
  mark('sdk-built');
  const wifi = await adb('shell', 'settings', 'get', 'global', 'wifi_on');
  const mobile = await adb('shell', 'settings', 'get', 'global', 'mobile_data');
  assert(/^[01]$/.test(wifi) && /^[01]$/.test(mobile), 'GATE_RADIO_STATE');
  const reverse = await adb('reverse', '--list');
  assert(reverse.includes('tcp:7000 tcp:17001') && reverse.includes('tcp:8884 tcp:8884'), 'GATE_REVERSE_STATE');
  let observer, serial, serialExited = false, phoneResult;
  const abort = new AbortController();
  let credentials = await runGatewayAppGate(accountCredential, { pagePath: '/home/device' }, 15000);
  mark('account-session-issued');
  const deadline = setTimeout(() => abort.abort(), 100000);
  try {
    await adb('shell', 'svc', 'wifi', 'disable'); await adb('shell', 'svc', 'data', 'disable');
    await adb('reverse', '--remove', 'tcp:7000'); await adb('reverse', '--remove', 'tcp:8884');
    await adb('shell', 'am', 'force-stop', 'iot.diandeng.tech.debug');
    await adb('shell', 'am', 'start', '-n', 'iot.diandeng.tech.debug/iot.diandeng.tech.MainActivity');
    const pid = await adb('shell', 'pidof', 'iot.diandeng.tech.debug');
    assert(/^\d+$/.test(pid), 'GATE_APP_PID');
    await adb('forward', 'tcp:9222', `localabstract:webview_devtools_remote_${pid}`);
    await delay(8000); mark('phone-offline-started');
    const script = 'import serial,sys,time,json,re\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();end=time.monotonic()+100\nwhile time.monotonic()<end:\n line=s.readline().decode("ascii","replace").strip()\n if line.startswith("childState="):\n  v=dict(re.findall(r"(\\w+)=(-?\\d+)",line));print(json.dumps({k:int(v[k]) for k in ["childState","secure","error","routeError","executionWanted","managementSubscribed","ipc0StackMin"] if k in v}),flush=True)\ns.close()';
    serial = spawn(config.python, ['-u', '-c', script, config.port], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    let buffer = '';
    serial.stdout.on('data', chunk => {
      buffer += chunk.toString(); let i;
      while ((i = buffer.indexOf('\n')) >= 0) {
        const value = JSON.parse(buffer.slice(0, i)); buffer = buffer.slice(i + 1);
        mark('hub', { ...value, wanted: value.executionWanted });
      }
    });
    serial.on('exit', () => { serialExited = true; });
    serial.on('error', () => { serialExited = true; mark('serial-failed'); });
    observer = await startCloudObserver(sdk, credentials, config.child, abort.signal, mark);
    credentials = undefined;
    assert(observer.snapshot().fresh && observer.snapshot().counter === config.expectedCounter, 'GATE_CLOUD_BASELINE');
    const hubDeadline = performance.now() + 3000;
    while (!events.some(e => e.phase === 'hub' && e.wanted === 1 && e.secure === 1)
      && performance.now() < hubDeadline) await delay(100);
    assert(events.some(e => e.phase === 'hub' && e.wanted === 1 && e.secure === 1), 'GATE_HUB_BASELINE');
    mark('phone-start', { cloud: observer.snapshot() });
    phoneResult = await runGatewayAppGate(observeOfflinePage, {
      child: config.child, instance: config.instance, expectedCounter: config.expectedCounter,
      pairedCloud: true, pagePath: '/home/device',
    }, 55000);
    mark('phone-finished', { result: phoneResult });
    assertOfflinePageResult(phoneResult, config.expectedCounter);
    await observer.settled();
    // A recovery observed just before cleanup is not a measured useful
    // window. Retain the SAME caller until one full recovery -> yield ends;
    // only observe existing events, never send work to manufacture demand.
    const windowDeadline = performance.now() + 30000;
    while (assertPairedEvidence(events, phoneResult, config.expectedCounter).cloudUsefulWindowsMillis.length === 0) {
      assert(!abort.signal.aborted && !serialExited && !observer.snapshot().fatal
        && performance.now() < windowDeadline, 'GATE_CLOUD_WINDOW_INCOMPLETE');
      await delay(100);
    }
    await observer.settled(); // Leave only after the next genuine read recovery.
    assert(!serialExited && observer.snapshot().fresh && observer.snapshot().ownerActive
      && observer.snapshot().counter === config.expectedCounter, 'GATE_CLOUD_NOT_RECOVERED');
    mark('paired-passed', assertPairedEvidence(events, phoneResult, config.expectedCounter));
  } catch (error) {
    mark('failed', { code: /^GATE_[A-Z0-9_]+$/.test(error?.message || '') ? error.message : 'GATE_UNEXPECTED',
      protocolCode: Number.isInteger(error?.code) ? error.code : undefined });
    process.exitCode = 1;
  } finally {
    credentials = undefined; clearTimeout(deadline);
    try { await observer?.close(); } catch { mark('cloud-cleanup-failed'); process.exitCode = 1; }
    abort.abort(); serial?.kill();
    if (!phoneResult) {
      try {
        await runGatewayAppGate(async input => {
          const root = ng.getComponent(document.querySelector('app-root'));
          await root.navCtrl.navigateRoot('/home/device');
          await root.deviceService.deviceUi.ble.disconnect(input.child);
          return { cleaned: true };
        }, { child: config.child, pagePath: '/' }, 15000);
      } catch { mark('phone-cleanup-unconfirmed'); process.exitCode = 1; }
    }
    // Restoration attempts are independent: ADB failure must not skip the second switch/reverse.
    for (const args of [['shell', 'svc', 'wifi', wifi === '1' ? 'enable' : 'disable'],
      ['shell', 'svc', 'data', mobile === '1' ? 'enable' : 'disable'],
      ['reverse', 'tcp:7000', 'tcp:17001'], ['reverse', 'tcp:8884', 'tcp:8884']]) {
      try { await adb(...args); } catch { mark('phone-restoration-failed'); process.exitCode = 1; }
    }
    mark('closed');
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main(JSON.parse(await readFile(process.argv[2], 'utf8'))).catch(() => {
    console.log(JSON.stringify({ phase: 'preflight-failed', code: 'GATE_PREFLIGHT' })); process.exitCode = 1;
  });
}
