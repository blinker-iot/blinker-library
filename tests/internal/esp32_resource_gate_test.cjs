'use strict';

const assert = require('assert');
const path = require('path');
const {
  loadResourceProfile,
  parseResourceSnapshot,
  requireStageAdmission,
  validateResourceSnapshot,
} = require('./lib/esp32_resource_gate.cjs');

const profilePath = path.join(
  __dirname,
  'profiles',
  'esp32s3-arduino-3.3.6-wifi-ble.test.json',
);
const profile = loadResourceProfile(profilePath);
assert.doesNotThrow(() => requireStageAdmission(profile, 'smoke'));
assert.throws(
  () => requireStageAdmission(profile, '10'),
  /requires a production resource profile/,
);
assert.doesNotThrow(() => requireStageAdmission({ ...profile, admission: 'production' }, '100'));
const taskLines = Object.entries(profile.tasks)
  .map(([name, minimum]) => `[probe] task name=${name} stackMinBytes=${minimum + 16}`)
  .join('\n');
const diagnostics = [
  `[probe] board=${profile.target.board} arduino=${profile.target.arduinoCore} idf=${profile.target.idf}`,
  `[probe] ipcStackBytes=${profile.target.ipcStackBytes} canary=1 watchpoint=1`,
  `[probe] heap free=${profile.heapMinimumBytes + 8192}`
    + ` min=${profile.heapMinimumBytes} largest=${profile.largestBlockMinimumBytes}`,
  `[probe] tasks expected=${Object.keys(profile.tasks).length} capacity=32`,
  taskLines,
].join('\n');

const snapshot = parseResourceSnapshot(diagnostics);
assert.doesNotThrow(() => validateResourceSnapshot(snapshot, profile));

assert.throws(
  () => validateResourceSnapshot({ ...snapshot, idf: 'different' }, profile),
  /SDK profile mismatch/,
);
assert.throws(
  () => validateResourceSnapshot({
    ...snapshot,
    tasks: { ...snapshot.tasks, ipc0: profile.tasks.ipc0 - 1 },
  }, profile),
  /task ipc0 stack floor failed/,
);

const missingTask = { ...snapshot, tasks: { ...snapshot.tasks } };
delete missingTask.tasks.wifi;
assert.throws(
  () => validateResourceSnapshot(missingTask, profile),
  /required task missing: wifi/,
);

console.log('esp32 resource gate: pass');
