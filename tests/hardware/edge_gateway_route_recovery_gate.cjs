'use strict';

// Local, disposable Broker Inspector gate. Never shipped with the Arduino
// library or Broker image; no production HTTP endpoint or credential export.
function installRouteRecoveryGate(input, dependencies) {
  const requireValue = (value, code) => { if (!value) throw Error(code); };
  const durationMillis = input.durationMillis ?? 120000;
  requireValue(Number.isInteger(durationMillis) && durationMillis >= 1000 && durationMillis <= 180000, 'GATE_DURATION');
  requireValue(['observe', 'closure-receipt', 'admission', 'hello', 'manifest', 'bootstrap-exhaustion', 'bootstrap-terminal', 'unknown-action', 'runtime-state',
    'lease-request', 'lease-proof', 'lease-loss-all', 'execution-release-loss'].includes(input.fault), 'GATE_FAULT');
  const req = dependencies ? null : process.mainModule.require.bind(process.mainModule);
  const registry = dependencies?.registry || req('./protocol/v2/session-registry');
  const routes = dependencies?.routes || req('./protocol/v2/edge-gateway-route');
  const channels = dependencies?.channels || req('./protocol/v2/virtual-device-channel');
  const protocol = dependencies?.protocol || req('@aily-iot/device-v2-protocol');
  const hash = dependencies?.hash || (bytes => req('crypto').createHash('sha256').update(bytes).digest('hex'));
  const frame = dependencies?.frame || req('./protocol/v2/frame').parseBbp2Frame;
  const demand = dependencies?.demand || (!dependencies && req('./protocol/v2/app-lane').getV2AppConnectionDemand);
  const connection = registry.getV2EdgeHubRouteConnection(input.hub, input.tenant);
  requireValue(connection?.client?.broker && connection.claims.managementReady, 'GATE_HUB_NOT_READY');
  const { client, claims } = connection, broker = client.broker;
  const current = () => registry.getV2Connection(client) === claims && !client.closed;
  const channel = () => channels.getReadyV2VirtualDeviceChannel(input.child, input.tenant);
  const initial = channel();
  requireValue(initial && initial.claims.sessionId.startsWith(`edge:${claims.sessionId}:`), 'GATE_CHILD_NOT_READY');
  const initialCounter = initial.claims.stateValues.buttonPresses?.value;
  if (input.fault === 'runtime-state') requireValue(Number.isInteger(initialCounter), 'GATE_COUNTER_BASELINE');
  const startedAt = Date.now(), started = performance.now();
  const events = [], originalPublish = broker.publish, originalAccept = routes.acceptEdgeGatewayRouteUp;
  let closed = false, dropped = false, recovered = false, oldCommand = null, oldAck = null;
  let blockedBootstrapDigest = null;
  let closureNotice = null, closureReceipt = null;
  let leaseProof = null, leaseReplays = 0, leaseLossReleased = false;
  let patchDropped = false, stateDropped = false, commandAckDropped = false;
  const runtimeCommands = []; // At most two exact test Actions, never their payloads.
  let observedSession = initial.claims.sessionId, timer, deadline;
  const event = (phase, fields = {}) => {
    if (events.length < 128) events.push({ ms: Math.round(performance.now() - started), phase, ...fields });
  };
  const metadata = bytes => {
    if (bytes[0] === 0x8c) {
      const control = protocol.decodeGatewayDirectYield(bytes);
      if (control.childLogicalDeviceId !== input.child) return null;
      return { direct: true, operation: control.operation, controlId: control.controlId,
        challengeId: control.challengeId, leaseMillis: control.leaseMillis, digest: hash(bytes) };
    }
    if (bytes[0] === 0x8b) {
      const control = protocol.decodeGatewayExecutionDemand(bytes);
      if (control.childLogicalDeviceId !== input.child) return null;
      return { execution: true, operation: control.operation, challengeId: control.challengeId,
        leaseMillis: control.leaseMillis, incarnationDigest: hash(control.incarnation), digest: hash(bytes) };
    }
    if (bytes[0] === 0x8a && bytes[1] === 1) {
      const lease = protocol.decodeGatewayRouteLease(bytes);
      if (lease.childLogicalDeviceId !== input.child) return null;
      return { lease: true, acknowledgment: lease.acknowledgment,
        generation: Buffer.from(lease.generation).toString('hex'), challengeDigest: hash(lease.challenge), digest: hash(bytes) };
    }
    if (bytes[0] === 0x89) {
      const closure = protocol.decodeGatewayRouteClosure(bytes);
      if (closure.childLogicalDeviceId !== input.child) return null;
      return { closure: true, acknowledgment: closure.acknowledgment,
        generation: Buffer.from(closure.generation).toString('hex'), digest: hash(bytes) };
    }
    const envelope = protocol.decodeChildRouteEnvelope(bytes);
    if (envelope.childLogicalDeviceId !== input.child) return null;
    const header = frame(Buffer.from(envelope.innerBbp2Frame));
    return { kind: header.kind, sequence: header.sequence, frameBytes: envelope.innerBbp2Frame.length,
      generation: Buffer.from(envelope.correlationId.subarray(0, 8)).toString('hex'),
      correlation: Buffer.from(envelope.correlationId).toString('hex'), digest: hash(bytes) };
  };
  const snapshot = () => {
    const ready = channel();
    return { fault: input.fault, startedAt, closed, sameMqtt: current(), dropped, recovered,
      patchDropped, stateDropped, commandAckDropped,
      initialSession: initial.claims.sessionId, session: ready?.claims.sessionId || null,
      ready: !!ready, stateRevision: ready?.claims.stateRevision,
      buttonPresses: ready?.claims.stateValues.buttonPresses?.value,
      demandOwners: demand ? demand(input.tenant, input.child).owners : undefined, events: events.slice() };
  };
  const close = () => {
    if (!closed) {
      closed = true;
      clearInterval(timer); clearTimeout(deadline);
      if (broker.publish === hookedPublish) broker.publish = originalPublish;
      if (routes.acceptEdgeGatewayRouteUp === hookedAccept) routes.acceptEdgeGatewayRouteUp = originalAccept;
      oldCommand?.fill(0); oldCommand = null;
      oldAck?.fill(0); oldAck = null;
      closureNotice?.fill(0); closureNotice = null;
      closureReceipt?.fill(0); closureReceipt = null;
      leaseProof?.fill(0); leaseProof = null;
      event('closed');
    }
    return snapshot();
  };
  const recover = async () => {
    requireValue(!closed && current() && !recovered, 'GATE_RECOVERY_PRECONDITION');
    const target = input.recoveryTarget;
    requireValue(target && /^[a-f0-9]{32}$/.test(target.operationId)
      && Number.isSafeInteger(target.topologyVersion) && target.topologyVersion > 0
      && target.topologyVersion <= 0xffffffff, 'GATE_RECOVERY_TARGET');
    // Retire only the captured topology. A stale operation/version must not
    // clear a successor, sibling, or the shared MQTT connection.
    requireValue(routes.revokeEdgeGatewayRouteForChild(input.tenant, input.hub, {
      childLogicalDeviceId: input.child, operationId: Buffer.from(target.operationId, 'hex'),
      topologyVersion: target.topologyVersion
    }), 'GATE_RECOVERY_TARGET_CHANGED');
    recovered = true;
    leaseLossReleased = true; // Volatile route recovery ends loss-all injection; no Action is sent.
    event('retire');
    const admitted = await routes.admitEdgeGatewayRoute(broker, client, claims, input.shard);
    event('readmitted', { admitted });
    return snapshot();
  };
  function hookedPublish(packet, callback) {
    if (closed || !current() || packet.topic !== claims.routeSubscribeTopic) {
      return originalPublish.call(this, packet, callback);
    }
    let info, phase;
    if (packet.payload[0] === 0x8a && packet.payload[1] === 2) {
      const admission = protocol.decodeGatewayRouteAdmission(packet.payload);
      if (admission.childLogicalDeviceId !== input.child) return originalPublish.call(this, packet, callback);
      info = { floor: Buffer.from(admission.retiredGeneration).toString('hex'), digest: hash(packet.payload) };
      phase = 'admission';
    } else {
      info = metadata(packet.payload); phase = 'down';
      if (!info) return originalPublish.call(this, packet, callback);
      if (info.closure && !closureReceipt) closureReceipt = Buffer.from(packet.payload);
      if (input.fault === 'runtime-state' && info.kind === 0x12
        && !runtimeCommands.includes(info.correlation) && runtimeCommands.length < 2) {
        runtimeCommands.push(info.correlation);
      }
      if (input.fault === 'unknown-action' && info.kind === 0x12 && !oldCommand) {
        oldCommand = Buffer.from(packet.payload);
      }
    }
    const exhaust = input.fault === 'bootstrap-exhaustion' && info.kind === 2;
    if (exhaust && !blockedBootstrapDigest) blockedBootstrapDigest = info.digest;
    const dropState = input.fault === 'runtime-state' && info.kind === 0x10 && !stateDropped;
    if (dropState) stateDropped = true;
    const drop = (input.fault === 'execution-release-loss' && info.execution && info.operation === 2 && info.leaseMillis === 0)
      || (input.fault === 'lease-request' && info.lease && !dropped)
      || (input.fault === 'lease-loss-all' && !leaseLossReleased && info.closure)
      || (input.fault === 'closure-receipt' && info.closure && !dropped)
      || dropState || (input.fault === 'bootstrap-terminal' && info.kind === 2)
      || (exhaust && info.digest === blockedBootstrapDigest)
      || (!dropped && ((phase === 'admission' && input.fault === 'admission')
        || (info.kind === 2 && input.fault === 'manifest')));
    event(drop ? `drop-${phase}` : phase, info);
    if (drop) { dropped = true; queueMicrotask(() => callback?.(null)); return; }
    return originalPublish.call(this, packet, callback);
  }
  async function hookedAccept(aedes, source, identity, payload, shard) {
    if (closed || source !== client || identity !== claims || !current()) {
      return originalAccept.apply(this, arguments);
    }
    const info = metadata(payload);
    if (!info) return originalAccept.apply(this, arguments);
    if (info.closure && !closureNotice) closureNotice = Buffer.from(payload);
    if (info.lease && !leaseProof) leaseProof = Buffer.from(payload);
    const dropPatch = input.fault === 'runtime-state' && info.kind === 0x11 && !patchDropped;
    const dropAck = input.fault === 'runtime-state' && info.kind === 0x14 && !commandAckDropped
      && runtimeCommands.length === 2 && info.correlation === runtimeCommands[1];
    if (dropPatch) patchDropped = true;
    if (dropAck) commandAckDropped = true;
    const drop = (input.fault === 'lease-loss-all' && !leaseLossReleased)
      || (input.fault === 'lease-proof' && info.lease && !dropped)
      || dropPatch || dropAck || (!dropped && ((input.fault === 'hello' && info.kind === 1)
      || (input.fault === 'unknown-action' && oldCommand && info.kind === 0x14
        && info.correlation === metadata(oldCommand).correlation)));
    event(drop ? 'drop-up' : 'up', info);
    if (drop) {
      dropped = true;
      if (input.fault === 'unknown-action') {
        oldAck = Buffer.from(payload);
        await recover();
      }
      return null;
    }
    const result = await originalAccept.apply(this, arguments);
    // Lifecycle controls intentionally return no BBP lane result. Continued
    // fresh challenges/ready, not that null result, prove lease renewal.
    event('accepted', { kind: info.kind, closure: info.closure, lease: info.lease,
      execution: info.execution, operation: info.operation, challengeId: info.challengeId,
      challengeDigest: info.challengeDigest, bbpAccepted: info.kind === undefined ? undefined : result !== null, ready: !!channel() });
    return result;
  }
  broker.publish = hookedPublish;
  routes.acceptEdgeGatewayRouteUp = hookedAccept;
  timer = setInterval(() => {
    if (!current()) { event('mqtt-changed'); close(); return; }
    const ready = channel();
    if (ready && ready.claims.sessionId !== observedSession) {
      observedSession = ready.claims.sessionId;
      event('ready', { session: observedSession, revision: ready.claims.stateRevision });
    }
  }, 50);
  deadline = setTimeout(close, durationMillis);
  timer.unref?.(); deadline.unref?.();
  event('started');
  const waitFor = (matches, timeoutMillis, matched = 'ready') => new Promise(resolve => {
    let interval, timeout;
    const finish = reason => {
      clearInterval(interval); clearTimeout(timeout);
      event(`wait-${reason}`);
      resolve({ ...snapshot(), waitResult: reason });
    };
    interval = setInterval(() => {
      const state = snapshot();
      if (!state.sameMqtt || state.closed) finish('interrupted');
      else if (matches(state)) finish(matched);
    }, 50);
    timeout = setTimeout(() => finish('deadline'), timeoutMillis);
  });
  const waitReady = () => waitFor(state => state.ready && (input.fault === 'runtime-state'
    ? state.session === state.initialSession && patchDropped && stateDropped && commandAckDropped
      && state.buttonPresses === initialCounter + 2
    : state.session !== state.initialSession), 45000);
  return { snapshot, close, recover, waitReady,
    waitLeases: () => waitFor(state => state.ready && state.session === state.initialSession
      && new Set(state.events.filter(e => e.phase === 'accepted' && e.lease).map(e => e.challengeDigest)).size >= 3, 45000),
    waitLeaseDrop: () => waitFor(state => state.events.some(e => e.phase === 'drop-up' && e.lease), 15000, 'dropped'),
    waitOffline: () => waitFor(state => !state.ready && !initial.isOpen(), 40000, 'offline'),
    replayLease: async () => {
      requireValue(!closed && current() && leaseProof && leaseReplays < 2, 'GATE_LEASE_REPLAY_PRECONDITION');
      ++leaseReplays;
      const before = channel();
      await originalAccept.call(routes, broker, client, claims, Buffer.from(leaseProof), input.shard);
      requireValue(channel() === before, 'GATE_LEASE_REPLAY_CHANGED_ROUTE');
      event('old-lease-replayed');
      return snapshot();
    },
    waitPatch: () => waitFor(state => state.patchDropped, 5000), replayClosure: async () => {
    const ready = channel();
    requireValue(!closed && current() && closureNotice && closureReceipt && ready
      && ready.claims.sessionId !== initial.claims.sessionId, 'GATE_CLOSURE_REPLAY_PRECONDITION');
    await originalAccept.call(routes, broker, client, claims, Buffer.from(closureNotice), input.shard);
    requireValue(channel() === ready, 'GATE_STALE_CLOSURE_RETIRED_READY');
    await new Promise((resolve, reject) => originalPublish.call(broker, {
      cmd: 'publish', topic: claims.routeSubscribeTopic, payload: Buffer.from(closureReceipt),
      qos: 0, retain: false, dup: false
    }, error => error ? reject(error) : resolve()));
    event('old-closure-rejected');
    return snapshot();
  }, replayOld: async () => {
    requireValue(!closed && current() && oldCommand && oldAck && channel()
      && channel().claims.sessionId !== initial.claims.sessionId, 'GATE_REPLAY_PRECONDITION');
    const accepted = await originalAccept.call(routes, broker, client, claims, Buffer.from(oldAck), input.shard);
    requireValue(accepted === null, 'GATE_OLD_ACK_ACCEPTED');
    event('old-ack-rejected');
    // Deliberately replay the exact old envelope; the physical Hub must reject
    // its retired generation before it can send another Action to the child.
    await new Promise((resolve, reject) => originalPublish.call(broker, {
      cmd: 'publish', topic: claims.routeSubscribeTopic, payload: Buffer.from(oldCommand),
      qos: 0, retain: false, dup: false
    }, error => error ? reject(error) : resolve()));
    event('old-command-injected', metadata(oldCommand));
    return snapshot();
  } };
}

