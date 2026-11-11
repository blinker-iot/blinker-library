// Debug-App measurement only: delegates every operation to the real implementation.
// No synthetic credentials, readiness, HTTP responses or automatic Action replay.
export async function firstConnectAppGate(input) {
  // Retired experiments awaited an unbounded native hint. Never silently
  // accept them as measurement of the production-owned bounded window.
  if ('testConnectionPriority' in input || 'testPriorityRejection' in input)
    throw Error('Legacy BLE priority experiment removed; use the existing bounded bootstrap window');
  const { mode, logicalDeviceId: id, ownerId, expectedTransport = 'cloud', measureBlePhases = false,
    useCachedAccountForBle = false, requireCloudCapable = true, cancelAtPhase, eventLimit = 512 } = input;
  if (!Number.isInteger(eventLimit) || eventLimit < 16 || eventLimit > 2048) throw Error('Invalid measurement event limit');
  if (typeof requireCloudCapable !== 'boolean' || (!requireCloudCapable
    && (expectedTransport !== 'ble' || !measureBlePhases))) throw Error('Pure BLE requires explicit measured BLE mode');
  if (cancelAtPhase !== undefined && (mode !== 'cancel' || !measureBlePhases || expectedTransport !== 'ble'
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
      events: [], eventsDropped: 0, restore: [], active: true, commandsAttempted: false, exiting: false,
      commandAcks: [],
    };
    let reachedPhase;
    const phase = new Promise(resolve => { reachedPhase = resolve; });
    const trace = (name, state, facts = {}) => {
      if (!gate.active || gate.exiting) return;
      if (gate.events.length < eventLimit) gate.events.push({ name, state, ...facts, millis: Math.round(performance.now() - gate.started) });
      else gate.eventsDropped++;
      if (name === cancelAtPhase && state === 'start') reachedPhase();
    };
    // Exact native scanner categories only, never exception text or IDs. Data
    // descriptors avoid invoking arbitrary getters on an unknown rejection.
    const failureCategory = error => {
      const dataString = key => {
        try {
          const field = error && Object.getOwnPropertyDescriptor(error, key);
          return field && typeof field.value === 'string' ? field.value : undefined;
        } catch { return undefined; }
      };
      const categories = {
        BLE_SCAN_BACKGROUND: 'scan-background', BLE_SCAN_BUSY: 'scan-busy',
        BLE_SCAN_BUDGET_INVALID: 'scan-budget-invalid', BLE_SCAN_UNAVAILABLE: 'scan-unavailable',
        BLE_SCAN_BUDGET_STORAGE_FAILED: 'scan-budget-storage-failed', BLE_SCAN_THROTTLED: 'scan-throttled',
        BLE_SCAN_PERMISSION: 'scan-permission', BLE_SCAN_FAILED: 'scan-failed',
        BLE_SCAN_CLOSE_FAILED: 'scan-close-failed', BLE_SCAN_DEADLINE: 'scan-deadline',
        BLE_SCAN_REGISTRATION_FAILED: 'scan-registration-failed', BLE_SCAN_UNSUPPORTED: 'scan-unsupported',
        BLE_SCAN_RESOURCES: 'scan-resources', BLE_SCAN_CANCELLED: 'cancelled',
        BLE_DIRECT_SCAN_CANCELLED: 'cancelled', BLE_DIRECT_CONNECT_CANCELLED: 'cancelled',
        BLE_DIRECT_SCAN_TIMEOUT: 'scan-timeout',
      };
      const message = dataString('message');
      if (Object.hasOwn(categories, message)) return categories[message];
      if (dataString('name') === 'AbortError') return 'cancelled';
      try {
        // Use the built-in accessor, never an unknown object's name getter.
        const name = typeof DOMException === 'function'
          && Object.getOwnPropertyDescriptor(DOMException.prototype, 'name')?.get;
        if (name && name.call(error) === 'AbortError') return 'cancelled';
      } catch { /* Non-DOMException rejection. */ }
      return 'unknown';
    };
    // Observational settlement must not replace a Promise, inspect a payload,
    // or make an observation failure reject the actual operation.
    const settlement = (result, resolved, rejected) => {
      try {
        const then = result?.then;
        if (typeof then === 'function') then.call(result, resolved, rejected);
        else resolved(result);
      } catch { trace('measurement-observation', 'unavailable'); }
    };
    // Connection-local observation, including synchronous carrier facts. Keep
    // the exact original result/promise and never inspect record bodies.
    const observe = (object, name, label, onStart = () => {}, onResult = () => {}, selected = () => true) => {
      let original;
      try { original = object?.[name]; } catch { return false; }
      if (typeof original !== 'function') return false; // Report coverage, do not invent a phase.
      const hadOwn = Object.hasOwn(object, name);
      const wrapped = function (...args) {
        if (!selected(args)) return original.apply(this, args);
        const started = performance.now();
        let facts = {};
        if (gate.active && !gate.exiting) {
          try { facts = onStart(args) ?? {}; } catch { trace('measurement-observation', 'unavailable'); }
          trace(label, 'start', facts);
        }
        const rejected = error => {
          if (!gate.active || gate.exiting) return;
          trace(label, 'rejected', {
            ...facts, durationMs: Math.round(performance.now() - started), failureCategory: failureCategory(error),
          });
        };
        let result;
        try { result = original.apply(this, args); }
        catch (error) { rejected(error); throw error; }
        const resolved = value => {
          if (!gate.active || gate.exiting) return;
          trace(label, 'resolved', { ...facts, durationMs: Math.round(performance.now() - started) });
          try {
            const observed = onResult(value, args);
            // A future asynchronous observation failure is also only missing
            // coverage. It must not become an unhandled rejection.
            if (observed !== undefined) settlement(observed, () => {}, () => trace('measurement-observation', 'unavailable'));
          } catch { trace('measurement-observation', 'unavailable'); }
        };
        if (gate.active && !gate.exiting) settlement(result, resolved, rejected);
        return result;
      };
      object[name] = wrapped;
      gate.restore.push(() => {
        if (object[name] !== wrapped) throw Error('Measurement ownership changed');
        if (hadOwn) object[name] = original; else delete object[name];
      });
      return true;
    };
    const wrap = (object, name, targetArgument, label = name, onResult = () => {}) => {
      if (!observe(object, name, label, undefined, onResult, args => !targetArgument || args[0] === id))
        throw Error('Measurement method missing');
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
        if (!gate.active || gate.exiting) return client;
        let authenticating = true, authWrites = 0, pendingRecord, preparingPackets = false;
        const bootstrapPhase = record => {
          if (record?.length >= 10 && record[0] === 0x42 && record[1] === 0x4b
            && record[2] === 2 && record[5] === 10) {
            if (record[3] === 1) return 'hello';
            if (record[3] === 4) return ++authWrites === 1 ? 'method2-challenge' : 'method2-authorize';
          }
          return preparingPackets ? 'authenticated-hello' : 'unknown';
        };
        const recordSend = observe(link, 'sendRecord', 'record-send', recordArgs => {
          if (!authenticating || gate.exiting) return;
          pendingRecord = bootstrapPhase(recordArgs[0]);
          const recordBytes = recordArgs[0]?.byteLength;
          const facts = { phase: pendingRecord };
          if (Number.isInteger(recordBytes) && recordBytes >= 1 && recordBytes <= 1048) {
            const estimatedFragments = Math.ceil(recordBytes / 16);
            Object.assign(facts, { recordBytes, packetBytes: 20, estimatedFragments,
              estimatedInterFragmentWaitMs: (estimatedFragments - 1) * 30 });
          }
          trace('bootstrap-record', 'send', facts);
          return facts;
        });
        const recordReceive = observe(link, 'receiveRecord', 'record-receive', () =>
          authenticating && pendingRecord && !gate.exiting ? { phase: pendingRecord } : {}, record => {
          if (!authenticating || !pendingRecord || gate.exiting) return;
          // Receipt of bytes is not a validated proof. authenticate:resolved
          // remains the only full authentication-success observation.
          const recordBytes = record?.byteLength;
          trace('bootstrap-record', 'received', { phase: pendingRecord,
            ...(Number.isInteger(recordBytes) && recordBytes >= 1 && recordBytes <= 1048 ? { recordBytes } : {}) });
          pendingRecord = undefined;
        });
        const mtu = observe(link, 'maxPacketSize', 'mtu-probe', () => {}, size => {
          if (Number.isInteger(size) && size >= 20 && size <= 244)
            trace('packet-capacity', 'observed', { packetBytes: size });
        });
        const packetPrepare = observe(link, 'preparePacketNegotiation', 'packet-prepare', () => { preparingPackets = true; });
        const packetApply = observe(link, 'applyPacketSize', 'packet-apply', () => {}, (_, packetArgs) => {
          const size = packetArgs[0];
          if (Number.isInteger(size) && size >= 20 && size <= 244)
            trace('packet-accepted', 'observed', { packetBytes: size });
        });
        trace('phase-coverage', 'observed', { bootstrapRecords: recordSend && recordReceive,
          mtuProbe: mtu, packetNegotiation: packetPrepare && packetApply, noise: 'not-used-on-bound-direct-connect' });
        wrap(link, 'connect', false, 'gatt');
        wrap(client, 'authenticate', false, 'authentication', session => {
          authenticating = false;
          if (gate.exiting) return session;
          let manifestRequests = 0, cachedMatch = false;
          // Observe the real account-bound load exactly once when the session
          // requests it. Never read it early or retain the Manifest/fingerprint.
          const cache = session.manifestCache;
          trace('manifest-cache-hint', 'observed', { available: !!cache });
          observe(cache, 'load', 'manifest-cache-load', () => {}, candidate => {
            cachedMatch = !!candidate && candidate.revision === cache.hello.revision
              && candidate.fingerprint === cache.hello.fingerprint;
            trace('manifest-cache-candidate', 'observed', { present: !!candidate, matchesHello: cachedMatch });
          });
          const channel = session.channel, send = channel.send, ownSend = Object.hasOwn(channel, 'send');
          const measureSend = function (frame) {
            if (gate.active && !gate.exiting) {
              try {
                if (frame.kind === 2) manifestRequests++;
                trace('frame-send', 'start', { kind: frame.kind, bodyBytes: frame.body.length });
              } catch { trace('measurement-observation', 'unavailable'); }
            }
            return send.call(this, frame);
          };
          channel.send = measureSend;
          gate.restore.push(() => {
            if (channel.send !== measureSend) throw Error('Measurement channel ownership changed');
            if (ownSend) channel.send = send; else delete channel.send;
          });
          wrap(session, 'synchronize', false, 'manifest-state');
          wrap(session, 'synchronizeManifest', false, 'manifest', value => {
            const accepted = session.store.snapshot(id).manifestAccepted === true;
            trace('manifest-cache-result', 'observed', { accepted, requests: manifestRequests,
              outcome: manifestRequests > 0 ? (cachedMatch ? 'fetch-after-cache' : 'fetch')
                : cachedMatch && accepted ? 'hit' : 'unobserved' });
            return value;
          });
          wrap(session, 'acceptManifest', false, 'manifest-accept', value => {
            const state = session.store.snapshot(id);
            trace('manifest-ack-state', 'observed', { accepted: state.manifestAccepted === true, fresh: state.stateFresh === true });
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
      // Native disconnect and the production window own cancellation. This
      // observer never restores priority or requests a hint on a successor.
      const result = { ...snapshot(), exitMillis, cancelAtPhase, phaseReached,
        cancelFailed: !!cancelAtPhase && !phaseReached, shortlyAfterExit,
        events: [...gate.events], eventsDropped: gate.eventsDropped, sameAccountSession: gate.session === ui.deviceV2.client.session };
      gate.active = false;
      for (const restore of gate.restore.reverse()) restore();
      delete globalThis.__blinkerFirstConnectGate;
      return result;
    }
    for (let attempt = 0; attempt < 200; attempt++) {
      const current = page();
      if (current?.logicalDeviceId === id && current.canControl && current.snapshot.stateFresh
        && current.snapshot.manifestAccepted && ui.transport(id).value === expectedTransport) {
        trace('page-ready', 'observed', { manifestAccepted: true, stateFresh: true, transport: expectedTransport });
        return { ...snapshot(), readyMillis: Math.round(performance.now() - gate.started),
          events: [...gate.events], eventsDropped: gate.eventsDropped, sameAccountSession: gate.session === ui.deviceV2.client.session };
      }
      await sleep(50);
    }
    return { ...snapshot(), openFailed: true, events: [...gate.events], eventsDropped: gate.eventsDropped };
  }
  if (!gate || gate.id !== id) throw Error('Matching open measurement required');
  if (mode === 'observe') {
    const samples = [];
    for (let n = 0; n < 14; n++) { samples.push(snapshot()); await sleep(500); }
    return { samples, events: [...gate.events], eventsDropped: gate.eventsDropped, sameAccountSession: gate.session === ui.deviceV2.client.session };
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
      events: [...gate.events], eventsDropped: gate.eventsDropped, sameAccountSession: gate.session === ui.deviceV2.client.session };
  }
  if (mode === 'close') {
    const at = performance.now();
    gate.exiting = true;
    await root.viewService.router.navigateByUrl('/home/device');
    for (let n = 0; n < 60 && (ui.connections.size || ui.ble.accountLinks.size || ui.ble.opening); n++) await sleep(100);
    const result = { ...snapshot(), cleanupMillis: Math.round(performance.now() - at),
      events: [...gate.events], eventsDropped: gate.eventsDropped, sameAccountSession: gate.session === ui.deviceV2.client.session };
    gate.active = false;
    for (const restore of gate.restore.reverse()) restore();
    delete globalThis.__blinkerFirstConnectGate;
    return result;
  }
  throw Error('Unsupported measurement mode');
}
