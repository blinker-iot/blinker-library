'use strict';

const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const {
  loadResourceProfile,
  parseResourceSnapshot,
  requireStageAdmission,
  validateResourceSnapshot,
} = require('./lib/esp32_resource_gate.cjs');

function argumentsOf(argv) {
  const values = {};
  for (let index = 0; index < argv.length; index += 2) {
    const name = argv[index];
    if (!name.startsWith('--') || index + 1 >= argv.length) {
      throw new Error(`invalid argument near ${name}`);
    }
    values[name.slice(2)] = argv[index + 1];
  }
  return values;
}

const args = argumentsOf(process.argv.slice(2));
const appRoot = args['app-root'] || process.env.BLINKER_APP_ROOT || 'D:\\codes\\blinker-app';
const adb = args.adb || process.env.BLINKER_ADB
  || 'D:\\Android\\Sdk\\platform-tools\\adb.exe';
const nodeModules = path.join(appRoot, 'node_modules');
const WebSocket = require(require.resolve('ws', { paths: [nodeModules] }));
const androidSerial = args.android;
const serialPort = args.port;
const logicalDeviceId = args.device;
const provisioningName = args.provisioning;
const ssid = args.ssid;
const wifiPassword = process.env.BLINKER_TEST_WIFI_PASSWORD;
const stage = args.stage || 'smoke';
const count = stage === 'smoke' ? 1 : Number(stage);
const previousOutput = args.previous;
const output = args.output;
const resourceProfilePath = args['resource-profile'];
const directReadyTimeout = Number(args['direct-ready-timeout'] || 15_000);
const packageName = args.package || 'iot.diandeng.tech.debug';
const cdpPort = Number(args['cdp-port'] || 9222);

for (const [name, value] of Object.entries({
  android: androidSerial,
  port: serialPort,
  device: logicalDeviceId,
  provisioning: provisioningName,
  ssid,
})) {
  if (!value) throw new Error(`--${name} is required`);
}
if (!/^COM\d+$/i.test(serialPort)) throw new Error('invalid serial port');
if (!['smoke', '10', '30', '100'].includes(stage)) {
  throw new Error('--stage must be smoke, 10, 30 or 100');
}
if (!Number.isInteger(directReadyTimeout) || directReadyTimeout < 1) {
  throw new Error('invalid Direct ready timeout');
}
if (!wifiPassword) throw new Error('BLINKER_TEST_WIFI_PASSWORD is required');
if (!resourceProfilePath) throw new Error('--resource-profile is required');
if (stage !== 'smoke' && !output) throw new Error('--output is required for a formal stage');

function lastGatePass(file) {
  const lines = fs.readFileSync(path.resolve(file), 'utf8').trim().split(/\r?\n/);
  for (let index = lines.length - 1; index >= 0; index -= 1) {
    const value = JSON.parse(lines[index]);
    if (value.step === 'gate-pass') return value;
  }
  throw new Error(`previous stage has no gate-pass: ${file}`);
}

const resourceProfile = loadResourceProfile(resourceProfilePath);
const resolvedResourceProfilePath = path.resolve(resourceProfilePath);
requireStageAdmission(resourceProfile, stage);
const expectedPreviousCycles = stage === '30' ? 10 : stage === '100' ? 30 : 0;
if (expectedPreviousCycles !== 0) {
  if (!previousOutput) throw new Error(`--previous is required for stage ${stage}`);
  const previous = lastGatePass(previousOutput);
  if (previous.stage !== String(expectedPreviousCycles)
    || previous.cycles !== expectedPreviousCycles
    || previous.logicalDeviceId !== logicalDeviceId
    || previous.provisioningName !== provisioningName
    || previous.resourceProfile !== resolvedResourceProfilePath) {
    throw new Error(`stage ${stage} requires a matching ${expectedPreviousCycles}-cycle result`);
  }
}

function run(file, values, options = {}) {
  return execFileSync(file, values, {
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
    ...options,
  }).trim();
}

function adbRun(...values) {
  return run(adb, ['-s', androidSerial, ...values]);
}

function serial(command, seconds) {
  const script = [
    `$p=[System.IO.Ports.SerialPort]::new('${serialPort}',115200,'None',8,'One')`,
    '$p.DtrEnable=$false;$p.RtsEnable=$false;$p.ReadTimeout=200;$p.Open()',
    'Start-Sleep -Milliseconds 250;$p.DiscardInBuffer()',
    `$p.Write('${command}')`,
    `$until=[DateTime]::UtcNow.AddSeconds(${seconds})`,
    'while([DateTime]::UtcNow -lt $until){try{$line=$p.ReadLine();if($line){$line.Trim()}}catch [System.TimeoutException]{}}',
    '$p.Close()',
  ].join(';');
  return run('powershell.exe', ['-NoProfile', '-Command', script]);
}

function record(value) {
  const line = JSON.stringify(value);
  console.log(line);
  if (output) fs.appendFileSync(output, `${line}\n`, 'utf8');
}

function visible(expression) {
  return `[...document.querySelectorAll(${JSON.stringify(expression)})]`
    + '.filter(element => element.offsetParent !== null)';
}

