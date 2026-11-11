import { runGatewayAppGate } from './gateway_app_gate.mjs';

// A bounded debug-App gate over the real page port. Never extracts auth/PSK,
// edits a page layout, changes routing policy or retries an uncertain Action.
export async function runLanSelfAppGate(mode, logicalDeviceId) {
  if (!['open', 'inspect', 'control', 'retired', 'close'].includes(mode)
    || !/^device_[0-9a-f-]{36}$/.test(logicalDeviceId)) throw new Error('Invalid self gate request');
  const snapshot = await runGatewayAppGate(async ({ mode, logicalDeviceId: id }) => {
    const root = globalThis.ng?.getComponent(document.querySelector('app-root'));
    const ui = root?.deviceService?.deviceUi;
    if (!ui?.lan || !root?.dataService?.auth?.uuid) throw new Error('Signed-in LAN debug App required');
    let gate = globalThis.__blinkerLanSelfGate;
    if (gate && gate.id !== id) throw new Error('Another self gate owns the page');
    if (mode === 'open') {
      if (gate && !gate.page.signal.aborted) throw new Error('Inspect or close existing gate first');
      await root.userService.getAllInfo();
      if (!ui.lan.available(id)) throw new Error('Target not admitted to LAN');
      const page = ui.openConnection(id);
      // A new page owns its baseline; a previous run may belong to another account.
      const accountSession = ui.deviceV2.client.session;
      gate = globalThis.__blinkerLanSelfGate = { id, page, phase: 'opening', commands: false, timeline: [],
        openedAt: Date.now(), commandResults: [], accountSession };
      gate.watch = ui.watchConnectivity(id).subscribe(value => {
        gate.connectivity = value;
        gate.timeline.push({ at: Date.now(), transport: value.activeTransport, cloud: value.cloudSessionState });
        if (gate.timeline.length > 32) gate.timeline.shift();
      });
      // Keep the checkpoint even if CDP disconnects, but never leave the page
      // indefinitely occupied when an operator does not return to close it.
      gate.stateWatch = ui.watchState(id).subscribe(state => { gate.state = state; });
      gate.timer = setTimeout(() => {
        page.close(); gate.watch.unsubscribe(); gate.stateWatch.unsubscribe(); gate.phase = 'deadline-closed';
      }, 120000);
      void page.ready.then(() => { gate.phase = 'page-ready'; }, () => { gate.phase = 'page-failed'; });
    }
    if (!gate) throw new Error('Open the self gate first');
    // Keep only the session object (never credentials) to observe actual cleanup
    // after the route entry has been retired by an ACL update or page exit.
    gate.lanSession ??= ui.connections.get(id)?.lan?.session;
    if (mode === 'control') {
      if (gate.commands || gate.page.signal.aborted || gate.connectivity?.activeTransport !== 'lan') {
        throw new Error('Fresh LAN-ready gate required; uncertain commands cannot be replayed');
      }
      const requireLanControl = () => {
        if (gate.connectivity?.activeTransport !== 'lan' || ui.connections.get(id)?.lan?.permissions !== 3)
          throw new Error('LAN Control permission required; do not count a Cloud fallback as LAN evidence');
      };
      requireLanControl();
      // Latch before either send: CDP timeout/disconnection cannot cause a second Action.
      gate.commands = true;
      gate.phase = 'property-sending';
      const propertyStarted = Date.now();
      await ui.sendCommand(id, 'power', true);
      gate.commandResults.push({ key: 'power', ackMillis: Date.now() - propertyStarted,
        transportAtSend: 'lan', transportAfterAck: gate.connectivity?.activeTransport });
      requireLanControl();
      gate.phase = 'action-sending';
      // BLINKER_ACTION(button) is Action<void>: its wire value is CBOR null,
      // not the integer used by some typed-action fixtures.
      const actionStarted = Date.now();
      await ui.sendCommand(id, 'button', null);
      gate.commandResults.push({ key: 'button', ackMillis: Date.now() - actionStarted,
        transportAtSend: 'lan', transportAfterAck: gate.connectivity?.activeTransport });
      gate.phase = 'commands-acknowledged';
    }
    if (mode === 'retired') {
      if (gate.lanSession?.state !== 'closed') throw new Error('Original LAN session must already be closed');
      let rejected = false;
      try {
        await gate.lanSession.command('power', false);
      } catch { rejected = true; }
      if (!rejected) throw new Error('Closed LAN session accepted a command');
      gate.closedCommandRejected = true;
    }
    if (mode === 'close') {
      // scope.closed means caller retirement, not native socket completion.
      // Observe the existing session's barrier; never force-close a shared caller.
      const group = ui.connections.get(id);
      const soleLanOwner = gate.lanSession && group?.lan?.session === gate.lanSession
        && [...group.scopes].filter(scope => !scope.signal.aborted).length === 1;
      let cleanupTimer, removeClosed;
      const started = Date.now();
      const nativeReleased = soleLanOwner ? new Promise((resolve, reject) => {
        cleanupTimer = setTimeout(() => reject(new Error('LAN native release deadline')), 5000);
        removeClosed = gate.lanSession.subscribeClosed(() => {
          gate.nativeReleaseMillis = Date.now() - started; resolve();
        });
      }) : Promise.resolve();
      gate.page.close(); clearTimeout(gate.timer); gate.watch.unsubscribe(); gate.stateWatch.unsubscribe();
      try { await gate.page.closed; await nativeReleased; }
      finally { clearTimeout(cleanupTimer); removeClosed?.(); }
      gate.phase = 'page-closed';
    }
    const state = gate.state;
    return JSON.stringify({ id, phase: gate.phase, pageAborted: gate.page.signal.aborted, openedAt: gate.openedAt,
      commandResults: gate.commandResults,
      cloud: ui.deviceV2.state.value, sameAccountSession: gate.accountSession === ui.deviceV2.client.session,
      lanSessionState: gate.lanSession?.state, lanCallerOccupied: ui.lan.connector.occupied,
      nativeReleaseMillis: gate.nativeReleaseMillis,
      closedCommandRejected: gate.closedCommandRejected === true,
      connectivity: gate.connectivity, timeline: gate.timeline,
      manifestAccepted: state?.manifestAccepted ?? false, stateFresh: state?.stateFresh ?? false,
      endpoints: (state?.endpoints ?? []).map(v => ({ key: v.key, role: v.role, value: v.value })) });
  }, { pagePath: '/home/device', mode, logicalDeviceId }, 35000);
  return JSON.parse(snapshot);
}
