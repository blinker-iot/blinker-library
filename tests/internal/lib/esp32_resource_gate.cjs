'use strict';

const fs = require('fs');
const path = require('path');

function positiveInteger(value, label) {
  if (!Number.isInteger(value) || value < 1) {
    throw new Error(`${label} must be a positive integer`);
  }
}

function loadResourceProfile(file) {
  const profile = JSON.parse(fs.readFileSync(path.resolve(file), 'utf8'));
  if (profile.schemaVersion !== 1) throw new Error('unsupported resource profile');
  if (!['test-only', 'production'].includes(profile.admission)) {
    throw new Error('resource profile admission must be test-only or production');
  }
  if (!profile.target || typeof profile.target !== 'object') {
    throw new Error('resource profile target is required');
  }
  if (typeof profile.target.board !== 'string'
    || typeof profile.target.arduinoCore !== 'string'
    || typeof profile.target.idf !== 'string'
    || !profile.target.board || !profile.target.arduinoCore || !profile.target.idf) {
    throw new Error('resource profile SDK identity is required');
  }
  positiveInteger(profile.target.ipcStackBytes, 'target.ipcStackBytes');
  if (!profile.tasks || typeof profile.tasks !== 'object'
    || Array.isArray(profile.tasks) || Object.keys(profile.tasks).length === 0) {
    throw new Error('resource profile tasks are required');
  }
  positiveInteger(profile.heapMinimumBytes, 'heapMinimumBytes');
  positiveInteger(profile.largestBlockMinimumBytes, 'largestBlockMinimumBytes');
  for (const [name, minimum] of Object.entries(profile.tasks)) {
    if (!name || name.includes('\n') || name.includes('\r')) {
      throw new Error('invalid task name in resource profile');
    }
    positiveInteger(minimum, `tasks.${name}`);
  }
  return profile;
}

function requireStageAdmission(profile, stage) {
  if (stage !== 'smoke' && profile.admission !== 'production') {
    throw new Error(
      `stage ${stage} requires a production resource profile; got ${profile.admission}`,
    );
  }
}

function parseResourceSnapshot(diagnostics) {
  const versions = diagnostics.match(
    /board=([^\s]+) arduino=([^\s]+) idf=([^\s]+)/,
  );
  const sdk = diagnostics.match(
    /ipcStackBytes=(\d+) canary=(\d+) watchpoint=(\d+)/,
  );
  const heap = [...diagnostics.matchAll(
    /heap free=(\d+) min=(\d+) largest=(\d+)/g,
  )].at(-1);
  const expectedTasks = Number(diagnostics.match(/tasks expected=(\d+)/)?.[1]);
  const tasks = Object.fromEntries(
    [...diagnostics.matchAll(/task name=(.+?) stackMinBytes=(\d+)/g)]
      .map(match => [match[1], Number(match[2])]),
  );
  if (!versions || !sdk || !heap || !expectedTasks
    || Object.keys(tasks).length !== expectedTasks) {
    throw new Error('resource diagnostics are incomplete');
  }
  return {
    board: versions[1],
    arduinoCore: versions[2],
    idf: versions[3],
    ipcStackBytes: Number(sdk[1]),
    canary: Number(sdk[2]),
    watchpoint: Number(sdk[3]),
    heapFree: Number(heap[1]),
    heapMinimum: Number(heap[2]),
    largestBlock: Number(heap[3]),
    tasks,
  };
}

function validateResourceSnapshot(snapshot, profile) {
  const target = profile.target;
  if (snapshot.board !== target.board
    || snapshot.arduinoCore !== target.arduinoCore
    || snapshot.idf !== target.idf
    || snapshot.ipcStackBytes !== target.ipcStackBytes
    || snapshot.canary !== 1 || snapshot.watchpoint !== 1) {
    throw new Error(`SDK profile mismatch: ${JSON.stringify({
      board: snapshot.board,
      arduinoCore: snapshot.arduinoCore,
      idf: snapshot.idf,
      ipcStackBytes: snapshot.ipcStackBytes,
      canary: snapshot.canary,
      watchpoint: snapshot.watchpoint,
    })}`);
  }
  if (snapshot.heapMinimum < profile.heapMinimumBytes) {
    throw new Error(
      `heap floor failed: ${snapshot.heapMinimum} < ${profile.heapMinimumBytes}`,
    );
  }
  if (snapshot.largestBlock < profile.largestBlockMinimumBytes) {
    throw new Error(
      `largest block floor failed: ${snapshot.largestBlock} < ${profile.largestBlockMinimumBytes}`,
    );
  }
  for (const [name, minimum] of Object.entries(profile.tasks)) {
    if (!(name in snapshot.tasks)) throw new Error(`required task missing: ${name}`);
    if (snapshot.tasks[name] < minimum) {
      throw new Error(`task ${name} stack floor failed: ${snapshot.tasks[name]} < ${minimum}`);
    }
  }
}

module.exports = {
  loadResourceProfile,
  parseResourceSnapshot,
  requireStageAdmission,
  validateResourceSnapshot,
};
