// Debug-WebView gate using the public Cloud API; not two visible UI pages.
// One account MQTT, bounded owners, no retries of Commands with unknown outcomes.
export async function multiChildAppAction(input) {
  const must = (value, code) => { if (!value) throw Error(code); };
  const service = globalThis.ng?.getComponent(document.querySelector('app-root'))?.deviceService?.deviceUi?.deviceV2;
  must(service && Array.isArray(input.children) && input.children.length === 2
    && new Set(input.children).size === 2 && input.children.every(id => /^ble_[A-Za-z0-9_-]{22}$/.test(id)), 'GATE_CHILDREN');
  let gate = globalThis.__blinkerMultiChildGate;
  if (input.action === 'start' || input.action === 'start-release-a') {
    must(!gate || gate.closed, 'GATE_ALREADY_RUNNING');
    gate = { ids: [...input.children], controllers: input.children.map(() => new AbortController()),
      owners: [], actions: new Set(), closed: false };
    globalThis.__blinkerMultiChildGate = gate;
    gate.close = () => {
      if (gate.closed) return;
      gate.closed = true; clearTimeout(gate.timer);
      gate.controllers.forEach(controller => controller.abort());
      gate.owners.forEach(owner => owner?.release());
      gate.directController?.abort(); gate.directOwner?.release();
    };
    gate.timer = setTimeout(gate.close, 90000);
    try {
      await service.start();
      gate.session = service.client.session; // Equality witness only; never fabricate owners or readiness.
      await Promise.all(gate.ids.map(async (id, index) => {
        const signal = gate.controllers[index].signal;
        const owner = await service.acquireConnectionDemand(id, signal);
        gate.owners[index] = owner; await owner.accepted;
        await service.waitUntilReady(id, signal);
      }));
    } catch (error) { gate.close(); throw error; }
  }
  must(gate && gate.ids.every((id, index) => id === input.children[index]), 'GATE_IDENTITY');
  const snapshot = () => ({ closed: gate.closed, sameAccountMqtt: gate.session === service.client.session,
    children: gate.ids.map((id, index) => {
      const state = service.snapshot(id);
      return {id, ownerReleased:gate.controllers[index].signal.aborted, reachable:state.cloudReachable,
        manifest:state.manifestAccepted, fresh:state.stateFresh, counter:state.values.buttonPresses?.value};
    }) });
  if (input.action === 'close') { gate.close(); await Promise.all(gate.owners.map(owner => owner.closed)); }
  else if (input.action === 'release-a' || input.action === 'start-release-a') {
    gate.controllers[0].abort(); gate.owners[0].release(); await gate.owners[0].closed;
  } else if (input.action === 'direct-a') {
    must(!gate.closed && gate.session === service.client.session && !gate.actions.has(input.action)
      && gate.controllers[0].signal.aborted && !gate.controllers[1].signal.aborted, 'GATE_DIRECT_PRECONDITION');
    gate.actions.add(input.action);
    gate.directController = new AbortController();
    const before = snapshot();
    try {
      const owner = await service.reserveDirectPriority(gate.ids[0], gate.directController.signal);
      gate.directOwner = owner;
      await owner.accepted; await owner.ready; owner.assertReady();
      const yielded = snapshot();
      must(yielded.sameAccountMqtt && yielded.children[1].fresh && yielded.children[1].reachable === true
        && Number.isInteger(yielded.children[1].counter), 'GATE_SIBLING_NOT_READY');
      await service.command(gate.ids[1], 'button', null); // Once-only; no command to A and no physical phone ACL.
      const after = snapshot();
      must(after.children[1].counter === yielded.children[1].counter + 1, 'GATE_COUNTER');
      // Allow the external serial witness to observe one native link. This
      // proves exact Hub yielding, not a completed phone BLE authentication.
      await new Promise(resolve => setTimeout(resolve, 1200));
      owner.assertReady();
      return { physicalYieldReady: true, before, yielded, after, acknowledged: 1 };
    } finally {
      gate.directController.abort(); gate.directOwner?.release();
      await gate.directOwner?.closed;
    }
  } else if (input.action === 'commands' || input.action === 'b-after-a') {
    must(!gate.closed && gate.session === service.client.session && !gate.actions.has(input.action), 'GATE_COMMAND_ONCE');
    const indexes = input.action === 'commands' ? [0, 1] : [1];
    const before = snapshot();
    must(indexes.every(i => !gate.controllers[i].signal.aborted && before.children[i].fresh
      && before.children[i].reachable === true && Number.isInteger(before.children[i].counter)), 'GATE_READY');
    gate.actions.add(input.action); // Mark before awaiting so a lost CDP response cannot duplicate the Action.
    const acks = await Promise.all(indexes.map(i => service.command(gate.ids[i], 'button', null)));
    const after = snapshot();
    must(indexes.every(i => after.children[i].counter === before.children[i].counter + 1), 'GATE_COUNTER');
    return {before, after, acknowledged:acks.length};
  } else must(['start','status'].includes(input.action), 'GATE_ACTION');
  return snapshot();
}
