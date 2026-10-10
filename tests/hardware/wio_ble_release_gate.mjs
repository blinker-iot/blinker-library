#!/usr/bin/env node

const SERVICE_UUID = '5f6d0001-3f5b-4e4f-9f4d-626c696e6b32';
const TRANSMIT_UUID = '5f6d0003-3f5b-4e4f-9f4d-626c696e6b32';

function usage() {
  console.error(
    'usage: node wio_ble_release_gate.mjs --device <BLE address> ' +
      '[--cdp http://127.0.0.1:9222] [--readiness-cycles 3] ' +
      '[--auth-cycles 3] [--reconnect-cycles 32]'
  );
}

function parseArguments(argv) {
  const values = {
    cdp: 'http://127.0.0.1:9222',
    readinessCycles: 3,
    authCycles: 3,
    reconnectCycles: 32,
    deadlineMillis: 15000,
    toleranceMillis: 2000,
  };
  const names = {
    '--cdp': 'cdp',
    '--device': 'deviceId',
    '--readiness-cycles': 'readinessCycles',
    '--auth-cycles': 'authCycles',
    '--reconnect-cycles': 'reconnectCycles',
    '--deadline-ms': 'deadlineMillis',
    '--tolerance-ms': 'toleranceMillis',
  };
  for (let index = 0; index < argv.length; index += 2) {
    const key = names[argv[index]];
    const value = argv[index + 1];
    if (!key || value === undefined) throw new Error(`invalid argument: ${argv[index]}`);
    values[key] = key.endsWith('Cycles') || key.endsWith('Millis')
      ? Number.parseInt(value, 10)
      : value;
  }
  if (!values.deviceId) throw new Error('--device is required');
  for (const key of [
    'readinessCycles',
    'authCycles',
    'reconnectCycles',
    'deadlineMillis',
    'toleranceMillis',
  ]) {
    if (!Number.isInteger(values[key]) || values[key] < 0) {
      throw new Error(`${key} must be a non-negative integer`);
    }
  }
  return values;
}

async function firstPageWebSocket(cdp) {
  const response = await fetch(`${cdp.replace(/\/$/, '')}/json`);
  if (!response.ok) throw new Error(`CDP page query failed: HTTP ${response.status}`);
  const pages = await response.json();
  const page = pages.find((candidate) => candidate.type === 'page');
  if (!page?.webSocketDebuggerUrl) throw new Error('no debuggable Android WebView page');
  return page.webSocketDebuggerUrl;
}

async function evaluate(webSocketUrl, expression) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(webSocketUrl);
    const timer = setTimeout(() => {
      socket.close();
      reject(new Error('CDP evaluation timed out'));
    }, 15 * 60 * 1000);
    socket.onerror = () => {
      clearTimeout(timer);
      reject(new Error('CDP WebSocket failed'));
    };
    socket.onopen = () => {
      socket.send(
        JSON.stringify({
          id: 1,
          method: 'Runtime.evaluate',
          params: { expression, awaitPromise: true, returnByValue: true },
        })
      );
    };
    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      if (message.id !== 1) return;
      clearTimeout(timer);
      socket.close();
      const result = message.result?.result;
      if (message.result?.exceptionDetails || result?.subtype === 'error') {
        const exception = message.result?.exceptionDetails;
        reject(
          new Error(
            result?.description ||
              exception?.exception?.description ||
              exception?.text ||
              'WebView evaluation failed'
          )
        );
        return;
      }
      resolve(result?.value);
    };
  });
}

