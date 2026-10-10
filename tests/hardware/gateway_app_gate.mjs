import assert from 'node:assert/strict';

// Execute a bounded, caller-supplied gate in the signed-in debug WebView.
// Never extract tokens or print arbitrary browser exception/HTTP payloads.
export async function runGatewayAppGate(action, config, timeoutMs = 180_000) {
  const pages = await (await fetch(`${config.cdp || 'http://127.0.0.1:9222'}/json/list`, {
    signal: AbortSignal.timeout(5000),
  })).json();
  const page = pages.find(value => value.type === 'page' && value.url?.includes(config.pagePath || '/gateway-enrollment')
    && value.webSocketDebuggerUrl);
  assert(page, 'Open the exact Hub enrollment page in the signed-in debug App');
  const socket = new WebSocket(page.webSocketDebuggerUrl);
  try {
    return await new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('Gate deadline; inspect live operation before retrying')), timeoutMs);
      const fail = () => { clearTimeout(timer); reject(new Error('CDP disconnected; inspect live operation')); };
      socket.onerror = fail;
      socket.onclose = fail;
      socket.onopen = () => socket.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate', params: {
        expression: `(${action.toString()})(${JSON.stringify(config)})`,
        awaitPromise: true, returnByValue: true,
      } }));
      socket.onmessage = event => {
        const message = JSON.parse(event.data);
        if (message.id !== 1) return;
        clearTimeout(timer);
        if (message.error || message.result?.exceptionDetails) {
          // Only source coordinates; arbitrary exception/HTTP text can contain secrets.
          const line = message.result?.exceptionDetails?.lineNumber;
          reject(new Error(`App gate failed${Number.isInteger(line) ? ` at expression line ${line + 1}` : ''}; inspect preconditions/recovery`));
        }
        else resolve(message.result.result.value);
      };
    });
  } finally {
    // Closing CDP is not cancellation of an App transaction. On deadline,
    // inspect its checkpoint/links rather than launching another gate.
    socket.close();
  }
}
