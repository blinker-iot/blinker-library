import assert from 'node:assert/strict';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { appColdAction } from './edge_gateway_cold_start_gate.mjs';

// Real App Session API only; never synthesize Broker owners or device MQTT credentials.
export async function appExecutionAction(input) {
  const page = globalThis.ng?.getComponent(document.querySelector('app-edge-gateway-enrollment'));
  if (!page?.available || page.busy || page.workflow || page.edgeHubLogicalDeviceId !== input.hub) throw Error('GATE_PAGE');
  const cloud = globalThis.ng.getComponent(document.querySelector('app-root')).deviceService.deviceUi.deviceV2;
  if (input.action === 'close') {
    for (const owner of globalThis.__edgeDemandOwners || []) owner.release();
    delete globalThis.__edgeDemandOwners;
    await cloud.stop(); return { closed: true };
  }
  if (input.action === 'end-session') {
    const owners = globalThis.__edgeDemandOwners || [];
    if (owners.length !== 1) throw Error('GATE_SESSION_OWNER');
    // Do not release first: prove the production session retirement path.
    await cloud.stop();
    const reason = await owners[0].closed;
    delete globalThis.__edgeDemandOwners;
    return { reason };
  }
  if (input.action === 'acquire') {
    if (globalThis.__edgeDemandOwners?.length) throw Error('GATE_OWNERS_BUSY');
    await cloud.ensureReady(input.child);
    const owners = globalThis.__edgeDemandOwners = [];
    const started = performance.now();
    for (let i = 0; i < input.owners; i++) {
      const owner = cloud.client.session.acquireConnectionDemand(input.child);
      owners.push(owner); await owner.accepted;
    }
    return { owners: owners.length, acceptedMs: Math.round(performance.now() - started) };
  }
  if (input.action === 'release-one' || input.action === 'release-all') {
    const owners = globalThis.__edgeDemandOwners || [];
    do { owners.pop()?.release(); } while (input.action === 'release-all' && owners.length);
    return { owners: owners.length };
  }
  if (input.action === 'cancel') {
    const abort = new AbortController();
    const owner = cloud.client.session.acquireConnectionDemand(input.child, abort.signal);
    const accepted = owner.accepted.then(() => false, e => e.name === 'AbortError');
    abort.abort();
    return { cancelled: await accepted, reason: await owner.closed };
  }
  throw Error('GATE_ACTION');
}

export function verifyExecutionEvidence(state) {
  assert(state.sameMqtt && state.ready && state.session === state.initialSession, 'GATE_ROUTE_CHANGED');
  assert.equal(state.demandOwners, 0, 'GATE_OWNER_LEAK');
  const decisions = state.events.filter(e => e.phase === 'down' && e.execution && e.operation === 2);
  const positive = decisions.filter(e => e.leaseMillis > 0);
  assert(new Set(positive.map(e => `${e.incarnationDigest}:${e.challengeId}`)).size >= 2, 'GATE_NO_RENEWAL');
  assert(positive.every(e => e.leaseMillis <= 30000), 'GATE_LEASE_TOO_LONG');
  assert(decisions.at(-1)?.leaseMillis === 0, 'GATE_NO_RELEASE');
  assert(!state.events.some(e => e.phase === 'down' && e.kind === 0x12), 'GATE_UNEXPECTED_ACTION');
}

