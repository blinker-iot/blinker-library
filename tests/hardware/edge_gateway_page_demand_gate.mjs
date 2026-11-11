import assert from 'node:assert/strict';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// Run through gateway_app_gate in the signed-in local debug WebView. Uses the
// real page's lifecycle/Action and public UI port, never Session.owner internals.
export async function appPageDemandAction(input) {
  const must = (condition, code) => { if (!condition) throw Error(code); };
  const root = globalThis.ng?.getComponent(document.querySelector('app-root'));
  const ui = root?.deviceService?.deviceUi;
  must(ui && /^ble_[A-Za-z0-9_-]+$/.test(input.child), 'GATE_PAGE_CONTEXT');
  const device = ui.data.getDevice(input.child);
  must(device?.deviceType === 'ble' && device.cloudEnabled === true, 'GATE_CHILD_PROFILE');
  if (input.action === 'enter' || input.action === 'leave') {
    await root.navCtrl.navigateRoot(input.action === 'enter' ? `/device/${input.child}` : '/home/device');
    return { navigated: input.action };
  }
  if (input.action === 'read') {
    const started = performance.now();
    await ui.connect(input.child);
    return { synchronizedMs: Math.round(performance.now() - started) };
  }
  if (input.action === 'scope-status') {
    return { scopes: ui.connections.get(input.child)?.scopes.size || 0, active: ui.active };
  }
  if (input.action === 'start-session') {
    await ui.deviceV2.start();
    return { started: true };
  }
  const pages = [...document.querySelectorAll('app-device-v2-page')]
    .map(element => globalThis.ng.getComponent(element))
    .filter(page => page?.device?.deviceName === input.child && page.viewActive);
  must(pages.length === 1, 'GATE_EXACT_VISIBLE_PAGE');
  const page = pages[0];
  const snapshot = () => ({ canControl: page.canControl, visible: page.pageVisible,
    appActive: page.appActive, direct: page.direct, synchronized: page.synchronized,
    connectionScope: !!page.connection, fresh: page.snapshot.stateFresh,
    counter: page.snapshot.endpoints.find(field => field.key === 'buttonPresses')?.value,
    error: !!page.error });
  if (input.action === 'status') return snapshot();
  if (input.action === 'end-session') {
    const scope = page.connection;
    must(scope && !scope.signal.aborted, 'GATE_PAGE_SCOPE');
    await ui.deviceV2.stop();
    await scope.closed;
    return { retired: scope.signal.aborted, ...snapshot() };
  }
  must(input.action === 'button' && page.canControl, 'GATE_NOT_CONTROLLABLE');
  const field = page.snapshot.endpoints.find(field => field.key === 'button' && field.role === 'action');
  must(field?.valueType === 'null', 'GATE_BUTTON');
  const before = snapshot().counter;
  must(Number.isInteger(before), 'GATE_COUNTER');
  await page.execute(field); // Exactly one user action; never retry an unknown result.
  return { before, ...snapshot(), commandError: !!page.fieldError(field) };
}

// Hardware adapters are injected so sequence/fail-stop rules can be tested
// without a phone. The real runner never fabricates visibility or demand owners.
export async function runPageDemandScenario(io) {
  const until = async (read, matches, timeout = 15000) => {
    const deadline = performance.now() + timeout;
    do {
      const state = await read();
      if (matches(state)) return state;
      await new Promise(resolve => setTimeout(resolve, 250));
    } while (performance.now() < deadline);
    throw Error('GATE_WAIT_DEADLINE');
  };
  const pageReady = () => until(() => io.app('status'), s => s.canControl && s.fresh && !s.direct);
  // A replaced page may leave a cancelled remote owner until its original
  // lease expires if release was lost. Require exact eventual aggregation.
  const owners = async (count, timeout = 40000) => {
    const state = await until(() => io.broker('status'), s => {
      assert(s.sameMqtt && s.ready && !s.closed && s.session === s.initialSession, 'GATE_ROUTE_CHANGED');
      return s.demandOwners === count;
    }, timeout);
    io.mark('owners', { count, counter: state.buttonPresses });
    return state;
  };
  await io.app('enter');
  const before = await pageReady();
  await owners(1);
  await io.serialWanted(1);
  const action = await io.app('button'); // No retry, even on unknown/timeout.
  assert(!action.commandError && action.counter === before.counter + 1, 'GATE_ACTION_RESULT');
  io.mark('button', action);
  const leave = async () => {
    await io.app('leave');
    // Client cancellation is immediate; delivery is best-effort with the
    // existing server TTL as fallback. Never call a retained local scope idle.
    await until(() => io.app('scope-status'), s => s.scopes === 0 && s.active);
    await owners(0);
  };
  await leave(); await io.serialWanted(0);
  await io.app('enter'); await pageReady(); await owners(1);
  io.mark('background-requested', {});
  const backgroundAt = performance.now();
  await io.background();
  // Native suspension can prevent any JS release. Observe the existing 30 s
  // server lease instead of requiring a best-effort packet within 15 s.
  await owners(0, 40000);
  const ownerZeroMs = Math.round(performance.now() - backgroundAt);
  await io.serialWanted(0);
  io.mark('background-released', { ownerZeroMs, observedMs: Math.round(performance.now() - backgroundAt) });
  await io.foreground(); await pageReady(); await owners(1); await io.serialWanted(1);
  const retired = await io.app('end-session');
  assert(retired.retired && !retired.canControl, 'GATE_SESSION_FENCE');
  await owners(0); await io.serialWanted(0);
  io.mark('session-retired', retired);
  await io.app('start-session'); await pageReady(); await owners(1);
  await leave();
  io.mark('one-shot-read', await io.app('read'));
  await owners(0);
  // A just-completed read can race its in-flight positive decision. Wait for
  // the final zero, not an old serial zero sampled before that grant arrived.
  await until(() => io.broker('status'), s => s.demandOwners === 0
    && s.events.filter(e => e.phase === 'down' && e.execution && e.operation === 2).at(-1)?.leaseMillis === 0);
  await io.serialWanted(0);
  const after = await owners(0);
  assert.equal(after.events.filter(e => e.phase === 'down' && e.execution && e.operation === 2).at(-1)?.leaseMillis,
    0, 'GATE_NO_FINAL_RELEASE');
  assert.equal(after.buttonPresses, before.counter + 1, 'GATE_UNEXPECTED_ACTION');
  const commands = after.events.filter(e => e.phase === 'down' && e.kind === 0x12);
  assert(commands.length > 0 && new Set(commands.map(e => e.digest)).size === 1, 'GATE_EXTRA_COMMAND');
  assert(after.events.some(e => e.phase === 'accepted' && e.kind === 0x14), 'GATE_NO_ACK');
  assert(after.events.some(e => e.phase === 'accepted' && e.kind === 0x11), 'GATE_NO_PATCH');
  io.mark('passed', { before: before.counter, after: after.buttonPresses, broker: after });
}

