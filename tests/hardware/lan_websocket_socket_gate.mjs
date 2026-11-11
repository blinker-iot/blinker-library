import net from 'node:net';
import { pathToFileURL } from 'node:url';

// No credentials, grants or business writes. Run only against an isolated LAN
// self test board. This exercises the actual SDK parser, unlike host stubs.
export async function runLanWebSocketSocketGate(host, port = 8920) {
  if (!net.isIPv4(host) || !Number.isInteger(port) || port < 1 || port > 65535) {
    throw new Error('Explicit test-board IPv4 and port required');
  }
  const request = (path = '/bbp2', extra = '', method = 'GET') =>
    `${method} ${path} HTTP/1.1\r\nHost: ${host}:${port}\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n`
    + 'Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n' + extra + '\r\n';
  const cases = [
    ['challenge', request(), true],
    ['zero-body', request('/bbp2', 'Content-Length: 0\r\n'), true],
    ['query', request('/bbp2?x'), false],
    ['body', request('/bbp2', 'Content-Length: 1\r\n'), false],
    ['chunked', request('/bbp2', 'Transfer-Encoding: chunked\r\n'), false],
    ['wrong-method', request('/bbp2', '', 'POST'), false],
    ['oversized-header', request('/bbp2', `X-Test: ${'x'.repeat(1024)}\r\n`), false],
    ['early-frame', Buffer.concat([Buffer.from(request()), Buffer.from([0x82, 0x80, 0, 0, 0, 0])]), false],
    ['challenge-after-rejections', request(), true],
  ];
  const results = [];
  for (const [name, wire, expectChallenge] of cases) {
    const result = await new Promise(resolve => {
      const started = Date.now(); let bytes = Buffer.alloc(0), done = false, upgraded = false;
      const socket = net.createConnection({ host, port });
      const finish = (challenge, reason) => {
        if (done) return; done = true; clearTimeout(timer); socket.destroy();
        const rejected = !upgraded && ['closed', 'ECONNRESET', 'http-rejected'].includes(reason);
        resolve({ name, pass: expectChallenge ? challenge : rejected,
          challenge, upgraded, reason, ms: Date.now() - started });
      };
      const timer = setTimeout(() => finish(false, 'timeout'), 5000);
      socket.on('connect', () => socket.write(wire));
      socket.on('data', chunk => {
        bytes = Buffer.concat([bytes, chunk]);
        if (bytes.length > 2048) return finish(false, 'oversized-response');
        const header = bytes.indexOf('\r\n\r\n');
        if (header < 0) return;
        if (!bytes.subarray(0, header).toString().startsWith('HTTP/1.1 101 ')) return finish(false, 'http-rejected');
        upgraded = true;
        const frame = bytes.subarray(header + 4);
        if (frame.length < 2) return;
        const length = frame[1] & 127;
        const offset = length === 126 ? 4 : 2;
        if (frame.length < offset) return;
        const size = length === 126 ? frame.readUInt16BE(2) : length;
        if (frame[0] !== 0x82 || (frame[1] & 128) || length === 127 || size === 0 || size > 420) {
          return finish(false, 'invalid-frame');
        }
        if (frame.length >= offset + size) finish(true, 'binary-first-packet');
      });
      socket.on('error', error => finish(false, error.code ?? 'socket-error'));
      socket.on('close', () => finish(false, 'closed'));
    });
    results.push(result);
    // Let SDK close_fn retire its single fd; never evict an active caller.
    await new Promise(resolve => setTimeout(resolve, 300));
  }
  return { pass: results.every(result => result.pass), results };
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const result = await runLanWebSocketSocketGate(process.argv[2], Number(process.argv[3] ?? 8920));
  console.log(JSON.stringify(result, null, 2));
  if (!result.pass) process.exitCode = 1;
}
