import assert from 'node:assert/strict';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { runGatewayAppGate } from './gateway_app_gate.mjs';
import { appColdAction } from './edge_gateway_cold_start_gate.mjs';

// One exact Hub/child, no reset/rebind. Uses the same bounded interception and
// production App account helpers as cold-start gates, not a production API.
export function verifyLeaseEvidence(fault, broker) {
  assert(broker.sameMqtt, 'GATE_MQTT_CHANGED');
  const events = broker.events;
  assert(!events.some(e => e.phase === 'down' && e.kind === 0x12), 'GATE_UNEXPECTED_ACTION');
  if (fault === 'lease-loss-all') {
    assert(!broker.ready && broker.waitResult === 'offline', 'GATE_LEASE_NOT_EXPIRED');
    assert(events.some(e => e.phase === 'drop-up' && e.lease), 'GATE_PROOF_NOT_DROPPED');
    assert(events.some(e => e.phase === 'drop-up' && e.closure), 'GATE_CLOSURE_NOT_DROPPED');
    assert(events.some(e => e.phase === 'drop-up' && e.kind === 1), 'GATE_HELLO_NOT_DROPPED');
    assert(!events.some(e => e.phase === 'accepted'), 'GATE_LOSS_NOT_TOTAL');
    assert(!events.some(e => e.phase === 'admission'), 'GATE_UNREQUESTED_RECOVERY');
    return;
  }
  assert(broker.ready && broker.waitResult === 'ready' && broker.session === broker.initialSession, 'GATE_LEASE_SESSION');
  const proofs = events.filter(e => e.phase === 'accepted' && e.lease);
  assert(new Set(proofs.map(e => e.challengeDigest)).size >= 3, 'GATE_LEASE_CYCLES');
  if (fault === 'observe') assert(!broker.dropped, 'GATE_UNEXPECTED_DROP');
  else {
    const phase = fault === 'lease-request' ? 'drop-down' : 'drop-up';
    const drops = events.filter(e => e.phase === phase && e.lease);
    assert.equal(drops.length, 1, 'GATE_DROP_COUNT');
    const retry = events.find(e => e.phase === phase.slice(5) && e.lease && e.digest === drops[0].digest);
    assert(retry && retry.ms - drops[0].ms >= 4500 && retry.ms - drops[0].ms < 8000, 'GATE_EXACT_RETRY');
  }
}

async function main() {
  const config = JSON.parse(process.argv[2] || '{}');
  assert(config.hub && config.child && config.tenant && config.instance && config.container, 'GATE_CONFIG');
  assert(['observe', 'lease-request', 'lease-proof', 'lease-loss-all'].includes(config.fault), 'GATE_FAULT');
  if (config.fault === 'lease-loss-all') assert(config.port && config.python, 'GATE_SERIAL_CONFIG');
  const execute = promisify(execFile);
  const run = async (file, args, timeout = 55000) => (await execute(file, args, {
    timeout, windowsHide: true, maxBuffer: 128 * 1024,
  })).stdout;
  const remote = '/tmp/edge_gateway_route_recovery_gate.cjs';
  const brokerCall = async action => {
    console.log(JSON.stringify({ phase: `broker-${action}` }));
    return JSON.parse(await run('docker', ['exec', config.container, 'node', remote, action, JSON.stringify(config)]));
  };
  const appCall = action => {
    console.log(JSON.stringify({ phase: `app-${action}` }));
    return runGatewayAppGate(appColdAction, { ...config, action, readyWait: false }, 45000);
  };
  try {
    await run('docker', ['exec', config.container, 'node', '-e', 'process.kill(1,"SIGUSR1")']);
    await run('docker', ['cp', fileURLToPath(new URL('./edge_gateway_route_recovery_gate.cjs', import.meta.url)), `${config.container}:${remote}`]);
    const before = await appCall('prepare');
    assert(Number.isInteger(before.before), 'GATE_COUNTER');
    await brokerCall('start');
    let broker, offline, serial, recovery;
    if (config.fault === 'lease-loss-all') {
      const armed = await brokerCall('waitLeaseDrop');
      assert(armed.sameMqtt && armed.waitResult === 'dropped', 'GATE_NOT_ARMED');
      // Normal disconnect only. Filter diagnostic output; never print access
      // material or toggle DTR/RTS to reset the application.
      const program = 'import serial,sys,time\ns=serial.Serial();s.port=sys.argv[1];s.baudrate=115200;s.timeout=.1;s.dtr=False;s.rts=False;s.open();s.reset_input_buffer();s.write(b"d");end=time.monotonic()+3\nwhile time.monotonic()<end:\n v=s.readline().decode("utf8","replace").strip()\n if v.startswith(("childDisconnect=","childState=")):print(v)\ns.close()';
      serial = await run(config.python, ['-c', program, config.port], 10000);
      assert(serial.includes('childDisconnect=0'), 'GATE_DISCONNECT');
      broker = await brokerCall('waitOffline');
      verifyLeaseEvidence(config.fault, broker);
      offline = await appCall('offline-verify');
      assert(offline.rejected, 'GATE_APP_OFFLINE');
      const replayed = await brokerCall('replayLease');
      assert(!replayed.ready && replayed.sameMqtt, 'GATE_OLD_PROOF_RESURRECTED');
      await new Promise(resolve => setTimeout(resolve, 5000));
      const stillOffline = await brokerCall('status');
      assert(!stillOffline.ready && stillOffline.sameMqtt
        && !stillOffline.events.some(e => e.phase === 'admission'), 'GATE_UNREQUESTED_RECOVERY');
      await brokerCall('recover'); // Lifts injection; only volatile admission.
      const recovered = await brokerCall('waitReady');
      assert(recovered.sameMqtt && recovered.ready && recovered.session !== recovered.initialSession, 'GATE_RECOVERY');
      await brokerCall('replayLease'); // Same captured old proof against a new generation.
      recovery = { session: recovered.session, events: recovered.events.filter(e => ['retire', 'readmitted', 'ready', 'wait-ready'].includes(e.phase)) };
    } else {
      broker = await brokerCall('waitLeases');
      verifyLeaseEvidence(config.fault, broker);
      await brokerCall('replayLease'); // Already-consumed nonce; must not change the route.
    }
    const after = await appCall('ready-verify');
    assert.equal(after.after, before.before, 'GATE_UNEXPECTED_COUNTER_CHANGE');
    console.log(JSON.stringify({ fault: config.fault, before: before.before, after: after.after, offline, serial, recovery, broker }));
  } finally {
    // A failed test does not issue another Action or reset a device.
    await appCall('close').catch(() => undefined);
    await brokerCall('close').catch(() => undefined);
    await brokerCall('disable-inspector').catch(() => undefined);
  }
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  main().catch(() => {
    console.error('Lease gate failed; inspect projection and App state before continuing. No Action was issued.');
    process.exitCode = 1;
  });
}