function gateExpression(config) {
  const wireConfig = JSON.stringify({
    ...config,
    serviceUuid: SERVICE_UUID,
    transmitUuid: TRANSMIT_UUID,
  });
  return `(async () => {
    const config = ${wireConfig};
    const ble = Capacitor?.Plugins?.BluetoothLe;
    if (!ble) throw new Error('Capacitor BluetoothLe plugin is unavailable');
    if (document.visibilityState !== 'visible') {
      throw new Error('the target Android WebView must remain in the foreground');
    }
    await ble.initialize({ androidNeverForLocation: true });
    const wait = (milliseconds) => new Promise((resolve) => setTimeout(resolve, milliseconds));
    const disconnectQuietly = async () => {
      try { await ble.disconnect({ deviceId: config.deviceId }); } catch {}
    };
    const timeoutCycle = async (notify) => {
      await disconnectQuietly();
      await wait(600);
      let signalDisconnect;
      const disconnected = new Promise((resolve) => { signalDisconnect = resolve; });
      const listener = await ble.addListener(
        'disconnected|' + config.deviceId,
        () => signalDisconnect(Date.now())
      );
      const startedAt = Date.now();
      try {
        await ble.connect({ deviceId: config.deviceId, timeout: 8000 });
        const connectedAt = Date.now();
        let readyAt = connectedAt;
        if (notify) {
          await ble.startNotifications({
            deviceId: config.deviceId,
            service: config.serviceUuid,
            characteristic: config.transmitUuid,
            timeout: 5000,
          });
          readyAt = Date.now();
        }
        const disconnectedAt = await Promise.race([
          disconnected,
          wait(config.deadlineMillis + config.toleranceMillis + 5000).then(() => -1),
        ]);
        return {
          connectedMillis: connectedAt - startedAt,
          readyMillis: readyAt - connectedAt,
          disconnectFromConnectMillis:
            disconnectedAt < 0 ? -1 : disconnectedAt - connectedAt,
          disconnectFromReadyMillis:
            disconnectedAt < 0 ? -1 : disconnectedAt - readyAt,
        };
      } finally {
        await listener.remove();
        await disconnectQuietly();
      }
    };
    const reconnectCycle = async () => {
      const startedAt = Date.now();
      try {
        await ble.connect({ deviceId: config.deviceId, timeout: 8000 });
        await ble.startNotifications({
          deviceId: config.deviceId,
          service: config.serviceUuid,
          characteristic: config.transmitUuid,
          timeout: 5000,
        });
        await ble.readRssi({ deviceId: config.deviceId });
        await ble.disconnect({ deviceId: config.deviceId });
        return { ok: true, elapsedMillis: Date.now() - startedAt };
      } catch (error) {
        await disconnectQuietly();
        return { ok: false, elapsedMillis: Date.now() - startedAt, error: String(error) };
      }
    };
    const report = { readiness: [], authentication: [], reconnect: [] };
    for (let index = 0; index < config.readinessCycles; ++index) {
      report.readiness.push(await timeoutCycle(false));
    }
    for (let index = 0; index < config.authCycles; ++index) {
      report.authentication.push(await timeoutCycle(true));
    }
    for (let index = 0; index < config.reconnectCycles; ++index) {
      report.reconnect.push(await reconnectCycle());
      await wait(600);
    }
    return JSON.stringify(report);
  })()`;
}

function validate(report, config) {
  const earliest = config.deadlineMillis - 2500;
  const latest = config.deadlineMillis + config.toleranceMillis;
  const failures = [];
  report.readiness.forEach((cycle, index) => {
    const elapsed = cycle.disconnectFromConnectMillis;
    if (elapsed < earliest || elapsed > latest) {
      failures.push(`readiness cycle ${index + 1}: ${elapsed} ms`);
    }
  });
  report.authentication.forEach((cycle, index) => {
    const elapsed = cycle.disconnectFromReadyMillis;
    if (elapsed < earliest || elapsed > latest) {
      failures.push(`authentication cycle ${index + 1}: ${elapsed} ms`);
    }
  });
  report.reconnect.forEach((cycle, index) => {
    if (!cycle.ok) failures.push(`reconnect cycle ${index + 1}: ${cycle.error}`);
  });
  return failures;
}

async function main() {
  let config;
  try {
    config = parseArguments(process.argv.slice(2));
  } catch (error) {
    usage();
    throw error;
  }
  if (typeof WebSocket === 'undefined') {
    throw new Error('Node.js with the built-in WebSocket API is required');
  }
  const webSocketUrl = await firstPageWebSocket(config.cdp);
  const encoded = await evaluate(webSocketUrl, gateExpression(config));
  const report = JSON.parse(encoded);
  const failures = validate(report, config);
  console.log(JSON.stringify({ config, report, passed: failures.length === 0, failures }, null, 2));
  process.exit(failures.length ? 1 : 0);
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
