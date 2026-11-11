import assert from 'node:assert/strict';
import { execFile, spawn } from 'node:child_process';
import { promisify } from 'node:util';
import { createInterface } from 'node:readline';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';

// No cross-clock subtraction: all end-to-end marks use the host's monotonic
// clock. Broker/App durations are separate observations, never clock offsets.
export async function measureColdStart(io) {
  const marks = [];
  let origin;
  const mark = (phase, fields = {}) => {
    if (origin !== undefined && marks.length < 64) marks.push({ ms: Math.round(io.now() - origin), phase, ...fields });
  };
  try {
    await io.prepare();
    await io.openSerial(mark);
    const waiting = io.waitReady(); // Observer only; no credential or protocol mutation.
    // Observe rejection immediately even if the serial trigger fails first.
    void waiting.catch(() => undefined);
    origin = io.now(); mark('trigger');
    await io.trigger();
    const broker = await waiting;
    mark('broker-observed', { result: broker.waitResult });
    assert(broker.sameMqtt && broker.waitResult === 'ready' && broker.ready
      && broker.session !== broker.initialSession, 'GATE_COLD_NOT_READY');
    mark('app-control-start');
    const app = await io.control();
    mark('app-control-result');
    assert.equal(app.after, app.before + 1, 'GATE_COUNTER');
    return { marks, broker, app };
  } finally { await io.close(); }
}

