import { runGatewayAppGate } from './gateway_app_gate.mjs';

// One bounded witness on the App's existing account MQTT. No native connection,
// credentials, automatic Action retry, page-layout changes or broker shortcuts.
export async function runCloudWitnessAppGate(mode, logicalDeviceId) {
  if (!['open', 'inspect', 'sample', 'action', 'start', 'close'].includes(mode)
    || !/^(?:device_[0-9a-f-]{36}|ble_[A-Za-z0-9_-]{22})$/.test(logicalDeviceId)) throw new Error('Invalid witness request');
  return runGatewayAppGate(cloudWitnessAppAction, { pagePath: '/home/device', mode, logicalDeviceId }, 35000);
}

// Self-contained for CDP serialization and isolated lifecycle tests.
export async function cloudWitnessAppAction({ mode, logicalDeviceId: id }) {
    const root = globalThis.ng?.getComponent(document.querySelector('app-root'));
    const cloud = root?.deviceService?.deviceUi?.deviceV2;
    if (!cloud || !root.dataService.auth?.uuid) throw new Error('Signed-in debug App required');
    const values = snapshot => Object.fromEntries(['power', 'buttonPresses']
      .filter(key => snapshot.values[key]).map(key => [key, snapshot.values[key].value]));
    let gate = globalThis.__blinkerCloudWitness;
    if (gate && gate.id !== id && (mode !== 'open' || !gate.owner.signal.aborted))
      throw new Error('Another witness owns this gate');
    if (mode === 'open') {
      if (gate && !gate.owner.signal.aborted) throw new Error('Close existing witness first');
      await root.userService.getAllInfo();
      const owner = new AbortController();
      gate = globalThis.__blinkerCloudWitness = { id, owner, phase: 'opening', samples: [], states: [],
        openedAt: Date.now(), session: cloud.client.session, actionSent: false, sampling: false };
      const stop = () => {
        if (owner.signal.aborted) return;
        owner.abort(); gate.interest?.release(); gate.unwatch?.();
        clearTimeout(gate.next); clearTimeout(gate.deadline); gate.running = false;
      };
      gate.stop = stop;
      gate.unwatch = cloud.store.subscribe((deviceId, snapshot) => {
        if (deviceId !== id || owner.signal.aborted || !snapshot.stateFresh) return;
        gate.states.push({ at: Date.now(), revision: snapshot.stateRevision, values: values(snapshot) });
        if (gate.states.length > 64) gate.states.shift();
      });
      gate.deadline = setTimeout(() => { gate.phase = 'deadline-closed'; stop(); }, 120000);
      try {
        gate.interest = await cloud.acquireStateInterest(id, owner.signal);
        void gate.interest?.closed.then(() => {
          if (!owner.signal.aborted) { gate.phase = 'observation-closed'; stop(); }
        });
        await gate.interest?.accepted;
        await cloud.waitUntilReady(id, owner.signal);
        owner.signal.throwIfAborted();
        gate.phase = 'ready';
      } catch (error) {
        gate.phase = 'open-failed';
        gate.failure = { name: ['AbortError', 'DeviceV2RouteError', 'DeviceV2DemandError', 'DeviceV2ReadyWaitError'].includes(error?.name)
          ? error.name : 'Error', code: Number.isInteger(error?.code) && error.code > 0 && error.code <= 65535 ? error.code : undefined };
        stop(); throw error;
      }
    }
    if (!gate) throw new Error('Open witness first');
    const requireReady = () => {
      gate.owner.signal.throwIfAborted();
      if (gate.phase !== 'ready' || gate.session !== cloud.client.session || cloud.state.value !== 'ready')
        throw new Error('Original ready account required');
    };
    const sample = async () => {
      requireReady();
      if (gate.sampling || gate.samples.length >= 24) throw new Error('Witness capacity');
      gate.sampling = true;
      const requested = gate.samples.length % 2 === 0, startedAt = Date.now();
      try {
        await cloud.command(id, 'power', requested);
        gate.owner.signal.throwIfAborted();
        const snapshot = cloud.snapshot(id);
        gate.samples.push({ startedAt, ackAt: Date.now(), requested,
          sameAccountSession: gate.session === cloud.client.session,
          cloudState: cloud.state.value, stateRevision: snapshot.stateRevision });
      } finally { gate.sampling = false; }
    };
    if (mode === 'sample') {
      if (gate.running) throw new Error('Periodic witness already active');
      await sample();
    }
    if (mode === 'action') {
      requireReady();
      if (gate.actionSent || gate.owner.signal.aborted) throw new Error('Action already attempted');
      gate.actionSent = true; // Latch before send; never retry uncertain execution.
      gate.actionStartedAt = Date.now();
      await cloud.command(id, 'button', null);
      gate.actionAckAt = Date.now();
      gate.actionAcknowledged = true;
    }
    if (mode === 'start') {
      if (gate.running || gate.owner.signal.aborted) throw new Error('Witness already running or closed');
      gate.running = true;
      const tick = async () => {
        if (gate.owner.signal.aborted || gate.samples.length >= 24) { gate.running = false; return; }
        try { await sample(); }
        catch { if (!gate.owner.signal.aborted) gate.phase = 'sample-failed'; gate.running = false; return; }
        if (!gate.owner.signal.aborted)
          gate.next = setTimeout(tick, 1000); // Bounded sequential writes, not an unbounded loop producer.
      };
      void tick();
    }
    if (mode === 'close') {
      gate.stop(); clearTimeout(gate.deadline); gate.running = false; gate.phase = 'closed';
    }
    const snapshot = cloud.snapshot(id);
    return { phase: gate.phase, failure: gate.failure, openedAt: gate.openedAt, samples: gate.samples, states: gate.states,
      actionAcknowledged: gate.actionAcknowledged === true, aborted: gate.owner.signal.aborted,
      actionAckMillis: gate.actionAckAt === undefined ? undefined : gate.actionAckAt - gate.actionStartedAt,
      cloudState: cloud.state.value, sameAccountSession: gate.session === cloud.client.session,
      manifestAccepted: snapshot.manifestAccepted, stateFresh: snapshot.stateFresh,
      stateRevision: snapshot.stateRevision, values: values(snapshot),
      manifestFingerprint: snapshot.manifest?.fingerprint };
}