async function main() {
  const config = JSON.parse(process.argv[2] || '{}');
  assert(config.hub && config.child && config.tenant && config.instance && config.container && config.port && config.python);
  assert(['observe', 'execution-release-loss'].includes(config.fault || 'observe'));
  if (config.fault === 'execution-release-loss') assert(config.adb && config.androidSerial);
  const execute = promisify(execFile);
  const run = async (file, args, timeout = 45000) => (await execute(file, args, {
    timeout, windowsHide: true, maxBuffer: 128 * 1024,
  })).stdout;
  const remote = '/tmp/edge_gateway_route_recovery_gate.cjs';
  const broker = async action => JSON.parse(await run('docker', ['exec', config.container, 'node', remote, action,
    JSON.stringify({ ...config, fault: config.fault || 'observe' })]));
  const app = (action, owners = 2) => runGatewayAppGate(appExecutionAction, { ...config, action, owners }, 45000);
  const mark = (phase, value) => console.log(JSON.stringify({ phase, ...value }));
  const wait = async (matches, timeout = 15000) => {
    const until = performance.now() + timeout;
    do {
      const state = await broker('status');
      assert(state.sameMqtt && !state.closed, 'GATE_INTERRUPTED');
      if (matches(state)) return state;
      await new Promise(resolve => setTimeout(resolve, 500));
    } while (performance.now() < until);
    throw Error('GATE_WAIT_DEADLINE');
  };
  const serialWanted = async (wanted, seconds = 14) => {
    // Open without reset; the internal sketch prints transitions, otherwise every 10 s.
    const script = 'import serial,sys,time\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();s.reset_input_buffer();end=time.monotonic()+int(sys.argv[3])\nwhile time.monotonic()<end:\n v=s.readline().decode("utf8","replace").strip()\n if v.startswith("childState=") and " executionWanted="+sys.argv[2]+" " in v: print(v);break\ns.close()';
    const line = (await run(config.python, ['-c', script, config.port, String(wanted), String(seconds)], seconds * 1000 + 3000)).trim();
    assert(line.includes(` executionWanted=${wanted} `), 'GATE_HUB_DEMAND_NOT_OBSERVED');
    assert(line.includes(' secure=1 ') && line.includes(' routeError=0 '), 'GATE_HUB_ROUTE');
    mark('hub-observed', { wanted, diagnostic: line });
  };
  try {
    await run('docker', ['exec', config.container, 'node', '-e', 'process.kill(1,"SIGUSR1")']);
    await run('docker', ['cp', fileURLToPath(new URL('./edge_gateway_route_recovery_gate.cjs', import.meta.url)), `${config.container}:${remote}`]);
    const before = await runGatewayAppGate(appColdAction, { ...config, action: 'prepare', readyWait: false }, 45000);
    assert(Number.isInteger(before.before), 'GATE_COUNTER');
    await broker('start');
    if (config.fault === 'execution-release-loss') {
      mark('loss-owner', await app('acquire', 1));
      await wait(s => s.demandOwners === 1 && s.events.some(e => e.execution && e.leaseMillis > 0));
      await serialWanted(1);
      const started = performance.now();
      await run(config.adb, ['-s', config.androidSerial, 'shell', 'am', 'force-stop', 'iot.diandeng.tech.debug']);
      const retired = await wait(s => s.demandOwners === 0 && s.events.some(e => e.phase === 'drop-down' && e.execution));
      mark('killed-app-owner-retired', { ms: Math.round(performance.now() - started), owners: retired.demandOwners });
      await serialWanted(0, 40);
      const state = await broker('status');
      assert(state.sameMqtt && state.ready && state.demandOwners === 0 && state.session === state.initialSession, 'GATE_ROUTE_CHANGED');
      assert(!state.events.some(e => e.phase === 'down' && e.execution && e.operation === 2 && e.leaseMillis === 0), 'GATE_RELEASE_DELIVERED');
      assert.equal(state.buttonPresses, before.before);
      mark('loss-passed', { ms: Math.round(performance.now() - started), before: before.before, after: state.buttonPresses, broker: state });
      return;
    }
    mark('two-owners', await app('acquire'));
    await wait(s => s.demandOwners === 2 && s.events.some(e => e.execution && e.operation === 2 && e.leaseMillis > 0));
    await serialWanted(1);
    const beforePartial = await broker('status');
    const previousIds = new Set(beforePartial.events.filter(e => e.execution && e.leaseMillis > 0).map(e => e.challengeId));
    mark('release-one', await app('release-one'));
    await wait(s => s.demandOwners === 1);
    const retained = await wait(s => s.demandOwners === 1 && s.events.some(e => e.phase === 'down'
      && e.execution && e.leaseMillis > 0 && !previousIds.has(e.challengeId)), 20000);
    assert(!retained.events.slice(beforePartial.events.length).some(e => e.execution && e.operation === 2 && e.leaseMillis === 0), 'GATE_PARTIAL_RELEASE');
    mark('retained-owner-renewed', { owners: retained.demandOwners });
    mark('release-all', await app('release-all'));
    let released = await wait(s => s.demandOwners === 0 && s.events.filter(e => e.phase === 'down' && e.execution && e.operation === 2).at(-1)?.leaseMillis === 0);
    verifyExecutionEvidence(released);
    await serialWanted(0);
    const cancelled = await app('cancel'); assert(cancelled.cancelled && cancelled.reason === 'cancelled'); mark('cancel-before-ack', cancelled);
    await wait(s => s.demandOwners === 0);
    mark('session-owner', await app('acquire', 1));
    await wait(s => s.demandOwners === 1);
    await serialWanted(1);
    const ended = await app('end-session');
    assert.equal(ended.reason, 'session-ended'); mark('session-ended', ended);
    released = await wait(s => s.demandOwners === 0 && s.events.filter(e => e.phase === 'down' && e.execution && e.operation === 2).at(-1)?.leaseMillis === 0);
    await serialWanted(0);
    verifyExecutionEvidence(released);
    const after = await runGatewayAppGate(appColdAction, { ...config, action: 'ready-verify' }, 45000);
    assert.equal(after.after, before.before);
    mark('passed', { before: before.before, after: after.after, broker: released });
  } catch (error) {
    mark('failed', { code: /^GATE_[A-Z_]+$/.test(error?.message || '') ? error.message : 'GATE_DEPENDENCY',
      broker: await broker('status').catch(() => null) });
    throw error;
  } finally {
    await app('close').catch(() => undefined);
    await runGatewayAppGate(appColdAction, { ...config, action: 'close' }, 15000).catch(() => undefined);
    await broker('close').catch(() => undefined);
    await broker('disable-inspector').catch(() => undefined);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(() => { console.error('Execution gate failed; inspect evidence before retrying. No Action or reset was issued.'); process.exitCode = 1; });
}