// Executed only in the signed-in, exact Hub debug page. Uses the production
// account client, not permit-join or a copied device MQTT credential.
export async function appColdAction(input) {
  const page = globalThis.ng?.getComponent(document.querySelector('app-edge-gateway-enrollment'));
  const must = (condition, code) => { if (!condition) throw Error(code); };
  must(page?.available && !page.busy && !page.workflow && page.edgeHubLogicalDeviceId === input.hub, 'GATE_PAGE');
  const app = globalThis.ng.getComponent(document.querySelector('app-root'));
  const cloud = app.deviceService.deviceUi.deviceV2, ble = page.gateway.ble;
  must(!ble.accountLinks.size && !ble.accountSessions.size, 'GATE_DIRECT_BUSY');
  if (input.action === 'close') {
    globalThis.__edgeColdObservation?.stop(); delete globalThis.__edgeColdObservation;
    await cloud.stop(); return { closed: true };
  }
  if (input.action === 'prepare') {
    must(!(await page.gateway.recoveries(input.hub)).length
      && !(await page.gateway.accountPermitJoinCheckpoints(page.gateway.accountContext()).list()).length, 'GATE_PENDING');
    const instance = await ble.credentialDeviceInstanceId(input.child);
    try { must([...instance].map(x => x.toString(16).padStart(2, '0')).join('') === input.instance, 'GATE_INSTANCE'); }
    finally { instance.fill(0); }
    await cloud.ensureReady(input.child);
    await cloud.client.session.subscribePresence(input.child);
    await cloud.client.session.refresh(input.child);
    globalThis.__edgeColdObservation?.stop();
    const started = performance.now(), observations = [];
    let offlineReadiness, readyWait;
    const waitControllers = [];
    const stop = cloud.store.subscribe((id, state) => {
      if (id !== input.child || observations.length >= 32) return;
      const value = { reachable: state.cloudReachable, fresh: state.stateFresh, accepted: state.manifestAccepted };
      const previous = observations.at(-1);
      if (previous && Object.keys(value).every(key => previous[key] === value[key])) return;
      observations.push({ ms: Math.round(performance.now() - started), ...value });
      // Readiness only: never issue a test Action while the target is offline.
      if (state.cloudReachable === false && !offlineReadiness) {
        offlineReadiness = cloud.client.session.ensureReady(input.child).then(
          () => ({ rejected: false }),
          error => ({ rejected: error?.code === 'DEVICE_V2_TARGET_UNREACHABLE' }),
        );
        if (input.readyWait) {
          const started = performance.now();
          const cancelled = new AbortController(), retained = new AbortController();
          waitControllers.push(cancelled, retained);
          const first = cloud.waitUntilReady(input.child, cancelled.signal).then(
            () => ({ cancelled: false }), error => ({ cancelled: error?.code === 'DEVICE_V2_READY_CANCELLED' }),
          );
          const second = cloud.waitUntilReady(input.child, retained.signal).then(
            () => ({ ready: true, elapsedMs: Math.round(performance.now() - started) }),
            error => ({ ready: false, timeout: error?.code === 'DEVICE_V2_READY_TIMEOUT',
              reason: typeof error?.code === 'number' ? error.code
                : /^DEVICE_V2_READY_[A-Z]+$/.test(error?.code || '') ? error.code
                : error?.message === 'StatePage does not continue the active transfer' ? 'STATE_TRANSFER_RETIRED' : 'READ_FAILED',
              elapsedMs: Math.round(performance.now() - started) }),
          );
          cancelled.abort(); // Another caller must retain the same wait.
          readyWait = Promise.all([first, second]).then(([one, two]) => ({ ...one, ...two }));
        }
      }
    });
    globalThis.__edgeColdObservation = {
      stop: () => { stop(); waitControllers.forEach(controller => controller.abort()); },
      observations, offlineCheck: () => offlineReadiness, readyCheck: () => readyWait,
    };
    return { prepared: true, before: cloud.snapshot(input.child).values.buttonPresses?.value };
  }
  if (input.action === 'runtime-command') {
    const ack = await cloud.command(input.child, 'button', null);
    return { ack: ack.acknowledgedSequence };
  }
  if (input.action === 'ready-status') return await globalThis.__edgeColdObservation?.readyCheck() || { missing: true };
  if (input.action === 'ingress-command') {
    // Exactly one new Action. Neither this helper nor the user starts another
    // Action on timeout; production exact retransmissions remain unchanged.
    try {
      await cloud.command(input.child, 'button', null);
      return { acknowledged: true };
    } catch (error) {
      return { acknowledged: false, code: error?.code === 'DEVICE_V2_COMMAND_OUTCOME_UNKNOWN'
        ? error.code : Number.isInteger(error?.code) ? error.code : null };
    }
  }
  if (input.action === 'offline-verify') {
    const wait = await globalThis.__edgeColdObservation?.readyCheck();
    if (input.readyWait) must(wait?.cancelled && wait.timeout && !wait.ready
      && wait.elapsedMs >= 29_000 && wait.elapsedMs < 35_000, 'GATE_READY_DEADLINE');
    try { await cloud.ensureReady(input.child); return { rejected: false, wait }; }
    catch (error) { return { rejected: error?.code === 'DEVICE_V2_TARGET_UNREACHABLE', wait }; }
  }
  if (input.action === 'ready-verify') await cloud.ensureReady(input.child);
  if (input.action === 'runtime-verify' || input.action === 'ready-verify') {
    await cloud.client.session.refresh(input.child);
    return { after: cloud.snapshot(input.child).values.buttonPresses?.value };
  }
  must(input.action === 'control', 'GATE_ACTION');
  const started = performance.now();
  const wait = await globalThis.__edgeColdObservation?.readyCheck();
  if (input.readyWait) must(wait?.cancelled && wait.ready && wait.elapsedMs < 30_000, 'GATE_SHARED_READY');
  await cloud.ensureReady(input.child);
  await cloud.client.session.refresh(input.child);
  const snapshot = () => cloud.snapshot(input.child);
  const before = snapshot().values.buttonPresses.value;
  must(Number.isInteger(before) && snapshot().cloudReachable === true, 'GATE_COUNTER_BASELINE');
  const readyMs = Math.round(performance.now() - started);
  const offlineReadiness = await globalThis.__edgeColdObservation?.offlineCheck();
  must(offlineReadiness?.rejected, 'GATE_OFFLINE_READINESS');
  const ack = await cloud.command(input.child, 'button', null);
  await cloud.client.session.refresh(input.child);
  return { before, after: snapshot().values.buttonPresses.value, ack: ack.acknowledgedSequence,
    readyMs, elapsedMs: Math.round(performance.now() - started), offlineReadiness, wait,
    observations: globalThis.__edgeColdObservation?.observations || [] };
}