class Cdp {
  constructor(socket) {
    this.socket = socket;
    this.sequence = 0;
    this.pending = new Map();
    socket.on('message', data => {
      const message = JSON.parse(String(data));
      const request = this.pending.get(message.id);
      if (!request) return;
      this.pending.delete(message.id);
      clearTimeout(request.timer);
      request.resolve(message);
    });
  }

  static async connect(port) {
    const response = await fetch(`http://127.0.0.1:${port}/json`, {
      signal: AbortSignal.timeout(10_000),
    });
    const targets = await response.json();
    const page = targets.find(target => target.type === 'page');
    if (!page) throw new Error('app WebView was not found');
    const socket = new WebSocket(page.webSocketDebuggerUrl);
    await new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        socket.terminate();
        reject(new Error('app WebView connection timed out'));
      }, 10_000);
      socket.once('open', () => {
        clearTimeout(timer);
        resolve();
      });
      socket.once('error', error => {
        clearTimeout(timer);
        reject(error);
      });
    });
    return new Cdp(socket);
  }

  evaluate(expression, awaitPromise = false, timeoutMs = 15_000) {
    return new Promise((resolve, reject) => {
      const id = ++this.sequence;
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error('CDP evaluation timed out'));
      }, timeoutMs);
      this.pending.set(id, {
        timer,
        resolve: message => {
          if (message.error || message.result?.exceptionDetails) {
            reject(new Error(`CDP evaluation failed: ${JSON.stringify(message)}`));
            return;
          }
          resolve(message.result.result.value);
        },
      });
      try {
        this.socket.send(JSON.stringify({
          id,
          method: 'Runtime.evaluate',
          params: { expression, returnByValue: true, awaitPromise },
        }));
      } catch (error) {
        clearTimeout(timer);
        this.pending.delete(id);
        reject(error);
      }
    });
  }

  async waitFor(expression, timeoutMs, label) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      if (await this.evaluate(expression)) return;
      await new Promise(resolve => setTimeout(resolve, 250));
    }
    throw new Error(`${label} timed out`);
  }

  close() {
    this.socket.terminate();
  }
}

function prepareAndroid() {
  const powerState = adbRun('shell', 'dumpsys', 'power');
  if (!/mWakefulness=Awake/.test(powerState)) {
    throw new Error('Android device must be connected, awake and unlocked');
  }
  adbRun('reverse', 'tcp:7000', 'tcp:17000');
  adbRun('reverse', 'tcp:8884', 'tcp:8884');
  adbRun('shell', 'monkey', '-p', packageName, '-c', 'android.intent.category.LAUNCHER', '1');
  const pid = adbRun('shell', 'pidof', packageName);
  if (!pid) throw new Error('app process was not started');
  try { adbRun('forward', '--remove', `tcp:${cdpPort}`); } catch {}
  adbRun('forward', `tcp:${cdpPort}`, `localabstract:webview_devtools_remote_${pid}`);
}

async function openDevice(cdp) {
  const id = JSON.stringify(logicalDeviceId);
  await cdp.evaluate(`(async()=>{
    const root=ng.getComponent(document.querySelector('app-root'));
    await root.userService.getAllInfo();
    await root.navCtrl.navigateRoot('/device/'+encodeURIComponent(${id}));
  })()`, true);
  await cdp.waitFor(
    `location.pathname==='/device/'+encodeURIComponent(${id})`,
    15_000,
    'device page',
  );
}