async function main() {
  const [action, configText = '{}'] = process.argv.slice(2);
  if (!['preflight', 'start', 'status', 'recover', 'waitReady', 'waitPatch', 'waitLeases', 'waitLeaseDrop', 'waitOffline', 'replayLease', 'replayOld', 'replayClosure', 'close', 'disable-inspector'].includes(action)) throw Error('GATE_ACTION');
  const config = JSON.parse(configText);
  const http = require('http');
  const req = require('module').createRequire(require('path').join(process.cwd(), 'package.json'));
  const WebSocket = req('ws');
  const pages = await new Promise((resolve, reject) => {
    const request = http.get('http://127.0.0.1:9229/json/list', response => {
      let body = ''; response.on('data', chunk => { body += chunk; });
      response.on('end', () => { try { resolve(JSON.parse(body)); } catch { reject(Error('GATE_INSPECTOR_RESPONSE')); } });
    });
    request.on('error', () => reject(Error('GATE_INSPECTOR_UNAVAILABLE')));
    request.setTimeout(5000, () => request.destroy());
  });
  const socket = new WebSocket(pages[0].webSocketDebuggerUrl);
  const expression = action === 'preflight'
    ? `(()=>{const c=${JSON.stringify(config)},r=process.mainModule.require('./protocol/v2/session-registry'),v=process.mainModule.require('./protocol/v2/virtual-device-channel');const h=r.getV2EdgeHubRouteConnection(c.hub,c.tenant),d=v.getReadyV2VirtualDeviceChannel(c.child,c.tenant);return {hubConnected:!!h&&!h.client.closed,managementReady:!!h?.claims.managementReady,routeReady:!!h?.claims.routeReady,childReady:!!d,revision:d?.claims.stateRevision,counter:d?.claims.stateValues.buttonPresses?.value};})()`
    : action === 'start'
    ? `(async()=>{if(global.__edgeRouteGate&&!global.__edgeRouteGate.snapshot().closed)throw Error('GATE_BUSY');
      const c=${JSON.stringify(config)},r=await process.mainModule.require('./gateway-share-client').resolveActiveEdgeGatewayRoute(c.tenant,c.child,c.shard);
      if(!r||r.tenantId!==c.tenant||r.childLogicalDeviceId!==c.child||r.edgeHubLogicalDeviceId!==c.hub)throw Error('GATE_RECOVERY_TARGET');
      c.recoveryTarget={operationId:Buffer.from(r.operationId).toString('hex'),topologyVersion:r.topologyVersion};
      if(global.__edgeRouteGate&&!global.__edgeRouteGate.snapshot().closed)throw Error('GATE_BUSY');
      global.__edgeRouteGate=(${installRouteRecoveryGate.toString()})(c);return global.__edgeRouteGate.snapshot();})()`
    : action === 'disable-inspector'
      ? `(()=>{global.__edgeRouteGate?.close();delete global.__edgeRouteGate;setTimeout(()=>process.mainModule.require('inspector').close(),250);return {inspectorClosing:true};})()`
      : `global.__edgeRouteGate.${action === 'status' ? 'snapshot' : action}()`;
  try {
    const result = await new Promise((resolve, reject) => {
      const deadline = setTimeout(() => reject(Error('GATE_INSPECTOR_DEADLINE')), action.startsWith('wait') ? 50000 : 15000);
      socket.on('error', () => { clearTimeout(deadline); reject(Error('GATE_INSPECTOR_SOCKET')); });
      socket.on('open', () => socket.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate',
        params: { expression, awaitPromise: true, returnByValue: true } })));
      socket.on('message', bytes => {
        const response = JSON.parse(bytes);
        if (response.id !== 1) return;
        clearTimeout(deadline);
        if (response.error || response.result?.exceptionDetails) reject(Error('GATE_INSPECTOR_EVALUATION'));
        else resolve(response.result.result.value);
      });
    });
    console.log(JSON.stringify(result));
  } finally { socket.close(); }
}

module.exports = { installRouteRecoveryGate };
if (require.main === module) main().catch(() => { console.error('Route recovery gate failed; inspect preconditions, do not retry Actions blindly.'); process.exitCode = 1; });