async function main() {
  const config = JSON.parse(process.argv[2] || '{}');
  assert(config.hub && config.child && config.tenant && config.instance && config.port && config.python && config.container);
  assert(Number.isInteger(config.count) && config.count >= 1 && config.count <= 3, 'GATE_COUNT');
  assert(['observe', 'closure-receipt', 'manifest', 'runtime-state', 'ingress', 'bootstrap-terminal'].includes(config.fault || 'observe'), 'GATE_FAULT');
  assert(!['runtime-state', 'ingress', 'bootstrap-terminal'].includes(config.fault) || config.count === 1, 'GATE_RUNTIME_COUNT');
  const execute = promisify(execFile);
  const run = async (file, args, timeout = 55000) => (await execute(file, args, { timeout, windowsHide: true, maxBuffer: 128 * 1024 })).stdout;
  const remote = '/tmp/edge_gateway_route_recovery_gate.cjs';
  // Progress contains only fixed phase names, never exception/HTTP payloads.
  // Keep evidence even when a later cleanup failure masks the original error.
  const brokerCall = async action => {
    console.log(JSON.stringify({ phase: `broker-${action}` }));
    return JSON.parse(await run('docker', ['exec', config.container, 'node', remote, action,
      JSON.stringify({ ...config, fault: config.fault === 'ingress' ? 'observe' : config.fault || 'observe' })]));
  };
  const appCall = action => {
    console.log(JSON.stringify({ phase: `app-${action}` }));
    return runGatewayAppGate(appColdAction, { ...config, action }, 45000);
  };
  const script = fileURLToPath(new URL('./edge_gateway_route_recovery_gate.cjs', import.meta.url));
  let serial, lines;
  try {
    await run('docker', ['exec', config.container, 'node', '-e', 'process.kill(1,"SIGUSR1")']);
    await run('docker', ['cp', script, `${config.container}:${remote}`]);
    if (config.fault === 'bootstrap-terminal') {
      await appCall('prepare');
      await brokerCall('start');
      await brokerCall('recover');
      // Deliberately outlive both 11 s exchanges and the 1 s re-admission.
      // The extra observation verifies there is no third automatic attempt.
      const broker = await brokerCall('waitReady');
      const requests = broker.events.filter(value => value.phase === 'drop-down' && value.kind === 2);
      const digests = [...new Set(requests.map(value => value.digest))];
      assert(broker.sameMqtt && !broker.ready && broker.waitResult === 'deadline', 'GATE_NOT_TERMINAL');
      assert(digests.length === 2 && digests.every(digest => requests.filter(value => value.digest === digest).length === 4), 'GATE_RETRY_BOUND');
      assert(!broker.events.some(value => value.phase === 'down' && value.kind === 18), 'GATE_UNEXPECTED_COMMAND');
      const app = await appCall('offline-verify');
      assert(app.rejected, 'GATE_OFFLINE_READINESS');
      console.log(JSON.stringify({ fault: config.fault, app, broker }));
      return;
    }
    if (config.fault === 'ingress') {
      const prepared = await appCall('prepare');
      assert(Number.isInteger(prepared.before), 'GATE_COUNTER_BASELINE');
      await brokerCall('start');
      // Only the explicitly compiled internal receiver-capacity probe accepts
      // q. No SDK memory writes, secret changes or production fault endpoint.
      const marks = [];
      let armedResolve, releaseResolve, rejectProbe;
      const armed = new Promise(resolve => { armedResolve = resolve; });
      const release = new Promise((resolve, reject) => { releaseResolve = resolve; rejectProbe = reject; });
      void release.catch(() => undefined);
      const timeout = setTimeout(() => rejectProbe(Error('GATE_INGRESS_DEADLINE')), 20000);
      const program = 'import serial,sys\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();s.reset_input_buffer();s.write(b"q")\nwhile True:\n v=s.readline().decode("utf8","replace").strip()\n if v.startswith("ingress="):print(v,flush=True)';
      serial = spawn(config.python, ['-u', '-c', program, config.port], { windowsHide: true });
      lines = createInterface({ input: serial.stdout });
      serial.once('error', () => rejectProbe(Error('GATE_SERIAL')));
      serial.once('exit', () => rejectProbe(Error('GATE_SERIAL_EXIT')));
      lines.on('line', value => {
        if (marks.length < 32) marks.push(value);
        if (value === 'ingress=armed') armedResolve();
        if (value.startsWith('ingress=released')) releaseResolve();
      });
      try {
        await Promise.race([armed, release.then(() => { throw Error('GATE_NOT_ARMED'); })]);
        const action = await appCall('ingress-command');
        await release;
        assert.equal(action.acknowledged, false, 'GATE_UNACCEPTED_ACTION_ACKED');
        const expirations = marks.filter(value => value.startsWith('ingress=expired'));
        assert(expirations.length > 0 && expirations.every(value => {
          const held = /heldMs=(\d+)/.exec(value);
          return held && Number(held[1]) >= 950 && Number(held[1]) < 1300;
        }), 'GATE_INGRESS_EXPIRY');
        const broker = await brokerCall('status');
        const app = await appCall('runtime-verify');
        assert(broker.sameMqtt && broker.ready && broker.session === broker.initialSession, 'GATE_INGRESS_SESSION');
        assert.equal(app.after, prepared.before, 'GATE_UNACCEPTED_ACTION_EXECUTED');
        console.log(JSON.stringify({ fault: 'ingress', before: prepared.before, action, marks, app, broker }));
      } finally { clearTimeout(timeout); }
      return;
    }
    if (config.fault === 'runtime-state') {
      const prepared = await appCall('prepare');
      assert(Number.isInteger(prepared.before), 'GATE_COUNTER_BASELINE');
      await brokerCall('start');
      const first = await appCall('runtime-command');
      // Reports are intentionally coalesced by the device. Observe the first
      // lost Patch before a second Action, otherwise there is no revision gap.
      const gap = await brokerCall('waitPatch');
      assert(gap.sameMqtt && gap.patchDropped && gap.waitResult === 'ready', 'GATE_PATCH_GAP');
      const second = await appCall('runtime-command');
      const broker = await brokerCall('waitReady');
      assert(broker.sameMqtt && broker.ready && broker.waitResult === 'ready'
        && broker.session === broker.initialSession && broker.patchDropped
        && broker.stateDropped && broker.commandAckDropped, 'GATE_RUNTIME_RECOVERY');
      const app = await appCall('runtime-verify');
      assert.equal(app.after, prepared.before + 2, 'GATE_COUNTER');
      console.log(JSON.stringify({ fault: config.fault, before: prepared.before, first, second, app, broker }));
      return;
    }
    for (let iteration = 1; iteration <= config.count; iteration++) {
      const result = await measureColdStart({
        now: () => performance.now(),
        prepare: async () => { await appCall('prepare'); await brokerCall('start'); },
        openSerial: mark => new Promise((resolve, reject) => {
          // Only 'd' is written. DTR/RTS are disabled: no resetAccess/reboot.
          const program = 'import serial,sys\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();s.reset_input_buffer()\nprint("opened",flush=True)\nsys.stdin.readline();s.write(b"d");print("sent",flush=True)\nwhile True:\n v=s.readline().decode("utf-8","replace").strip()\n if v.startswith(("childDisconnect=","childState=")): print(v,flush=True)';
          serial = spawn(config.python, ['-u', '-c', program, config.port], { windowsHide: true });
          lines = createInterface({ input: serial.stdout });
          const deadline = setTimeout(() => reject(Error('GATE_SERIAL_OPEN')), 5000);
          serial.once('error', () => { clearTimeout(deadline); reject(Error('GATE_SERIAL')); });
          serial.once('exit', () => { clearTimeout(deadline); reject(Error('GATE_SERIAL_EXIT')); });
          lines.on('line', value => {
            if (value === 'opened') { clearTimeout(deadline); resolve(); return; }
            if (value === 'sent') { mark('serial-sent'); return; }
            const state = /^childState=(\d+) secure=(\d+) error=(\d+)/.exec(value);
            if (state) mark('child', { state: Number(state[1]), secure: state[2] === '1', error: Number(state[3]) });
            if (value.startsWith('childDisconnect=')) mark('disconnect-result', { ok: value === 'childDisconnect=0' });
          });
        }),
        waitReady: () => brokerCall('waitReady'),
        trigger: async () => { serial.stdin.write('go\n'); },
        control: async () => {
          if (config.readyWait) {
            const readiness = await appCall('ready-status');
            console.log(JSON.stringify({ phase: 'app-shared-ready', readiness }));
            assert(readiness.cancelled && readiness.ready, 'GATE_SHARED_READY');
          }
          let closureReplay;
          if (config.fault === 'closure-receipt') {
            const closure = await brokerCall('replayClosure');
            assert(closure.events.some(value => value.phase === 'accepted' && value.closure && !value.ready), 'GATE_CLOSURE_OFFLINE');
            assert(closure.events.some(value => value.phase === 'drop-down' && value.closure), 'GATE_CLOSURE_RECEIPT_LOSS');
            const notices = closure.events.filter(value => value.phase === 'up' && value.closure);
            assert(notices.length > 0 && notices.every(value => value.digest === notices[0].digest), 'GATE_CLOSURE_EXACT_RETRY');
            closureReplay = { sameMqtt: closure.sameMqtt, ready: closure.ready, notices: notices.length,
              staleRejected: closure.events.some(value => value.phase === 'old-closure-rejected') };
          }
          const app = await appCall('control');
          return closureReplay ? { ...app, closureReplay } : app;
        },
        close: async () => {
          lines?.close(); serial?.kill();
          try { await brokerCall('close'); } finally { await appCall('close'); }
        }
      });
      console.log(JSON.stringify({ iteration, ...result }));
    }
  } finally {
    lines?.close(); serial?.kill();
    try { await appCall('close'); } finally { await brokerCall('disable-inspector'); }
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(() => { console.error('Cold gate failed; inspect state before retrying; never replay the Action automatically.'); process.exitCode = 1; });
}