async function provisionCycle(cdp, cycle) {
  const startedAt = Date.now();
  const mark = (step, extra = {}) => record({
    stage, cycle, ms: Date.now() - startedAt, step, ...extra,
  });
  await cdp.evaluate(`${visible('ion-button[href*="/settings"]')}[0].click()`);
  await cdp.waitFor(
    `location.pathname.endsWith('/settings')&&${visible('button')}.some(b=>b.innerText.trim()==='开始')`,
    10_000,
    'settings page',
  );
  await cdp.evaluate(`${visible('button')}.find(b=>b.innerText.trim()==='开始').click()`);
  await cdp.waitFor(
    `${visible('button')}.some(b=>b.innerText.trim()==='我已重置网络')`,
    5_000,
    'network reset confirmation',
  );
  // Match the product flow: leave the control page first, then reset the
  // device while the App is showing the reset guide. Resetting over Serial on
  // the live control page would intentionally trigger its Direct retry budget
  // and create a test-only race with WiFiProv.
  const resetLog = serial('n', 7);
  if (!resetLog.includes('resetNetwork=') || !/state=2|state=provisioning/.test(resetLog)) {
    throw new Error('device did not enter provisioning');
  }
  mark('provisioning');
  await cdp.evaluate(`${visible('button')}.find(b=>b.innerText.trim()==='我已重置网络').click()`);
  await cdp.waitFor(
    `location.pathname==='/tools/esp32-provision'&&${visible('button')}.some(b=>b.innerText.trim()==='扫描 ESP32')`,
    10_000,
    'provisioning page',
  );
  await cdp.evaluate(`${visible('button')}.find(b=>b.innerText.trim()==='扫描 ESP32').click()`);

  const provisionName = JSON.stringify(provisioningName);
  await cdp.waitFor(
    `${visible('button.device-row')}.some(b=>b.querySelector('strong')?.textContent.trim()===${provisionName})`,
    20_000,
    'provisioning advertisement',
  );
  mark('advertisement');
  await cdp.evaluate(
    `${visible('button.device-row')}.find(b=>b.querySelector('strong')?.textContent.trim()===${provisionName}).click()`,
  );

  const networkName = JSON.stringify(ssid);
  await cdp.waitFor(
    `document.querySelector('#wifi-password')&&${visible('button.network-row')}`
      + `.some(b=>b.querySelector('strong')?.textContent.trim()===${networkName})`,
    30_000,
    'Security1 session and network scan',
  );
  mark('security1-ready');
  await cdp.evaluate(`(()=>{
    ${visible('button.network-row')}.find(b=>b.querySelector('strong')?.textContent.trim()===${networkName}).click();
    const input=document.querySelector('#wifi-password');
    Object.getOwnPropertyDescriptor(HTMLInputElement.prototype,'value').set.call(
      input,${JSON.stringify(wifiPassword)}
    );
    input.dispatchEvent(new Event('input',{bubbles:true}));
  })()`);
  await cdp.waitFor(
    `${visible('button')}.some(b=>b.innerText.trim()==='发送 Wi-Fi 配置'&&!b.disabled)`,
    5_000,
    'provision command',
  );
  await cdp.evaluate(`${visible('button')}.find(b=>b.innerText.trim()==='发送 Wi-Fi 配置').click()`);
  mark('credentials-sent');

  const devicePath = `/device/${encodeURIComponent(logicalDeviceId)}`;
  await cdp.waitFor(`location.pathname===${JSON.stringify(devicePath)}`, 60_000, 'device navigation');
  const devicePageAt = Date.now();
  mark('device-page');
  const pageText = `document.querySelector('app-device-v2-page')?.innerText||''`;
  await cdp.waitFor(
    `(${pageText}).includes('蓝牙已连接')`, directReadyTimeout, 'Direct handoff',
  );
  const directReadyMs = Date.now() - devicePageAt;
  mark('direct-ready', { directReadyMs });

  const before = String(await cdp.evaluate(pageText));
  const revision = Number(before.match(/State\s+(\d+)/)?.[1]);
  const power = /power\s+true/.test(before);
  if (!Number.isInteger(revision)) throw new Error('state revision is unavailable');
  await cdp.evaluate(`${visible('app-device-v2-page ion-toggle')}[0].click()`);
  await cdp.waitFor(
    `(()=>{const text=${pageText};const match=text.match(/State\\s+(\\d+)/);return !!match&&Number(match[1])!==${revision}`
      + `&&/power\\s+${power ? 'false' : 'true'}/.test(text)})()`,
    15_000,
    'Direct command acknowledgement',
  );
  mark('direct-command');

  const diagnostics = serial('d', 8);
  if (!/state=active/.test(diagnostics)
    || !/network=1 local=1 cloud=1/.test(diagnostics)
    || !/operationFailures=0/.test(diagnostics)
    || !/stateTransportErrors=0/.test(diagnostics)) {
    throw new Error('device diagnostics did not pass');
  }
  const resources = parseResourceSnapshot(diagnostics);
  validateResourceSnapshot(resources, resourceProfile);
  return {
    stage,
    cycle,
    durationMs: Date.now() - startedAt,
    heapFree: resources.heapFree,
    heapMinimum: resources.heapMinimum,
    largestBlock: resources.largestBlock,
    directReadyMs,
    tasks: resources.tasks,
  };
}

(async () => {
  prepareAndroid();
  const cdp = await Cdp.connect(cdpPort);
  const results = [];
  try {
    await openDevice(cdp);
    for (let cycle = 1; cycle <= count; cycle += 1) {
      const result = await provisionCycle(cdp, cycle);
      results.push(result);
      record({ step: 'cycle-pass', ...result });
    }
  } finally {
    cdp.close();
  }
  const taskNames = [...new Set(results.flatMap(result => Object.keys(result.tasks)))];
  const taskMinima = Object.fromEntries(taskNames.map(name => [
    name,
    Math.min(...results.map(result => result.tasks[name] ?? Number.MAX_SAFE_INTEGER)),
  ]));
  record({
    step: 'gate-pass',
    stage,
    cycles: results.length,
    logicalDeviceId,
    provisioningName,
    heapMinimum: Math.min(...results.map(result => result.heapMinimum)),
    largestBlockMinimum: Math.min(...results.map(result => result.largestBlock)),
    directReadyMaximumMs: Math.max(...results.map(result => result.directReadyMs)),
    resourceAdmission: resourceProfile.admission,
    resourceProfile: resolvedResourceProfilePath,
    taskMinima,
  });
})().catch(error => {
  record({ step: 'gate-failed', error: String(error?.message || error) });
  process.exitCode = 1;
});
