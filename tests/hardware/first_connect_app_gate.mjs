// Debug-App measurement only: delegates every operation to the real implementation.
// No synthetic credentials, readiness, HTTP responses or automatic Action replay.
export async function firstConnectAppGate({ mode, logicalDeviceId: id, ownerId, expectedTransport = 'cloud', measureBlePhases = false, useCachedAccountForBle = false, requireCloudCapable = true, testConnectionPriority, testPriorityRejection = false, cancelAtPhase }) {
  if (typeof requireCloudCapable !== 'boolean' || (!requireCloudCapable
    && (expectedTransport !== 'ble' || !measureBlePhases))) throw Error('Pure BLE requires explicit measured BLE mode');
  if (testConnectionPriority !== undefined && (testConnectionPriority !== 1
    || expectedTransport !== 'ble' || !measureBlePhases)) throw Error('Invalid BLE timing experiment');
  if (typeof testPriorityRejection !== 'boolean' || testPriorityRejection && testConnectionPriority !== 1
    || cancelAtPhase !== undefined && (mode !== 'cancel' || !measureBlePhases || expectedTransport !== 'ble'
      || !['gatt', 'authentication', 'manifest', 'state'].includes(cancelAtPhase))) throw Error('Invalid BLE fault/cancel experiment');
  const root = ng.getComponent(document.querySelector('app-root'));
  const ui = root.deviceService.deviceUi;
  if (root.dataService.auth?.uuid !== ownerId) throw Error('Exact test owner required');
  const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
  const page = () => {
    const element = document.querySelector('app-device-v2-page');
    return element && ng.getComponent(element);
  };
  const snapshot = () => {
    const p = page();
    return {
      at: Date.now(), route: location.pathname, ready: p?.logicalDeviceId === id && p.canControl === true,
      manifestAccepted: p?.snapshot.manifestAccepted === true, stateFresh: p?.snapshot.stateFresh === true,
      cloud: ui.deviceV2.state.value, transport: ui.transport(id).value,
      groups: ui.connections.size, links: ui.ble.accountLinks.size,
      bleSessions: ui.ble.accountSessions.size, bleOpening: !!ui.ble.opening,
      lanOccupied: ui.lan?.connector.occupied ?? false,
      revision: p?.snapshot.stateRevision,
      fields: p?.snapshot.endpoints.map(f => ({ key: f.key, value: p.value(f) })),
      hasError: !!p?.error,
    };
  };
  let gate = globalThis.__blinkerFirstConnectGate;
  if (mode === 'open' || mode === 'cancel') {
    if (gate || ui.connections.size || ui.ble.accountLinks.size || ui.ble.accountSessions.size || ui.ble.opening) throw Error('Idle App required');
    if (!await ui.ble.hasActiveCredential(id) || !ui.supportsDirectBle(id)
      || (requireCloudCapable && !ui.isCloudCapable(id))) {
      throw Error('Real BLE credential and required capabilities missing');
    }
    if (useCachedAccountForBle && (expectedTransport !== 'ble' || !measureBlePhases)) {
      throw Error('Cached account is only for measured BLE; real admission remains required');
    }
    if (!useCachedAccountForBle) await root.userService.getAllInfo();
    gate = globalThis.__blinkerFirstConnectGate = {
      id, started: performance.now(), session: ui.deviceV2.client.session,
      events: [], restore: [], commandsAttempted: false, exiting: false,
      testPriorityRejection, commandAcks: [],
    };
    let reachedPhase;
    const phase = new Promise(resolve => { reachedPhase = resolve; });
    const trace = (name, state) => {
      gate.events.push({ name, state, millis: Math.round(performance.now() - gate.started) });
      if (name === cancelAtPhase && state === 'start') reachedPhase();
    };
    const priority = async (deviceId, value) => {
      const label = value === 1 ? 'priority-request' : 'priority-balanced';
      trace(label, 'start');
      try {
        if (testPriorityRejection) throw Error('Explicit unsupported-hint fault injection');
        await globalThis.Capacitor.nativePromise('BluetoothLe', 'requestConnectionPriority', {
          deviceId, connectionPriority: value,
        });
        trace(label, 'resolved');
      } catch { trace(label, 'rejected'); } // An optional hint cannot reject authentication/control.
    };
    const wrap = (object, name, targetArgument, label = name, onResult = value => value) => {
      const original = object[name];
      if (typeof original !== 'function') throw Error('Measurement method missing');
      const hadOwn = Object.hasOwn(object, name);
      const wrapped = function (...args) {
        if (targetArgument && args[0] !== id) return original.apply(this, args);
        trace(label, 'start');
        try {
          return Promise.resolve(original.apply(this, args)).then(value => {
            trace(label, 'resolved'); return onResult(value, args);
          }, error => { trace(label, 'rejected'); throw error; });
        } catch (error) { trace(label, 'rejected'); throw error; }
      };
      object[name] = wrapped;
      gate.restore.push(() => {
        if (object[name] !== wrapped) throw Error('Measurement ownership changed');
        if (hadOwn) object[name] = original; else delete object[name];
      });
    };
    wrap(ui, 'connectCloud', true);
    wrap(ui, 'sendCommand', true, 'command', (value, args) => {
      gate.commandAcks.push({ key: args[1], millis: Math.round(performance.now() - gate.started) });
      return value;
    });
    wrap(ui.ble, 'ensureReady', true);
    if (measureBlePhases) {
      wrap(ui.ble, 'discoverDirect', false, 'discovery');
      const original = ui.ble.client, hadOwn = Object.hasOwn(ui.ble, 'client');
      const factory = function (link, ...args) {
        const client = original.call(this, link, ...args);
        wrap(link, 'connect', false, 'gatt', async (value, args) => {
          if (testConnectionPriority !== undefined && !gate.exiting) {
            // Explicit, connection-local Android experiment. No MTU, packet,
            // authentication, wait interval or persistent configuration changes.
            const deviceId = args[0].device.deviceId;
            let balanced;
            gate.balance = () => balanced ??= priority(deviceId, 0);
            await priority(deviceId, 1);
          }
          return value;
        });
        wrap(client, 'authenticate', false, 'authentication', session => {
          if (gate.exiting) return session;
          const channel = session.channel, send = channel.send, ownSend = Object.hasOwn(channel, 'send');
          const measureSend = function (frame) {
            gate.events.push({ name: 'frame-send', kind: frame.kind, bodyBytes: frame.body.length,
              millis: Math.round(performance.now() - gate.started) });
            return send.call(this, frame);
          };
          channel.send = measureSend;
          gate.restore.push(() => {
            if (channel.send !== measureSend) throw Error('Measurement channel ownership changed');
            if (ownSend) channel.send = send; else delete channel.send;
          });
          wrap(session, 'synchronize', false, 'manifest-state');
          wrap(session, 'synchronizeManifest', false, 'manifest');
          wrap(session, 'acceptManifest', false, 'manifest-accept', value => {
            const state = session.store.snapshot(id);
            gate.events.push({ name: 'manifest-ack-state', accepted: state.manifestAccepted, fresh: state.stateFresh,
              millis: Math.round(performance.now() - gate.started) });
            return value;
          });
          wrap(session, 'synchronizeState', false, 'state');
          return session;
        });
        return client;
      };
      ui.ble.client = factory;
      gate.restore.push(() => {
        if (ui.ble.client !== factory) throw Error('Measurement factory ownership changed');
        if (hadOwn) ui.ble.client = original; else delete ui.ble.client;
      });
    }
    await root.viewService.router.navigateByUrl('/device/' + id);
    if (mode === 'cancel') {
      let phaseReached = false, phaseTimer;
      if (cancelAtPhase) {
        phaseReached = await Promise.race([phase.then(() => true),
          new Promise(resolve => { phaseTimer = setTimeout(() => resolve(false), 8000); })]);
        clearTimeout(phaseTimer);
      } else await sleep(150);
      gate.exiting = true;
      const exitAt = performance.now();
      await root.viewService.router.navigateByUrl('/home/device');
      const exitMillis = Math.round(performance.now() - exitAt);
      await sleep(100);
      const shortlyAfterExit = snapshot();
      await sleep(6400);
      // Native disconnect ends this connection's hint; do not issue a late
      // BALANCED request against a possible successor GATT connection.
      const result = { ...snapshot(), exitMillis, cancelAtPhase, phaseReached,
        cancelFailed: !!cancelAtPhase && !phaseReached, shortlyAfterExit,
        testPriorityRejection: gate.testPriorityRejection,
        events: [...gate.events], sameAccountSession: gate.session === ui.deviceV2.client.session };
      for (const restore of gate.restore.reverse()) restore();
      delete globalThis.__blinkerFirstConnectGate;
      return result;
    }
    for (let attempt = 0; attempt < 200; attempt++) {
      const current = page();
      if (current?.logicalDeviceId === id && current.canControl && current.snapshot.stateFresh
        && current.snapshot.manifestAccepted && ui.transport(id).value === expectedTransport) {
        const readyBeforeBalanceMillis = Math.round(performance.now() - gate.started);
        await gate.balance?.();
        return { ...snapshot(), readyBeforeBalanceMillis, readyMillis: Math.round(performance.now() - gate.started),
          testPriorityRejection: gate.testPriorityRejection,
          events: [...gate.events], sameAccountSession: gate.session === ui.deviceV2.client.session };
      }
      await sleep(50);
    }
    await gate.balance?.();
    return { ...snapshot(), openFailed: true, events: [...gate.events] };
  }
  if (!gate || gate.id !== id) throw Error('Matching open measurement required');
  if (mode === 'observe') {
    const samples = [];
    for (let n = 0; n < 14; n++) { samples.push(snapshot()); await sleep(500); }
    return { samples, events: [...gate.events], sameAccountSession: gate.session === ui.deviceV2.client.session };
  }
  if (mode === 'commands') {
    const p = page();
    if (p?.logicalDeviceId !== id || !p.canControl || !p.snapshot.stateFresh || !p.snapshot.manifestAccepted
      || gate.commandsAttempted || ui.transport(id).value !== expectedTransport) throw Error('Ready once-only command gate required');
    gate.commandsAttempted = true;
    const fields = p.snapshot.endpoints, power = fields.find(f => f.key === 'power');
    const button = fields.find(f => f.key === 'button'), count = fields.find(f => f.key === 'buttonPresses');
    if (!power || !button || !count) throw Error('Standard example fields required');
    const before = snapshot(), oldPower = power.value, oldCount = count.value;
    const ackCount = gate.commandAcks.length;
    let at = performance.now();
    await p.setBoolean(power, { detail: { checked: !oldPower } });
    const propertyMillis = Math.round(performance.now() - at);
    const propertyAcknowledged = gate.commandAcks.length === ackCount + 1
      && gate.commandAcks.at(-1)?.key === 'power' && !p.fieldError?.(power);
    if (!propertyAcknowledged) return { ...snapshot(), before, propertyAcknowledged, commandsFailed: true };
    at = performance.now(); await p.execute(button);
    const actionMillis = Math.round(performance.now() - at);
    await sleep(500);
    const after = snapshot(), value = key => after.fields.find(f => f.key === key)?.value;
    const actionAcknowledged = gate.commandAcks.length === ackCount + 2
      && gate.commandAcks.at(-1)?.key === 'button' && !p.fieldError?.(button);
    return { ...after, before, propertyMillis, actionMillis, propertyAcknowledged, actionAcknowledged,
      commandsFailed: !actionAcknowledged || !after.ready || value('power') !== !oldPower || value('buttonPresses') !== oldCount + 1,
      events: [...gate.events], sameAccountSession: gate.session === ui.deviceV2.client.session };
  }
  if (mode === 'close') {
    const at = performance.now();
    gate.exiting = true;
    await root.viewService.router.navigateByUrl('/home/device');
    for (let n = 0; n < 60 && (ui.connections.size || ui.ble.accountLinks.size || ui.ble.opening); n++) await sleep(100);
    const result = { ...snapshot(), cleanupMillis: Math.round(performance.now() - at),
      events: [...gate.events], sameAccountSession: gate.session === ui.deviceV2.client.session };
    for (const restore of gate.restore.reverse()) restore();
    delete globalThis.__blinkerFirstConnectGate;
    return result;
  }
  throw Error('Unsupported measurement mode');
}