async function main() {
  const config = JSON.parse(process.argv[2] || '{}');
  assert(config.hub && config.child && config.tenant && config.instance && config.container
    && config.port && config.python && config.adb && config.androidSerial, 'GATE_CONFIG');
  const execute = promisify(execFile);
  const run = async (file, args, timeout = 20000) => (await execute(file, args,
    { timeout, windowsHide: true, maxBuffer: 128 * 1024 })).stdout;
  const remote = '/tmp/edge_gateway_route_recovery_gate.cjs';
  const broker = async action => JSON.parse(await run('docker', ['exec', config.container, 'node', remote,
    action, JSON.stringify({ ...config, fault: 'observe', durationMillis: 180000 })]));
  const app = action => runGatewayAppGate(appPageDemandAction, { ...config, pagePath: '/', action }, 45000);
  const adb = (...args) => run(config.adb, ['-s', config.androidSerial, ...args]);
  const mark = (phase, value) => console.log(JSON.stringify({ phase, ...value }));
  const foreground = async () => {
    await adb('shell', 'am', 'start', '-n', 'iot.diandeng.tech.debug/iot.diandeng.tech.MainActivity');
    const pid = (await adb('shell', 'pidof', 'iot.diandeng.tech.debug')).trim();
    assert(/^\d+$/.test(pid), 'GATE_APP_PID');
    await adb('forward', 'tcp:9222', `localabstract:webview_devtools_remote_${pid}`);
  };
  try {
    // USB reattachment clears these mappings. Local HTTP never contains Hub credentials.
    await adb('reverse', 'tcp:7000', 'tcp:17001');
    await adb('reverse', 'tcp:8884', 'tcp:8884');
    await foreground();
    await run('docker', ['exec', config.container, 'node', '-e', 'process.kill(1,"SIGUSR1")']);
    await run('docker', ['cp', fileURLToPath(new URL('./edge_gateway_route_recovery_gate.cjs', import.meta.url)), `${config.container}:${remote}`]);
    await broker('start');
    await runPageDemandScenario({ app, broker, mark, foreground,
      background: () => adb('shell', 'input', 'keyevent', 'KEYCODE_HOME'),
      serialWanted: async wanted => {
        const script = 'import serial,sys,time\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();s.reset_input_buffer();end=time.monotonic()+14\nwhile time.monotonic()<end:\n v=s.readline().decode("utf8","replace").strip()\n if v.startswith("childState=") and " executionWanted="+sys.argv[2]+" " in v: print(v);break\ns.close()';
        const line = (await run(config.python, ['-c', script, config.port, String(wanted)])).trim();
        assert(line.includes(` executionWanted=${wanted} `) && line.includes(' secure=1 ')
          && line.includes(' routeError=0 '), 'GATE_HUB_DEMAND_NOT_OBSERVED');
        mark('hub-observed', { wanted, diagnostic: line });
      },
    });
  } catch (error) {
    mark('failed', { code: /^GATE_[A-Z_]+$/.test(error?.message || '') ? error.message : 'GATE_DEPENDENCY',
      broker: await broker('status').catch(() => null) });
    process.exitCode = 1;
  } finally {
    await foreground().catch(() => undefined);
    await app('leave').catch(() => undefined);
    await broker('close').catch(() => undefined);
    await broker('disable-inspector').catch(() => undefined);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(() => { console.error('Page gate stopped; inspect evidence before retrying the Action.'); process.exitCode = 1; });
}
