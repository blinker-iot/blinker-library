// Debug-WebView observer only. Never logs options, payloads, IDs, credentials,
// or raw errors, and never changes native operations or connection policy.
// Self-contained so runGatewayAppGate can serialize it into the WebView.
export function bleNativeTimingGate({ mode, limit = 512 }) {
  const key = '__blinkerBleNativeTiming';
  const cap = globalThis.Capacitor;
  let gate = globalThis[key];
  if (mode === 'start') {
    if (gate) throw Error('BLE timing observer already installed');
    if (!Number.isInteger(limit) || limit < 16 || limit > 2048
      || typeof cap?.nativePromise !== 'function') throw Error('BLE timing precondition');
    const original = cap.nativePromise;
    const hadOwn = Object.hasOwn(cap, 'nativePromise');
    const methods = new Set(['connect', 'disconnect', 'getServices', 'discoverServices',
      'startNotifications', 'stopNotifications', 'write', 'writeWithoutResponse',
      'read', 'getMtu', 'requestConnectionPriority']);
    gate = { started: performance.now(), events: [], dropped: 0, active: true, sequence: 0 };
    const emit = (method, call, state) => {
      if (!gate.active) return;
      if (gate.events.length >= limit) { gate.dropped++; return; }
      gate.events.push({ method, call, state, millis: performance.now() - gate.started });
    };
    const wrapped = function (plugin, method, ...args) {
      if (plugin !== 'BluetoothLe' || !methods.has(method))
        return original.call(this, plugin, method, ...args);
      const call = ++gate.sequence;
      emit(method, call, 'start');
      let result;
      try { result = original.call(this, plugin, method, ...args); }
      catch (error) { emit(method, call, 'rejected'); throw error; }
      // Observe settlement but return the exact original promise/value.
      // The handlers do not throw or retain any native result or error.
      if (result && typeof result.then === 'function') {
        result.then(() => emit(method, call, 'resolved'), () => emit(method, call, 'rejected'));
      } else emit(method, call, 'resolved');
      return result;
    };
    gate.restore = () => {
      if (cap.nativePromise !== wrapped) throw Error('BLE timing observer ownership changed');
      gate.active = false;
      if (hadOwn) cap.nativePromise = original; else delete cap.nativePromise;
    };
    cap.nativePromise = wrapped;
    globalThis[key] = gate;
    return { installed: true };
  }
  if (!gate || !['read', 'stop'].includes(mode)) throw Error('Matching BLE timing observer required');
  const result = { events: gate.events.map(event => ({ ...event })), dropped: gate.dropped };
  if (mode === 'stop') { gate.restore(); delete globalThis[key]; }
  return result;
}
