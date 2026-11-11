import { test } from 'node:test';
import assert from 'node:assert/strict';
import { assertManagementYield } from './edge_gateway_management_yield_gate.mjs';
const held = { state: 'ready', started: 4000, observed: 4050, expiresAt: 99999 };
const close = { started: 4060 };
const terminal = { state: 'closed', observed: 4200, expiresAt: 99999 };
function page(cancelled = false) {
  const phases = cancelled
    ? ['reserve-start', 'reserved', 'cancel-requested', 'navigation-away', 'ready-rejected', 'page-retired']
    : ['reserved', 'physical-ready', 'sdk-connect-start', 'page-ready', 'sdk-close-done', 'os-released'];
  return { error: '', started: 1000, finished: 8500, idle: true, counter: 0, physicalReleased: true,
    readyMs: 4500, connects: cancelled ? 0 : 1, closes: cancelled ? 0 : 1,
    events: phases.map((phase, i) => ({ phase, ms: cancelled ? 100 + i * 20 : i ? 3200 + i * 50 : 100 })) };
}
test('management original Close precedes Direct Ready; HTTP reply may be later', () => {
  assertManagementYield(page(), held, close, terminal, false, 0);
  for (const phase of ['physical-ready', 'sdk-connect-start']) {
    const early = page(); early.events.find(e => e.phase === phase).ms = 1500;
    assert.throws(() => assertManagementYield(early, held, close, terminal, false, 0));
  }
});
test('a real accepted reservation must overlap an unchanged live window', () => {
  for (const bad of [{ ...held, state: 'closed' }, { ...held, expiresAt: 100000 }, { ...held, started: 900 }]) {
    assert.throws(() => assertManagementYield(page(), bad, close, terminal, false, 0));
  }
  assert.throws(() => assertManagementYield(page(), held, close, { ...terminal, state: 'expired' }, false, 0));
});
test('cancellation leaves original management alive and observes cleanup for late acquisition', () => {
  assertManagementYield(page(true), held, close, terminal, true, 0);
  for (const patch of [{ finished: 4500 }, { connects: 1 },
    { events: [...page(true).events, { phase: 'physical-ready', ms: 4400 }] }]) {
    assert.throws(() => assertManagementYield({ ...page(true), ...patch }, held, close, terminal, true, 0));
  }
});
