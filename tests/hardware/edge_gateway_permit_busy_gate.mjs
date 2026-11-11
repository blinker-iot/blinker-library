// Run inside a debug WebView after multiChildAppAction('start'). No candidate
// selection, grant delivery, topology mutation, or Action retry is performed.
export async function permitBusyAppAction(input) {
  const must = (value, code) => { if (!value) throw Error(code); };
  const gate = globalThis.__blinkerMultiChildGate;
  const service = globalThis.ng?.getComponent(document.querySelector('app-root'))?.deviceService?.deviceUi?.deviceV2;
  must(service && gate && !gate.closed && input.children?.length === 2
    && gate.ids.every((id, index) => id === input.children[index])
    && gate.session === service.client.session, 'GATE_OWNERS');
  must(typeof input.hub === 'string' && /^device_[a-f0-9-]{36}$/.test(input.hub)
    && /^[A-Za-z0-9_-]{22}$/.test(input.operation), 'GATE_PERMIT_IDENTITY');
  must(!gate.actions.has('permit-busy'), 'GATE_PERMIT_ONCE');
  const snapshot = () => ({ sameAccountMqtt: gate.session === service.client.session,
    children: gate.ids.map((id, index) => {
      const value = service.snapshot(id);
      return { id, active: !gate.controllers[index].signal.aborted,
        ready: value.cloudReachable === true && value.stateFresh && value.manifestAccepted,
        counter: value.values.buttonPresses?.value };
    }) });
  const ready = value => value.sameAccountMqtt && value.children.every(child => child.active && child.ready);
  const before = snapshot();
  must(ready(before), 'GATE_CHILDREN_NOT_READY');
  const session = await Capacitor.Plugins.SecureStorage.get('session');
  must(session?.accessToken, 'GATE_LOGIN');
  const api = input.api || 'http://127.0.0.1:7000';
  const root = '/api/v2/edge-gateway/permit-joins', item = `${root}/${input.operation}`;
  const request = async (path, method = 'GET', body) => {
    const response = await fetch(api + path, { method, signal: AbortSignal.timeout(5000),
      headers: { Authorization: 'Bearer ' + session.accessToken, 'Content-Type': 'application/json',
        ...(path === root ? { 'Idempotency-Key': input.operation } : {}) },
      body: body === undefined ? undefined : JSON.stringify(body) });
    must(response.ok, 'GATE_PERMIT_HTTP');
    const data = (await response.json()).data;
    must(data?.operationId === input.operation && data.edgeHubLogicalDeviceId === input.hub, 'GATE_PERMIT_RESPONSE');
    return { state: data.state }; // Never export credentials or opaque relay payloads.
  };
  gate.actions.add('permit-busy'); // An unknown HTTP outcome may only be read/closed, never re-opened here.
  let opened, terminal, after, closeConfirmed = false;
  try {
    opened = await request(root, 'POST', { edgeHubLogicalDeviceId: input.hub, adapterId: 1 });
    terminal = opened;
    const deadline = performance.now() + 4000;
    while (terminal.state === 'pending_open' && performance.now() < deadline) {
      await new Promise(resolve => setTimeout(resolve, 100));
      terminal = await request(item);
    }
    must(terminal.state === 'busy', 'GATE_EXPECTED_BUSY');
    after = snapshot();
    must(ready(after), 'GATE_CHILD_INTERRUPTED');
  } finally {
    // Set up cleanup before the Open request; its response may have been lost.
    // Close addresses only this random test operation, never an existing one.
    const result = await request(`${item}/close`, 'POST', {});
    closeConfirmed = ['closed', 'expired', 'rejected', 'busy', 'unsupported'].includes(result.state);
    if (!closeConfirmed) {
      const deadline = performance.now() + 4000;
      while (!closeConfirmed && performance.now() < deadline) {
        const current = await request(item);
        closeConfirmed = ['closed', 'expired', 'rejected', 'busy', 'unsupported'].includes(current.state);
        if (!closeConfirmed) await new Promise(resolve => setTimeout(resolve, 100));
      }
    }
    must(closeConfirmed, 'GATE_PERMIT_CLOSE_PENDING');
  }
  return { operation: input.operation, opened: opened.state, terminal: terminal.state, closeConfirmed, before, after };
}
